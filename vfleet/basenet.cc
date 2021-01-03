/****************************************************************************
 * basenet.cc for pvm 2.4
 * Author Joel Welling
 * Copyright 1992, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if ( CRAY_ARCH_T3D | CRAY_ARCH_T3E | MPP_SUPPORT )
#include <sys/types.h>
#include <unistd.h>
#endif

#include "basenet.h"
#include "servman.h" // to get name and loc of service manager
#include "logger.h"

#include "net_msgnames.h"

/*

 */

const char SMName[]= "ServiceManager";
const int SMInstance= 0;

int baseNet::netsnd_ack_messages[2]= { BASENET_ACK, BASENET_NAK };

int baseNet::initialized= 0;

int baseNet::awaiting_ack= 0;

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
int* baseNet::partner_tid_table= NULL;
int baseNet::partner_tid_table_size= 0;
int baseNet::partner_tid_table_max= 0;
int baseNet::partner_tid_table_current= 0;
int baseNet::check_partners_only= 0;
#endif

NetMsgType baseNet::last_ack_msg= BASENET_LASTMSG;

int baseNet::debug_static= 0;

char *baseNet::net_name= NULL;

int baseNet::net_instance= -1; // invalid value

RemObjInfo *baseNet::default_server= NULL;

NetMsgHandlerList *baseNet::msg_handlers= NULL;

RemObjInfo baseNet::self_info= RemObjInfo();

int RemObjInfo::server_tid= 0;

#ifdef MPP_SUPPORT
int baseServer::n_mpp_procs= 0;

RemObjInfo* baseServer::mpp_server_table= NULL;

int baseServer::mpp_server_index= -1;

char* baseServer::server_objectfile= NULL;

char* baseServer::server_arch= NULL;
#endif

RemObjInfo::RemObjInfo()
{
  tid= 0;
  for (int i=0; i<8; i++) bytes[i]= 0;
  object= 0;
}

RemObjInfo::RemObjInfo( const char *comp, const int inst, const long obj )
{
  if (!strcmp(comp, SMName) && inst == SMInstance) {
    if (!server_tid) {
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
      // Get server tid from file, since PVM groups don't extend
      // across T3D boundary
      char kludge_fname[64];
      sprintf(kludge_fname,"/tmp/vfleet_group_kludge.%d",(int)getuid());
      FILE *tid_file= fopen(kludge_fname,"r");
      if (!tid_file) {
	fprintf(stderr,
	     "RemObjInfo::RemObjInfo: service manager tid file not found!\n");
	pvm_exit();
	exit(-1);
      }
      if (fscanf(tid_file,"%d",&server_tid) != 1) {
	fprintf(stderr,
	     "RemObjInfo::RemObjInfo: service manager tid file not valid!\n");
	pvm_exit();
	exit(-1);
      }
      fclose(tid_file);
#else
      server_tid= pvm_gettid( (char*)comp, inst );
#endif
    }
    tid= server_tid;
  }
  else {
    // On SGI/Cray T3D and T3E, this only works if the other process
    // is also on the T3D or T3E, but there's no other option.  On
    // normal machines it always works.
    tid= pvm_gettid( (char *)comp, inst );
  }
  for (int i=0; i<8; i++) bytes[i]= 0;
  object= obj;
}

RemObjInfo::RemObjInfo( const int tid_in, const long obj )
{
  tid= tid_in;
  for (int i=0; i<8; i++) bytes[i]= 0;
  object= obj;
}

RemObjInfo::~RemObjInfo()
{
}

RemObjInfo& RemObjInfo::operator=( const RemObjInfo& a )
{
  if (this != &a) { // beware of a=a
    tid= a.tid;
    for (int i=0; i<8; i++) bytes[i]= a.bytes[i];
//    object= a.object;
  }
  return(*this);
}

RemObjInfo::RemObjInfo(const RemObjInfo& a)
{
  tid= a.tid;
  for (int i=0; i<8; i++) bytes[i]= a.bytes[i];
//  object= a.object;
}

int RemObjInfo::operator==( const RemObjInfo& other )
{
  return( (tid==other.tid) && (object==other.object));
}

int RemObjInfo::operator!=( const RemObjInfo& other )
{
  return( (tid != other.tid) || (object != other.object));
}

// This method creates a baseNet node, and connects it to a remote partner
// The remote partner is waiting for the ACK which connect sends.
baseNet::baseNet( RemObjInfo *rem_in )
{
  // Initialize PVM
  if (!initialized) initialize("NoName");
  
  // Housekeeping
  connected= 0;
  debug= 0;
  
  if (rem_in) { // Null info means sit and wait for remote partner to call
    
    // Save a copy of the remote info
    rem= new RemObjInfo;
    *rem= *rem_in;  // Mediated by special copy method
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    pvm_setopt(PvmAutoErr, 0);
    if (pvm_get_PE(rem->tid) >= 0) {
      partner_same_host= 1;
      add_partner_tid( rem->tid );
    }
    else partner_same_host= 0;
    pvm_setopt(PvmAutoErr, 1);
#else
#ifdef MPP_SUPPORT
    partner_same_host= 0; // probably partner on T3D, but no easy way to tell
#else
    if (pvm_tidtohost(rem->tid) == pvm_tidtohost(self_info.tid)) 
      partner_same_host= 1;
    else partner_same_host= 0;
#endif
#endif
    
    // Make contact with remote partner
    connect();
  }
  else { // Get ready to sit and wait
    rem= NULL;
  }
}

// This method creates a baseNet node, and requests the given server
// create a remote partner to call up and connect to it.
baseNet::baseNet( NetMsgType msg, char *info_str, RemObjInfo *server_rem )
{
  // Initialize PVM
  if (!initialized) initialize("NoName");

  // Make sure server is out there 
  if (pvm_pstat(server_rem->tid) != PvmOk ) {
    fprintf(stderr,"baseNet::baseNet: server tid %x is not running!\n",
	    server_rem->tid);
    pvm_exit();
    exit(-1);
  }

  // Housekeeping
  connected= 0;
  debug= 0;
  
  // Set up to talk to server\'s parent_receive
  rem= new RemObjInfo(*server_rem);
  partner_same_host= 0;
  
  // Transmit the message over PVM to the server
  if (debug)
    fprintf(stderr,"baseNet::baseNet: sending <%s> to tid %x, object %ld\n",
	    MsgInfo[msg].name, rem->tid, rem->object);
  bufsetup();
  netputreminfo();  // pack up info about self
  netputstring(info_str);
  if ( !netsnd(msg) )
    fprintf(stderr,"baseNet::baseNet: requested partner not acquired!\n");
  else {
    // Remote object information rides the ACK back to us
    netgetreminfo(rem);
    if (debug) 
      fprintf(stderr,
	      "baseNet::baseNet: %x %ld acquired remote partner %x %ld\n",
	      self_info.tid,self_info.object, rem->tid, rem->object);
    connected= 1;
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    pvm_setopt(PvmAutoErr, 0);
    if (pvm_get_PE(rem->tid) >= 0) {
      partner_same_host= 1;
      add_partner_tid(rem->tid);
    }
    else partner_same_host= 0;
    pvm_setopt(PvmAutoErr, 1);
#else
#ifdef MPP_SUPPORT
    partner_same_host= 0; // probably partner on T3D, but no easy way to tell
#else
    if (pvm_tidtohost(rem->tid) == pvm_tidtohost(self_info.tid)) 
      partner_same_host= 1;
    else partner_same_host= 0;
#endif
#endif
  }
}

baseNet::~baseNet()
{
  // Warn the remote partner about deletion
  bufsetup();
  if (connected) netsnd(BASENET_DELETED);

  // Clean up
  delete rem;
}

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
void baseNet::add_partner_tid( const int tid )
{
  int tid_found= 0;
  int i;
  for (i=0; i<partner_tid_table_max; i++) {
    if (partner_tid_table[i] == tid) {
      tid_found= 1;
      break;
    }
  }
  if (!tid_found) {
    if (partner_tid_table_max >= partner_tid_table_size) {
      fprintf(stderr,"Growing partner table: %d to %d!\n",
	      partner_tid_table_size, 2*partner_tid_table_size);
      partner_tid_table_size= 2*partner_tid_table_size;
      int* new_table= new int[partner_tid_table_size];
      for (i=0; i<partner_tid_table_max; i++)
	new_table[i]= partner_tid_table[i];
      delete [] partner_tid_table;
      partner_tid_table= new_table;
    }
    partner_tid_table[partner_tid_table_max++]= tid;
  }
}
#endif

void baseNet::connect()
{
  // Make sure initialization is complete
  if (!rem) {
    fprintf(stderr,"baseNet::connect: remote partner info uninitialized!\n");
    return;
  }

  // Make sure not already connected
  if (connected) {
    fprintf(stderr,
	    "baseNet::connect: tried to connect to remote partner twice!\n");
    return;
  }

  // Transact the connection;  other side is awaiting ACK
  if (partner_same_host) (void)pvm_initsend(PvmDataRaw);
  else (void)pvm_initsend(PvmDataDefault);
  netputnint((int *)&last_ack_msg, 1);
  int one= 1;
  netputnint(&one, 1); // ACK success code
  netputreminfo();  // pack up info about self

  // Break ack lock manually, since this isn\'t normal ACK path.
  break_ack_lock();  
  // The transaction never fails, because BASENET_ACK requires no ACK
  (void)netsnd(BASENET_ACK);

  connected= 1;
}

void baseNet::bufsetup()
{
  // The first thing that goes in a message is the object it is intended
  // for;  the dispatcher on the other end gets this info and delivers
  // the message.
  if (partner_same_host) (void)pvm_initsend(PvmDataRaw);
  else (void)pvm_initsend(PvmDataDefault);
#ifdef never
  fprintf(stderr,"bufsetup: putting %x_%x_%x_%x_%x_%x_%x_%x\n",
	  rem->bytes[0],rem->bytes[1],rem->bytes[2],rem->bytes[3],
	  rem->bytes[4],rem->bytes[5],rem->bytes[6],rem->bytes[7]);
#endif
  netputbytes( (char *)(rem->bytes), 8 );
}

int baseNet::netsnd( NetMsgType msg )
{
  // Refuse to do this is we are waiting to send an ack of another message
  if (awaiting_ack && (debug || debug_static)) {
    fprintf(stderr,
      "baseNet::netsnd: warning: sending <%s> to %x %ld while awaiting ack!\n",
	    MsgInfo[msg].name, partner_rem()->tid, partner_rem()->object);
  }

  // Transmit the message over PVM
  if (debug) 
    fprintf(stderr,"baseNet::netsnd: sending <%s> to tid %x, object %ld\n",
	    MsgInfo[msg].name, rem->tid, rem->object);
  pvm_send( rem->tid, msg );

  if (MsgInfo[msg].ack_expected) {
    int reply, replyto;
    int msg_object= 0;
    int success;

    // Wait for the acknowledgement message
    if (debug) 
      fprintf(stderr,"baseNet::netsnd: %s waiting on ack of %s\n",
	      get_net_name(), MsgInfo[msg].name);
    pvm_recv(-1, BASENET_ACK);
    reply= BASENET_ACK;

    // First element of ack should be message being acknowledged
    // Second element is success code
    replyto= BASENET_LASTMSG;
    if ((reply==BASENET_ACK || reply==BASENET_NAK)) {
      netgetnint( &replyto, 1 );
      netgetnint( &success, 1 );
    }

    if (debug) 
      fprintf(stderr,"baseNet::netsnd:  %s got <%s> to <%s>: %s\n",
	    get_net_name(),MsgInfo[reply].name,MsgInfo[replyto].name,
	    (success ? "success" : "failure!") );
    if (!success) return(0);

  }
  
  return(1);
}

// This routine gets called in the process\'s main event loop.  It
// dispatches incoming messages to the appropriate objects.  It returns
// if no messages are available.
void baseNet::service()
{
  // Do a non-blocking probe to see if there is a message to handle.
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  if (check_partners_only) { 
    for (int i=0; i<partner_tid_table_max; i++) {
      if (partner_tid_table[partner_tid_table_current]) {
	if ( pvm_probe(partner_tid_table[partner_tid_table_current], -1) )
	  service_blocking();
      }
      partner_tid_table_current++;
      if (partner_tid_table_current>=partner_tid_table_max)
	partner_tid_table_current= 0;
    }
  }
  else if ( pvm_probe(-1, -1) ) service_blocking();
#else
  if ( pvm_probe(-1, -1) ) service_blocking();
#endif
}

// This routine gets called in the process\'s main event loop.  It
// dispatches incoming messages to the appropriate objects.  It blocks
// until an event is received.
void baseNet::service_blocking()
{
  // Want to dispatch only one message per call, so in the event of
  // a flood of messages the user interface will still be able to get
  // a word in edgewise.

  NetMsgType msg;
  RemObjInfo dest_info;
  baseNet *obj;
  int rcv_ok;
  int buf_id;
  int bytes;
  int sender_tid;
  int tmptag;

  if (awaiting_ack) 
    // probably got here due to timer interrupt;  
    // do nothing to avoid disrupting ack mechanism
    return;

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  if (check_partners_only) { 
    // Loop forever, waiting for an incoming message
    buf_id= 0;
    while (buf_id == 0) {
      if (partner_tid_table[partner_tid_table_current])
	buf_id= pvm_nrecv(partner_tid_table[partner_tid_table_current], -1);
      if (buf_id < 0) {
	fprintf(stderr,
		"baseNet::service_blocking: pvm_nrecv failed, msg lost!\n");
	return;
      }
      if (buf_id == 0) {
	partner_tid_table_current++;
	if (partner_tid_table_current>=partner_tid_table_max)
	  partner_tid_table_current= 0;
      }
    }
  }
  else if ( (buf_id= pvm_recv(-1,-1))<0 ) {
    fprintf(stderr,"baseNet::service_blocking: pvm_recv failed, msg lost!\n");
    return;
  }
#else
  if ( (buf_id= pvm_recv(-1,-1))<0 ) {
    fprintf(stderr,"baseNet::service_blocking: pvm_recv failed, msg lost!\n");
    return;
  }
#endif
  if ( pvm_bufinfo( buf_id, &bytes, &tmptag, &sender_tid ) ) {
    fprintf(stderr,
	    "baseNet::service_blocking: pvm_bufinfo failed, msg lost!\n");
    return;
  }
  msg= (NetMsgType)tmptag;
  if (debug_static) 
    fprintf(stderr,"baseNet::service_blocking: got <%s> from tid %x\n",
	    MsgInfo[msg].name, sender_tid);
  
  if ( msg==BASENET_ACK || msg==BASENET_NAK ) {
 fprintf(stderr,"baseNet::service_blocking: got misdirected <%s>, ignored!\n",
	    MsgInfo[msg].name);
    return;
  }

  if ( pvm_upkbyte( (char *)(dest_info.bytes), 8, 1 ) )    
    fprintf(stderr,
	    "baseNet::service_blocking: unable to get object pointer!\n");

#ifdef never
  fprintf(stderr,"service_blocking: got %x_%x_%x_%x_%x_%x_%x_%x\n",
	  dest_info.bytes[0],dest_info.bytes[1],dest_info.bytes[2],
	  dest_info.bytes[3],dest_info.bytes[4],dest_info.bytes[5],
	  dest_info.bytes[6],dest_info.bytes[7]);
#endif

  obj= (baseNet *)(dest_info.object);
  
  if (MsgInfo[msg].ack_expected) {
    awaiting_ack= 1;
    last_ack_msg= msg;
  }
  
  if (dest_info.object)
    rcv_ok= obj->netrcv(msg);
  else rcv_ok= parent_receive(msg);  // no object given; use default handler

  if (awaiting_ack) { 
    awaiting_ack= 0;
    if (debug_static) {
	if (rcv_ok) 
	  fprintf(stderr,"baseNet::service_blocking: acknowledging <%s>\n",
		  MsgInfo[msg].name);
	else
	  fprintf(stderr,"baseNet::service_blocking: nak-acknowledging <%s>\n",
		  MsgInfo[msg].name);
      }

    // Construct ACK/NAK message.  First comes message being ACKed,
    // second comes success code.
    if (obj->partner_same_host) (void)pvm_initsend(PvmDataRaw);
    else (void)pvm_initsend(PvmDataDefault);
    pvm_pkint( (int *)&last_ack_msg, 1, 1 );    
    int one= 1;
    int zero= 0;
    if (rcv_ok) pvm_pkint( &one, 1, 1 );
    else pvm_pkint( &zero, 1, 1 );
    if (dest_info.object) obj->add_ack_payload(msg, rcv_ok);
    obj->netsnd(BASENET_ACK);
  }
}

int baseNet::netrcv(NetMsgType msg)
{
  int rcv_ok= 1;
  // Here we handle the message types the baseNet object recognizes

  if (debug) fprintf(stderr,"baseNet::netrcv: got <%s>\n",MsgInfo[msg].name);

  switch (msg) {
    
  case BASENET_DELETED:
    // The remote partner has been deleted.
    if (debug) fprintf(stderr,
	    "baseNet::netrcv: remote partner deleted; deleting self.\n");
    connected= 0;
    delete this;  // Default action is to die when the partner does.
    break;

  case BASENET_DEBUG_ON:
    debug= 1;
    fprintf(stderr,"baseNet::netrcv: debugging on.\n");
    break;

  case BASENET_DEBUG_OFF:
    if (debug) fprintf(stderr,"baseNet::netrcv: debugging on.\n");
    debug= 0;
    break;

  case BASENET_REQSERVER:
  case BASENET_ACK:
  case BASENET_NAK:
    fprintf(stderr,"baseNet::netrcv: got misdirected <%s>; ignored!\n",
	    MsgInfo[msg].name);
    break;
    
  default: 
    fprintf(stderr,
	    "baseNet::netrcv: got and ignored unknown message, ID= %d\n",
	    msg);
  }
  return( rcv_ok );
}

void baseNet::add_ack_payload(NetMsgType msg, const int netrcv_retval)
{
  // Default action is that there is no data passed with an ack/nak msg
}

// If the send buffer overflows, this gets called
void baseNet::putbufferr()
{
  fprintf(stderr,"baseNet::putbufferr: send buffer overflow, exiting!\n");
  abort();
}

// If the rcv buffer underflows, this gets called
void baseNet::getbufferr()
{
  fprintf(stderr,
	  "baseNet::getbufferr: receive buffer underflow, continuing!\n");
}

// This routine handles all the messages that don\'t have an object
// associated with them.
int baseNet::parent_receive( NetMsgType msg )
{
  int rcv_ok= 0;

  // First we try the methods in the handler list.
  NetMsgHandlerList *mthd= msg_handlers;
  while (mthd) { // list is null terminated
    if (mthd->msg == msg) { // Fire only first matching handler
      (*(mthd->handler))();
      return(rcv_ok);
    }
    else mthd= mthd->next;
  }

  // Failing that, we try some default methods.
  switch (msg) {
    
  case BASENET_REQBASENET:
    {
      RemObjInfo rem;
      netgetreminfo(&rem);
      baseNet *basenet= new baseNet(&rem);
    }
    break;
    
  case BASENET_REQSERVER:
  case BASENET_DELETED:
  case BASENET_ACK:
  case BASENET_NAK:
    fprintf(stderr,"baseNet::parent_receive: got misdirected <%s>; ignored!\n",
	    MsgInfo[msg].name);
    break;
    
  default: 
    fprintf(stderr,
	    "baseNet::netrcv: got and ignored unknown message, ID= %d\n",
	    msg);
  }
  return( rcv_ok );
}

// This routine adds a handler to the list of things that parent_receive
// tries on getting a message.
void baseNet::add_msg_handler( NetMsgType msg, void (*handler)() )
{
  NetMsgHandlerList *mthd= new NetMsgHandlerList;

  mthd->next= msg_handlers;
  msg_handlers= mthd;
  mthd->msg= msg;
  mthd->handler= handler;
}

// This actually puts a string length in ahead of the string
void baseNet::netputstring( const char *string )
{
  int length= strlen(string);
  netputnint(&length,1);
  if (pvm_pkstr((char *)string)) putbufferr();
}

// This returns the string in newly allocated memory;  the application
// is responsible for freeing it.
char *baseNet::netgetstring()
{
  int length= 0;
  char *string;
  netgetnint(&length,1);
  string= new char[length+1];
  if (pvm_upkstr(string)) getbufferr();
  return(string);
}

void baseNet::initialize( const char *name )
// This routine prepares the environment for network access
{
  if (!initialized) {
    self_info.tid= pvm_mytid();
    if (self_info.tid<0) {
      fprintf(stderr,"%s: Failed to enroll in PVM!\n",name);
      exit(-1);
    }

#ifdef never
    (void)pvm_setopt( PvmRoute, PvmRouteDirect );
#endif

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    extern int _MPP_MY_PE;
    if (_MPP_MY_PE) // not processor 0, which talks off the T3D
      (void)pvm_setopt( PvmRoute, PvmRouteDirect );
#endif

    net_name= new char[strlen(name) + 32];
    sprintf(net_name, "%s (tid %x)", name, self_info.tid);
    net_instance= pvm_joingroup( (char *)name );
#ifdef MPP_SUPPORT
    if (!strcmp(name,SMName) && (net_instance==SMInstance)) {
      char kludge_fname[64];
      sprintf(kludge_fname,"/tmp/vfleet_group_kludge.%d",(int)getuid());
      FILE *tid_file= fopen(kludge_fname,"w");
      if (!tid_file) {
	fprintf(stderr,
		"baseNet::initialize: cannot write kludge file!\n");
      pvm_exit();
      exit(-1);
      }
      fprintf(tid_file,"%d",self_info.tid);
      fclose(tid_file);
    }
#endif
    fprintf(stderr,"baseNet initialized; name and instance <%s> %d\n",
	    net_name, net_instance);

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    extern int _MPP_N_PES;
    partner_tid_table_size= _MPP_N_PES;
    partner_tid_table= new int[partner_tid_table_size];
    partner_tid_table_max= partner_tid_table_current= 0;
#endif

    initialized= 1;
    default_server= new RemObjInfo(SMName, SMInstance, 0);

    // Tell parent if any that we are ready
    (void)pvm_initsend(PvmDataDefault);
    int parent_tid;
    if ((parent_tid= pvm_parent()) != PvmNoParent) {
      fprintf(stderr,"baseNet::initialize: sending ready to tid %x\n",
	    parent_tid);
      (void)pvm_initsend(PvmDataDefault);
      pvm_send( parent_tid, BASENET_READY );
    }

  }
}

void baseNet::shutdown()
{
  pvm_exit(); // leave PVM
}

void baseNet::debug_on()
// Turn on debugging for self and partner
{
  debug= 1;
  bufsetup();
  if (!netsnd(BASENET_DEBUG_ON))
    fprintf(stderr,"baseNet::debug_on: error sending DEBUG_ON to partner.\n");
}

void baseNet::debug_off()
// Turn off debugging for self and partner
{
  debug= 0;
  bufsetup();
  if (!netsnd(BASENET_DEBUG_OFF))
    fprintf(stderr,
	    "baseNet::debug_off: error sending DEBUG_OFF to partner.\n");
}

void baseNet::netputreminfo()
// Put remote object info describing self
{
  RemObjInfo *rem_out= new RemObjInfo( self_info.tid, (long)this );

  netputreminfo(rem_out);
  delete rem_out;
}

void baseNet::netputreminfo( const RemObjInfo *rem_in )
{
  if (!rem_in) {
    fprintf(stderr,"baseNet::netputreminfo:  passed null info pointer!\n");
    return;
  }
  netputnint( &(rem_in->tid), 1 );
#ifdef never
  fprintf(stderr,"netputreminfo: putting %x_%x_%x_%x_%x_%x_%x_%x\n",
	  rem_in->bytes[0],rem_in->bytes[1],rem_in->bytes[2],rem_in->bytes[3],
	  rem_in->bytes[4],rem_in->bytes[5],rem_in->bytes[6],rem_in->bytes[7]);
#endif
  netputbytes( (char *)(rem_in->bytes), 8 );
}

void baseNet::netputremoteobj()
{
  netputreminfo( partner_rem() );
}

void baseNet::netgetreminfo( RemObjInfo *rem_in )
{
  if (!rem_in) {
    fprintf(stderr,"baseNet::netgetreminfo:  passed null info pointer!\n");
    return;
  }
  netgetnint(&(rem_in->tid), 1);
  netgetbytes((char *)(rem_in->bytes),8);
#ifdef never
  fprintf(stderr,"netgetreminfo: got %x_%x_%x_%x_%x_%x_%x_%x\n",
	  rem_in->bytes[0],rem_in->bytes[1],rem_in->bytes[2],rem_in->bytes[3],
	  rem_in->bytes[4],rem_in->bytes[5],rem_in->bytes[6],rem_in->bytes[7]);
#endif
}

baseServer::baseServer( const char *objectfile, char *arch )
// Fire up a server
{
  baseNet::break_ack_lock();

#ifdef MPP_SUPPORT
  if ( ( !strcmp(arch,"T3D") ) || ( !strcmp(arch,"T3E") ) ) {
    if (!mpp_server_table) spawn_mpp_servers( objectfile, arch );
    if (mpp_server_index >= n_mpp_procs-1) {
      fprintf(stderr,
	      "baseServer::baseServer: out of processes on %s; recycling!\n",
	      arch);
      mpp_server_index= -1;
    }
    mpp_server_index++;
    started= 1;
    server_info= mpp_server_table + mpp_server_index;
    return;
  }
#endif

  // If arch is NULL, PVM decides where to put the new process.  If
  // the first character of arch is upper case, it\'s a PVM architecture
  // type.  If it is lower case, it\'s a machine name.
  int arch_handling_flag;
  if (arch) {
    if (isupper(*arch)) arch_handling_flag= PvmTaskArch;
    else arch_handling_flag= PvmTaskHost;
  }
  else arch_handling_flag= PvmTaskDefault;

#ifdef never
  fprintf(stderr,"<%s><%s>\n",objectfile,arch);
  if (!strcmp(objectfile,"vrenserver")) {
    fprintf(stderr,"handler switch activated!\n");
    arch_handling_flag |= PvmTaskDebug;
  }
#endif

  int spawned_tid;
  if ( pvm_spawn( (char *)objectfile, 0, arch_handling_flag, arch, 
		  1, &spawned_tid ) < 0 || spawned_tid < 0) {
    fprintf(stderr,"baseServer::baseServer: unable to start up <%s>!\n",
	    objectfile);
    started= 0;
    server_info= NULL;
  }
  else {
    fprintf(stderr,"baseServer::baseServer: server <%s> is tid %x\n",
	    objectfile, spawned_tid);
    started= 1;
    server_info= new RemObjInfo( spawned_tid, 0 );
    fprintf(stderr,"waiting on spawned server...\n");
    int errorcode;
    if ( (errorcode= pvm_recv(spawned_tid, BASENET_READY)) < 0 )
      fprintf(stderr,"baseServer::baseServer: wait on <%s> returned %d!\n",
	      objectfile, errorcode);
    else fprintf(stderr," done waiting.\n");
  }
}

baseServer::baseServer()
// This is the version that gets started on the remote host
{
  server_info= NULL; // baseNet component of server will hold partner info
  started= 1;
  fprintf(stderr,"baseServer::baseServer: server_info= NULL\n");
}

baseServer::~baseServer()
// Shut down the server
{
  if ( pvm_kill(server_info->tid) < 0 ) {
    fprintf(stderr,"baseServer::~baseServer: trouble killing tid %x!\n",
	    server_info->tid);
  }
  delete server_info;
}

#ifdef MPP_SUPPORT
void baseServer::spawn_mpp_servers( const char *objectfile, char *arch )
{
  server_objectfile= new char[strlen(objectfile)+5];
  strcpy(server_objectfile, objectfile);
#ifdef CRAY_ARCH_C90
  // On the host of a Cray T3D, we need to distinguish between MPP 
  // vrenserver and a vrenserver that might run on the host.
  strcat(server_objectfile, "_t3d");
#endif
  server_arch= new char[strlen(arch)+1];
  strcpy(server_arch, arch);
  if (n_mpp_procs<1) {
    fprintf(stderr,
      "baseServer::spawn_mpp_procs: proc count unset or invalid, using 1!\n");
    n_mpp_procs= 1;
  }

  mpp_server_table= new RemObjInfo[n_mpp_procs];
  int *tid_buf= new int[n_mpp_procs];
  int retcode= pvm_spawn( server_objectfile, NULL, PvmTaskArch, "CRAY",
			 n_mpp_procs, tid_buf );
  if ( retcode<0 ) {
    fprintf(stderr,"baseServer::spawn_mpp_servers: spawn of <%s> failed!\n",
	    server_objectfile);
    abort();
  }
  else if (retcode<n_mpp_procs) {
    fprintf(stderr,
	    "baseServer::spawn_mpp_servers: only %d of %d spawns worked!\n",
	    retcode, n_mpp_procs);
    fprintf(stderr,"baseServer::spawn_mpp_servers: first error code %d\n",
	    tid_buf[retcode]);
    n_mpp_procs= retcode;
  }

  int which_slot= 0;
  for (int i=0; i<n_mpp_procs; i++) {
    fprintf(stderr,
	    "baseServer::spawn_mpp_servers: awaiting server <%s>, tid %x...\n",
	    server_objectfile, tid_buf[i]);
    int errorcode;
    if ( (errorcode= pvm_recv(tid_buf[i], BASENET_READY)) < 0 )
      fprintf(stderr,
	      "baseServer::spawn_mpp_servers: wait on <%s> returned %d!\n",
	      server_objectfile, errorcode);
    else {
      fprintf(stderr," done waiting.\n");
      mpp_server_table[which_slot++]= RemObjInfo( tid_buf[i], 0 );
    }
  }
  n_mpp_procs= which_slot;
}
#endif

int baseServer::server_running()
// Is it up and running?
{
  if (started) 
    return( pvm_pstat(server_info->tid) == PvmOk );
  else return( 0 );
}

void baseNet::break_ack_lock()
// Sometimes during transactions with servers it\'s necessary to send
// to someone else rather than replying with an ack.
{
  awaiting_ack= 0;
}

void baseNet::send_ack_and_break_lock()
// This can be used to send an ACK to a partner waiting for it,
// without exiting a method and thus allowing the normal mechanism
// to send the ack.  For example, the method may want to ACK and
// then proceed with other network transactions.
{
  if (awaiting_ack) {
    break_ack_lock();
    if (partner_same_host) (void)pvm_initsend(PvmDataRaw);
    else (void)pvm_initsend(PvmDataDefault);
    netputnint((int *)&last_ack_msg,1);
    int one= 1;
    netputnint(&one, 1); // second element of ack frame is success code
    (void)netsnd(BASENET_ACK);
  }
}

void baseNet::send_nak_and_break_lock()
// See comment for send_ack_and_break_lock
{
  if (awaiting_ack) {
    break_ack_lock();
    if (partner_same_host) (void)pvm_initsend(PvmDataRaw);
    else (void)pvm_initsend(PvmDataDefault);
    netputnint((int *)&last_ack_msg,1);
    int zero= 0;
    netputnint(&zero, 1); // second element of ack frame is success code
    (void)netsnd(BASENET_ACK);
  }
}

baseNet *baseNet::netgetlocalobj()
{
  RemObjInfo rem_in;
  netgetreminfo( &rem_in );

  if (rem_in.tid==self_info.tid) return( (baseNet *)(rem_in.object) );
  else {
    fprintf(stderr,
	    "baseNet::netgetlocalobj: object from tid %x, not this tid %x!\n",
	    rem_in.tid,self_info.tid);
    return( NULL );
  }
}

baseNet *baseNet::unpack_localobj( const char *input )
{
  int tid;
  int inbuf[8];
  RemObjInfo info;
  int nscanned= sscanf((char *)input,"%16x <%02x%02x%02x%02x%02x%02x%02x%02x>",
		       &tid, 
		       inbuf, inbuf+1, inbuf+2, inbuf+3,
		       inbuf+4, inbuf+5, inbuf+6, inbuf+7);
  for (int i=0; i<8; i++) info.bytes[i]= inbuf[i];
#ifdef never
  fprintf(stderr,
	  "unpack_localobj: scanned %d; tid %d, got %x_%x_%x_%x_%x_%x_%x_%x\n",
	  tid, nscanned,
	  info.bytes[0],info.bytes[1],info.bytes[2],info.bytes[3],
	  info.bytes[4],info.bytes[5],info.bytes[6],info.bytes[7]);
#endif
  if ( (nscanned != 9 ) || (tid != self_info.tid) )
    return NULL;
  return( (baseNet *)(info.object) );
}

char *baseNet::pack_netobj()
{
  char *string= new char[64];
#ifdef never
  fprintf(stderr,"pack_netobj: putting %x_%x_%x_%x_%x_%x_%x_%x\n",
	  rem->bytes[0],rem->bytes[1],rem->bytes[2],rem->bytes[3],
	  rem->bytes[4],rem->bytes[5],rem->bytes[6],rem->bytes[7]);
#endif
  sprintf(string,"%16x <%02x%02x%02x%02x%02x%02x%02x%02x>",
	  rem->tid, 
	  rem->bytes[0],rem->bytes[1],rem->bytes[2],rem->bytes[3],
	  rem->bytes[4],rem->bytes[5],rem->bytes[6],rem->bytes[7]);
  return(string);
}

// This routine is a duplicate of the baseNet class method of the same name
// This actually puts a string length in ahead of the string
void baseNetComm::netputstring( const char *string )
{
  int length= strlen(string);
  netputnint(&length,1);
  if (pvm_pkstr((char *)string)) putbufferr();
}

// This routine is a duplicate of the baseNet class method of the same name
// This returns the string in newly allocated memory;  the application
// is responsible for freeing it.
char *baseNetComm::netgetstring()
{
  int length= 0;
  char *string;
  netgetnint(&length,1);
  string= new char[length+1];
  if (pvm_upkstr(string)) getbufferr();
  return(string);
}

// This routine is a duplicate of the baseNet class method of the same name
// If the send buffer overflows, this gets called
void baseNetComm::putbufferr()
{
  fprintf(stderr,"baseNetComm::putbufferr: send buffer overflow, exiting!\n");
  abort();
}

// This routine is a duplicate of the baseNet class method of the same name
// If the rcv buffer underflows, this gets called
void baseNetComm::getbufferr()
{
  fprintf(stderr,
	  "baseNetComm::getbufferr: receive buffer underflow, continuing!\n");
}

