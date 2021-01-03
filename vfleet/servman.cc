/****************************************************************************
 * servman.cc
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
#include "basenet.h"
#include "vren.h"
#include "netvren.h"
#include "logger.h"
#include "netlogger.h"
#include "netcomtest.h"
#include "servman.h"

static char *progname;

static char **logserver_hosts= NULL;

static char **vrenserver_cast_hosts= NULL;

static char **vrenserver_comp_hosts= NULL;

static int logserver_host_id= 0;
static int vrenserver_cast_host_id= 0;
static int vrenserver_comp_host_id= 0;

static logServer *logserver= NULL;

static const int initial_hostfile_buflen= 32;
static const int hostname_maxlen= 128;

#ifdef MPP_SUPPORT
static int mpp_processor_count= 0;
#endif

static char** load_hostfile( char *fname )
{
  int buflen= initial_hostfile_buflen;

  char **result= new char*[buflen];

  FILE *infile= fopen(fname,"r");
  if (!infile) {
    fprintf(stderr,"%s: unable to open file <%s> for reading!\n",
	    progname,fname);
    exit(2);
  }

  int buf_index= 0;
  while (!feof(infile)) {

    char *host= new char[hostname_maxlen];
    result[buf_index++]= fgets(host, hostname_maxlen, infile);

    // Take care of trailing newlines
    if (result[buf_index-1]) {
      char *runner= result[buf_index-1];
      while (*runner) runner++;
      if ((runner>result[buf_index-1]) && (*(runner-1) == '\n')) 
	*(runner-1)= '\0';
      if (*(result[buf_index-1])==0) {
	delete result[buf_index-1];
	buf_index--;
      }
    }

    if (buf_index >= buflen) {
      // reallocate the hostname buffer
      char **new_result= new char*[2*buflen];
      for (int i=0; i<buf_index; i++) new_result[i]= result[i];
      delete [] result;
      result= new_result;
      buflen= 2*buflen;
    }
  }

  if (fclose(infile) == EOF) {
    fprintf(stderr,"%s: error closing file <%s>!\n",progname,fname);
  }

  result[buf_index]= NULL;
  return( result );
}

static ComTestServer* spawn_comtestserver()
{
  ComTestServer* result= new ComTestServer();
  return result;
}

static logServer *spawn_logserver()
{
  logServer *result= new logServer( logserver_hosts[ logserver_host_id ] );
  if (logserver_hosts[logserver_host_id])
    logserver_host_id++;
  else logserver_host_id= 0;
  return result;
}

static VRenServer *spawn_cast_vrenserver()
{
  VRenServer *result= 
    new VRenServer( vrenserver_cast_hosts[ vrenserver_cast_host_id ] );
  if (vrenserver_cast_hosts[vrenserver_cast_host_id])
    vrenserver_cast_host_id++;
  else vrenserver_cast_host_id= 0;
  return result;
}

static VRenServer *spawn_comp_vrenserver()
{
  VRenServer *result= 
    new VRenServer( vrenserver_comp_hosts[ vrenserver_comp_host_id ] );
  if (vrenserver_comp_hosts[vrenserver_comp_host_id])
    vrenserver_comp_host_id++;
  else vrenserver_comp_host_id= 0;
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
  baseNet::netgetreminfo(&rem);
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
  VRenServer *new_server;
#ifdef never
  if ((type==0) || (type==1)) new_server= spawn_cast_vrenserver();
  else new_server= spawn_comp_vrenserver();
#endif
  new_server= spawn_cast_vrenserver();
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

  progname= argv[0];

  // Parse operands
  extern char *optarg;
  extern int optind; 
  int c;
  int errflg= 0;
  while ((c= getopt(argc, argv, "r:c:l:")) != -1)
    switch (c) {
    case 'r': // Load raycaster host list
      vrenserver_cast_hosts= load_hostfile(optarg);
      break;
    case 'c': // Load compositor host list
      vrenserver_comp_hosts= load_hostfile(optarg);
      break;
    case 'l': // Load logserver host name or arch
      logserver_hosts= new char*[2];
      logserver_hosts[0]= optarg;
      logserver_hosts[1]= NULL;
      break;
    case '?': 
      errflg++;
      break;
    default: 
      errflg++;
      break;
    }
  if (errflg) {
    fprintf(stderr,
       "usage: %s [-r raycasterfile] [-c compositorfile] [-l loggerhost]\n",
	    progname);
    exit(2);
  }

  // Make up tables for any not included
  if (!logserver_hosts) {
    logserver_hosts= new char*[1];
    *logserver_hosts= NULL;
  }
  if (!vrenserver_cast_hosts) {
    vrenserver_cast_hosts= new char*[1];
    *vrenserver_cast_hosts= NULL;
  }
  if (!vrenserver_comp_hosts) {
    vrenserver_comp_hosts= new char*[1];
    *vrenserver_comp_hosts= NULL;
  }

  int i=0; 
  fprintf(stderr,"logserver_hosts:\n");
  while (logserver_hosts[i]) fprintf(stderr,"<%s>\n",logserver_hosts[i++]);
  fprintf(stderr,"vrenserver_cast_hosts:\n");
  i= 0;
  while (vrenserver_cast_hosts[i]) {
#ifdef MPP_SUPPORT
    if ((!strcmp(vrenserver_cast_hosts[i],"T3D"))
	|| (!strcmp(vrenserver_cast_hosts[i],"T3E"))) mpp_processor_count++;
#endif
    fprintf(stderr,"<%s>\n",vrenserver_cast_hosts[i++]);
  }
  fprintf(stderr,"vrenserver_comp_hosts:\n");
  i= 0;
  while (vrenserver_comp_hosts[i]) fprintf(stderr,"<%s>\n",vrenserver_comp_hosts[i++]);
  fprintf(stderr,"done with lists\n");

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

#ifdef MPP_SUPPORT
  if (mpp_processor_count) baseServer::set_mpp_proc_count(mpp_processor_count);
#endif

  ServiceManagerMainLoop();  // This will loop forever

  baseNet::shutdown();

  char* runner= *logserver_hosts;
  while (*runner) delete runner++;
  delete [] logserver_hosts;

  runner= *vrenserver_cast_hosts;
  while (*runner) delete runner++;
  delete [] vrenserver_cast_hosts;

  runner= *vrenserver_comp_hosts;
  while (*runner) delete runner++;
  delete [] vrenserver_comp_hosts;

  fprintf(stderr,"done.\n");
}


