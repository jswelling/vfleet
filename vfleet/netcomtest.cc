/****************************************************************************
 * netcomtest.cc
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
#include "netcomtest.h"

int netComTest::initialized= 0;

int ComTestServer::initialized= 0;

baseLogger* netComTest::logger= NULL;

char *ComTestServerExeName="comtestserver";

netComTest::netComTest( RemObjInfo *rem_in )
: baseNet(rem_in)
{
  if (!initialized) initialize("NoNameGiven");

  logger= new netLogger( ComTestServerExeName );
}

netComTest::netComTest()
: baseNet(COMTEST_REQCOMTEST)
{
  if (!initialized) initialize("NoNameGiven");
  logger= NULL;
}

netComTest::~netComTest()
{
  delete logger;
}

int netComTest::netrcv(NetMsgType msg)
{
  int rcv_ok= 1;
  switch (msg) {

  case COMTEST_DOIT: 
    int val;
    netgetnint(&val,1);
    doit(val);
    break;
    
  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netComTest::doit(int val)
{
  if (connected) {
    if (logger) {
      char buf[64];
      sprintf(buf,"doit: %d!",val);
      logger->comment(buf);
    }
    else {
      bufsetup();
      netputnint(&val,1);
      netsnd(COMTEST_DOIT);
    }
  }
  else fprintf(stderr,"doit: not connected!\n");
}

void netComTest::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );

    initialized= 1;
  }
}

ComTestServer::ComTestServer( char *arch ) 
: baseServer(ComTestServerExeName, arch), 
  baseNet( BASENET_REQSERVER, "", server_info )
{
  // The baseServer constructor fires up the executable, and the
  // baseNet constructor fires a requst to it which causes us to
  // get connected.  Thus, we\'re done.
}

ComTestServer::ComTestServer( RemObjInfo *rem_in )
: baseNet(rem_in), baseServer()
{
}

ComTestServer::~ComTestServer()
{
}

int ComTestServer::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case COMTEST_REQCOMTEST: // request for new ComTest- purpose of server
    {
      RemObjInfo remote;
      baseNet::netgetreminfo(&remote);

      if ( remote != *partner_rem() ) {
	// Too many people to talk to to use ACK mechanism
	send_ack_and_break_lock();
      }
      netComTest *comtest= new netComTest(&remote);
    }
    break;
    
  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void ComTestServer::initialize(char *cmd)
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

void ComTestServer::handle_server_request()
// This routine catches the initial server request from the partner
{
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  ComTestServer *server= new ComTestServer(&rem);
}

void ComTestServer::create_comtest( RemObjInfo *rem_in, char *param_info )
// Create a partner for the netComTest at rem_in
{
  break_ack_lock(); // so we can send to server, which will ack to requestor
  bufsetup();
  netputreminfo(rem_in);
  netputstring(param_info);
  netsnd(COMTEST_REQCOMTEST);
}


