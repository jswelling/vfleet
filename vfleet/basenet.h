/****************************************************************************
 * basenet.h for pvm 2.4
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
#include "pvm3.h"
#include "net_messages.h"

/*
  Notes-
  -Component names (passed to initialize() routines) should be
  continuous strings (with no intervening characters) not containing
  the character '>'.
  -The message BASENET_NAK is not currently used;  rather an ACK message
  is sent with a failure code.  This is because of the lack of rcvmulti
  in PVM 3.1.
*/

// Struct to hold info about a message
static const int NetMsgNameLength= 64;
struct NetMsgInfo {
  char name[ NetMsgNameLength ];
  int ack_expected;
};

// Struct to hold message handler info
struct NetMsgHandlerList {
  NetMsgType msg;
  void (*handler)();
  NetMsgHandlerList *next;
};

class baseNet;

// Info to specify a remote object
class RemObjInfo {
public:
  int tid;
  union {
  unsigned long object;
  unsigned char bytes[8];
  };
  RemObjInfo& operator=( const RemObjInfo& );
  RemObjInfo();
  RemObjInfo( const int tid_in, const long obj );
  RemObjInfo( const char *comp, const int inst, const long obj );
  RemObjInfo(const RemObjInfo&);
  ~RemObjInfo();
  int operator==( const RemObjInfo& );
  int operator!=( const RemObjInfo& );
  static void set_server_tid( const int server_tid_in ) 
  { server_tid= server_tid_in; }
private:
  static int server_tid;
};

// The basenet object itself
class baseNet {
friend class baseServer;
protected:
  static RemObjInfo *default_server;
public:
  baseNet( RemObjInfo *rem_in );
  baseNet( NetMsgType msg, char *info_string= "",
	  RemObjInfo *server= baseNet::default_server );
  virtual ~baseNet();
  void debug_on();
  void debug_off();
  static void debug_static_on() 
  { debug_static= 1; (void)pvm_setopt( PvmDebugMask, 1 ); }
  static void debug_static_off() 
  { debug_static= 0; (void)pvm_setopt( PvmDebugMask, 0 ); }
  static void initialize( const char *name );
  static void shutdown();
  static void service();  // This routine goes in the main event loop
  static void service_blocking(); // as service but blocks 
  static NetMsgInfo MsgInfo[ BASENET_LASTMSG ];
  char *get_net_name() const { return( net_name ); }
  static int get_net_instance() { return( net_instance ); }
  int same_proc_as_partner( RemObjInfo& other ) const 
  { return( partner_rem()->tid == other.tid ); }
  int same_host_as_partner() const
    { return partner_same_host; }
  int same_proc_as_me( RemObjInfo& other ) const
  { return( self_info.tid == other.tid ); }
  static void add_msg_handler( NetMsgType msg, void (*handler)() );
  static void netgetreminfo( RemObjInfo *rem_in );
  static baseNet *netgetlocalobj();
  char *pack_netobj(); // Pack remote obj info for self- caller must free mem
  static baseNet *unpack_localobj( const char *input );
  static char *netgetstring();  // Memory is allocated;  the app must free it.
protected:
  static char *net_name;
  static int net_instance;
  static int initialized;
  RemObjInfo *partner_rem() const { return rem; }
  static int parent_receive( NetMsgType msg );
  static void netgetnint( int *x, int num ) 
    { if ( pvm_upkint( x, num, 1 ) ) getbufferr(); }
  static void netgetnfloat( float *x, int num ) 
    { if ( pvm_upkfloat( x, num, 1 ) ) getbufferr(); }
  static void netgetndfloat( double *x, int num ) 
    { if ( pvm_upkdouble(x, num, 1) ) getbufferr(); }
  static void netgetncplx( float *x, int num ) 
    { if ( pvm_upkcplx( x, num, 1 ) ) getbufferr(); }
  static void netgetndcplx( double *x, int num ) 
    { if ( pvm_upkdcplx( x, num, 1 ) ) getbufferr(); }
  static void netgetbytes( char *x, int num ) 
    { if ( pvm_upkbyte( x, num, 1 ) ) getbufferr(); }
  static void netgetnshort( short *np, int num )
    { if (pvm_upkshort( np, num, 1 ) ) getbufferr(); }
  static void netgetnlong( long *np, int num )
    { if (pvm_upklong( np, num, 1 ) ) getbufferr(); }
  void netputnint( const int *ptr, int num ) 
    { if ( pvm_pkint( (int *)ptr, num, 1 ) ) putbufferr(); }
  void netputnfloat( const float *ptr, int num ) 
    { if ( pvm_pkfloat( (float *)ptr, num, 1 ) ) putbufferr(); }
  void netputndfloat( const double *ptr, int num )
    { if ( pvm_pkdouble( (double *)ptr, num, 1) ) putbufferr(); }
  void netputncplx( const float *ptr, int num ) 
    { if ( pvm_pkcplx( (float *)ptr, num, 1 ) ) putbufferr(); }
  void netputndcplx( const double *ptr, int num ) 
    { if ( pvm_pkdcplx( (double *)ptr, num, 1 ) ) putbufferr(); }
  void netputbytes( const char *ptr, int num ) 
    { if ( pvm_pkbyte( (char *)ptr, num, 1 ) ) putbufferr(); }
  void netputnshort( const short *np, int num )
    { if ( pvm_pkshort( (short *)np, num, 1 ) ) putbufferr(); }
  void netputnlong( const long *np, int num )
    { if ( pvm_pklong( (long *)np, num, 1 ) ) putbufferr(); }
  void netputstring( const char *ptr );
  void netputreminfo();  // Put remote obj info describing self
  void netputreminfo( const RemObjInfo *rem_in );
  void netputremoteobj(); // Put remote obj info describing partner
  int netsnd(NetMsgType msg);
  virtual int netrcv(NetMsgType msg);
  virtual void add_ack_payload(NetMsgType msg, const int netrcv_retval);
  virtual void connect();
  virtual void bufsetup();
  static void break_ack_lock();
  void send_ack_and_break_lock();
  void send_nak_and_break_lock();
  int connected;
  int debug;
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  void listen_only_to_local_partners()
    { check_partners_only= 1; }
  void listen_to_everyone()
    { check_partners_only= 0; }
#endif
private:
  static void putbufferr();
  static void getbufferr();
  static int netsnd_ack_messages[2];
  static NetMsgHandlerList *msg_handlers;
  static RemObjInfo self_info;
  static int debug_static;
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  static int* partner_tid_table;
  static int partner_tid_table_size;
  static int partner_tid_table_max;
  static int partner_tid_table_current;
  static void add_partner_tid( const int tid );
  static int check_partners_only;
#endif
  RemObjInfo *rem;
  static int awaiting_ack;
  static NetMsgType last_ack_msg;
  int partner_same_host;
};

// This class provides the encoding and decoding abilities of baseNet,
// for objects that need to encode and decode themselves but don't
// need to themselves be distributed.
class baseNetComm {
protected:
  static void netgetnint( int *x, int num ) 
    { if ( pvm_upkint( x, num, 1 ) ) getbufferr(); }
  static void netgetnfloat( float *x, int num ) 
    { if ( pvm_upkfloat( x, num, 1 ) ) getbufferr(); }
  static void netgetndfloat( double *x, int num ) 
    { if ( pvm_upkdouble(x, num, 1) ) getbufferr(); }
  static void netgetncplx( float *x, int num ) 
    { if ( pvm_upkcplx( x, num, 1 ) ) getbufferr(); }
  static void netgetndcplx( double *x, int num ) 
    { if ( pvm_upkdcplx( x, num, 1 ) ) getbufferr(); }
  static void netgetbytes( char *x, int num ) 
    { if ( pvm_upkbyte( x, num, 1 ) ) getbufferr(); }
  static void netgetnshort( short *np, int num )
    { if (pvm_upkshort( np, num, 1 ) ) getbufferr(); }
  static void netgetnlong( long *np, int num )
    { if (pvm_upklong( np, num, 1 ) ) getbufferr(); }
  static void netputnint( const int *ptr, int num ) 
    { if ( pvm_pkint( (int *)ptr, num, 1 ) ) putbufferr(); }
  static void netputnfloat( const float *ptr, int num ) 
    { if ( pvm_pkfloat( (float *)ptr, num, 1 ) ) putbufferr(); }
  static void netputndfloat( const double *ptr, int num )
    { if ( pvm_pkdouble( (double *)ptr, num, 1) ) putbufferr(); }
  static void netputncplx( const float *ptr, int num ) 
    { if ( pvm_pkcplx( (float *)ptr, num, 1 ) ) putbufferr(); }
  static void netputndcplx( const double *ptr, int num ) 
    { if ( pvm_pkdcplx( (double *)ptr, num, 1 ) ) putbufferr(); }
  static void netputbytes( const char *ptr, int num ) 
    { if ( pvm_pkbyte( (char *)ptr, num, 1 ) ) putbufferr(); }
  static void netputnshort( const short *np, int num )
    { if ( pvm_pkshort( (short *)np, num, 1 ) ) putbufferr(); }
  static void netputnlong( const long *np, int num )
    { if ( pvm_pklong( (long *)np, num, 1 ) ) putbufferr(); }
  static void netputstring( const char *ptr );
  static char *netgetstring();  // Memory is allocated;  the app must free it.
private:
  static void putbufferr();
  static void getbufferr();
};
  
class baseServer {
public:
  baseServer( const char *object_file, char *arch= NULL );
  baseServer();
  ~baseServer();
  int server_running();
#ifdef MPP_SUPPORT
  static void set_mpp_proc_count( const int proc_count ) 
    { if (!n_mpp_procs) n_mpp_procs= proc_count; } // settable only once  
#endif
protected:
  RemObjInfo *server_info;
  int started;
#ifdef MPP_SUPPORT
  static int n_mpp_procs;
  static RemObjInfo* mpp_server_table;
  static int mpp_server_index;
  static char* server_objectfile;
  static char* server_arch;
  static void spawn_mpp_servers( const char *objectfile, char *arch );
#endif
};
