/****************************************************************************
 * logserver.cc
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
#include "basenet.h"
#include "logger.h"
#include "netlogger.h"
#include "servman.h"

static void LogServerMainLoop()
{
  while (1) {
    baseNet::service_blocking();
  }
}

main( int argc, char *argv[] )
{
  fprintf(stdout,"logserver: argv[0]= <%s>\n",argv[0]);

  // Enroll in PVM
  netLogger::initialize( LogServerExeName );
  logServer::initialize("/tmp/test.log",argv[0]);
  netLogger::debug_static_on();
  logServer::debug_static_on();

  LogServerMainLoop();  // This will loop forever

  // Leave PVM
  netLogger::shutdown();

}
