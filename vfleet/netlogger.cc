/****************************************************************************
 * netlogger.cc
 * Author Joel Welling
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include "basenet.h"
#include "logger.h"
#include "netlogger.h"

int netLogger::initialized= 0;

baseLogger *netLogger::default_real_logger= 
    new transpstdoutLogger("NoNameGiven", 1);

int logServer::initialized= 0;

char *LogServerExeName="logserver";

baseLogger *logServer::outlogger= new baseLogger("NoName");
baseLogger *(*logServer::outlogger_creation_cb)()= NULL;

void netLogger::handle_netlogger_request()
// This is the default handler for requests for new netLoggers.
{
  // Handle incoming request for a logger here
  RemObjInfo rem;
  netgetreminfo(&rem);
  baseLogger *logger= new netLogger(default_real_logger, &rem);
}

netLogger::netLogger( baseLogger *logger, RemObjInfo *rem_in ) 
: baseNet(rem_in), baseLogger("*remote_request*")
{
  if (!initialized) initialize("NoNameGiven");

  outlogger= logger;

  open_logger();// This will do nothing, because the remote end has no ologger
}

netLogger::netLogger( char *cmd )
: baseNet(LOGGER_REQLOGGER), baseLogger(cmd)
{
  if (!initialized) initialize("NoNameGiven");
  outlogger= NULL;

  // The baseNet constructor has given us a remote partner, and because
  // we have not returned to the service routine that is the most
  // recent message transaction.  We now pass the remote partner
  // a bit of name info.  This transaction also expects an ACK.
  bufsetup();
  netputstring(user);
  netputstring(procname);
  if (!netsnd(LOGGER_MATCHNAMES)) 
    fprintf(stderr,"netLogger::netLogger: unable to exchange names!\n");

  // The partner is now ready for our open call.
  open_logger();
}

netLogger::~netLogger()
{
  close_logger();  // cause closure message to get written at the remote end
}

int netLogger::netrcv(NetMsgType msg)
{
  int rcv_ok= 1;
  switch (msg) {

  case LOGGER_MATCHNAMES: // adopt the remote partner\'s name and proc info
    delete user;
    user= netgetstring();
    delete procname;
    procname= netgetstring();
    break;
    
  case LOGGER_MESSAGE: // output a message for the remote partner
    if (outlogger) {
      char *string= netgetstring();
      char *ostring= new char[ strlen(user) + strlen(procname) 
			       + strlen(string) + 32 ];
      sprintf(ostring,"<%s> <%s> %d: %s", user, procname, id, string);
      outlogger->comment(ostring);
      delete string;
      delete ostring;
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netLogger::write_string(char *string)
{
  if (connected) {
    bufsetup();
    netputstring(string);
    netsnd( LOGGER_MESSAGE );
  }
}

void netLogger::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );

    // Add our message handler
    add_msg_handler( LOGGER_REQLOGGER, 
		     (void (*)())handle_netlogger_request );

    delete default_real_logger;
    default_real_logger= new transpstdoutLogger(name,1);

    initialized= 1;
  }
}

logServer::logServer( char *arch ) 
: baseServer(LogServerExeName, arch), 
  baseNet( BASENET_REQSERVER, "", server_info )
{
  // The baseServer constructor fires up the executable, and the
  // baseNet constructor fires a requst to it which causes us to
  // get connected.  Thus, we\'re done.
}

logServer::logServer( RemObjInfo *rem_in )
: baseNet(rem_in), baseServer()
{
}

logServer::~logServer()
{
  delete outlogger;
}

int logServer::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case LOGGER_REQLOGGER: // request for new logger- purpose of server
    {
      RemObjInfo remote;
      baseNet::netgetreminfo(&remote);

      if ( remote != *partner_rem() ) {
	// Too many people to talk to to use ACK mechanism
	send_ack_and_break_lock();
      }
      baseLogger *logger;
      if (outlogger_creation_cb)
	logger= new netLogger( (*outlogger_creation_cb)(), &remote );
      else
	logger= new netLogger(outlogger, &remote);
    }
    break;
    
  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void logServer::initialize(char *cmd)
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( cmd );

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void logServer::initialize( char *logfile, char *cmd )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( cmd );

    // Create a logger to which everything gets routed
    delete outlogger;
    outlogger= new transparentLogger(logfile, cmd);

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void logServer::initialize( baseLogger *outlogger_in, char *cmd )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( cmd );

    // Aim output at the given logger.
    delete outlogger;
    outlogger= outlogger_in;

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void logServer::initialize( baseLogger *(*outlogger_create)(), char *cmd )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( cmd );

    // Aim output at the given logger.
    delete outlogger;
    outlogger= NULL;
    outlogger_creation_cb= outlogger_create;

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void logServer::handle_server_request()
// This routine catches the initial server request from the partner
{
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  logServer *logserver= new logServer(&rem);
}

void logServer::create_logger( RemObjInfo *rem_in, char *param_info )
// Create a partner for the netLogger at rem_in
{
  break_ack_lock(); // so we can send to server, which will ack to requestor
  bufsetup();
  netputreminfo(rem_in);
  netputstring(param_info);
  netsnd(LOGGER_REQLOGGER);
}


