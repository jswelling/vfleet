/****************************************************************************
 * smtester.cc
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

#ifdef __GNUC__
#include <unistd.h>
#endif

#include <string.h>
#include <sdsc.h>
#include "netv.h"
#include "basenet.h"
#include "vren.h"
#include "netvren.h"
#include "logger.h"
#include "netlogger.h"
#include "netcomtest.h"
#include "servman.h"

static char *logserver_hosts[]= { 
  "SGI", 
  NULL };

static char *vrenserver_cast_hosts[]= {
  "PMAX",
  "PMAX",
  "PMAX",
  "PMAX",
  "PMAX",
  "PMAX",
  "PMAX",
  "PMAX",
  NULL };

static char *vrenserver_comp_hosts[]= {
  "PMAX",
  NULL };

static int logserver_host_id= 0;
static int vrenserver_cast_host_id= 0;
static int vrenserver_comp_host_id= 0;

static logServer *logserver= NULL;

static ComTestServer* spawn_comtestserver()
{
  ComTestServer* result= new ComTestServer();
  return result;
}

static logServer *spawn_logserver()
{
  logServer *result= new logServer( logserver_hosts[ logserver_host_id++ ] );
  if (!logserver_hosts[logserver_host_id]) logserver_host_id= 0;
  return result;
}

static VRenServer *spawn_cast_vrenserver()
{
  VRenServer *result= 
    new VRenServer( vrenserver_cast_hosts[ vrenserver_cast_host_id++ ] );
  if (!vrenserver_cast_hosts[vrenserver_cast_host_id]) 
    vrenserver_cast_host_id= 0;
  return result;
}

static VRenServer *spawn_comp_vrenserver()
{
  VRenServer *result= 
    new VRenServer( vrenserver_comp_hosts[ vrenserver_comp_host_id++ ] );
  if (!vrenserver_comp_hosts[vrenserver_comp_host_id]) 
    vrenserver_comp_host_id= 0;
  return result;
}

static void handle_netcomtest_request()
// This is the handler for requests for new netComTests
{
  // Spawn a new server and pass the request off to it
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  fprintf(stderr,"Got a netComTest request from tid %x, object %d\n",
	  rem.tid, rem.object);
  ComTestServer *new_server= spawn_comtestserver();
  new_server->create_comtest(&rem);
}

static void handle_netlogger_request()
// This is the handler for requests for new netLoggers.
{
  // Pass the request off to the logServer
  RemObjInfo rem;
  fprintf(stderr,"in baseNet::handle_netlogger_request\n");
  baseNet::netgetreminfo(&rem);
  fprintf(stderr,"Got a netLogger request from tid %x, object %d\n",
	  rem.tid, rem.object);
  logserver->create_logger(&rem);
}

static void handle_netvren_request()
// This is the handler for requests for new netvrens.
{
  // Spawn a new server and create the VRen on it.
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  char *param= baseNet::netgetstring();
  int type;
  sscanf(param, "%d", &type);
  fprintf(stderr,"*****got vrenserver request, type %d\n",type);
  VRenServer *new_server;
  if (type==0) new_server= spawn_cast_vrenserver();
  else new_server= spawn_comp_vrenserver();
  // new_server->debug_static_on();
  new_server->create_vren(&rem, param);
  delete param;
}

static void ServiceManagerMainLoop()
{
  while (1) {
    baseNet::service_blocking();
  }
}

main( int argc, char *argv[] )
{
  fprintf(stderr,"Starting...\n");

  // Enroll in PVM
  netLogger::initialize( SMName );
  if (baseNet::get_net_instance() != SMInstance) {
    fprintf(stderr,"%s: A service manager is already running!\n",
	    argv[0]);
    netLogger::shutdown();
    exit(2);
  }
  // netLogger::debug_static_on();
  logServer::initialize(argv[0]);
  netVRen::initialize( argv[0] );
  // netVRen::debug_static_on();
  VRenServer::initialize( argv[0] );

  // Create a logging server
  if (!logserver) logserver= spawn_logserver();

  // Override our creation handlers with versions
  // that redirect to the server
  netLogger::add_msg_handler( LOGGER_REQLOGGER, 
		   handle_netlogger_request );
  netVRen::add_msg_handler( VREN_REQVREN, handle_netvren_request );
  netComTest::add_msg_handler( COMTEST_REQCOMTEST, handle_netcomtest_request );

  ServiceManagerMainLoop();  // This will loop forever

  baseNet::shutdown();
  fprintf(stderr,"done.\n");
}


