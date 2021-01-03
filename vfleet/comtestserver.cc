/****************************************************************************
 * comtestserver.cc
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include "logger.h"
#include "basenet.h"
#include "netcomtest.h"
#include "servman.h"

static void ComTestServerMainLoop()
{
  while (1) {
    baseNet::service_blocking();
  }
}

main( int argc, char *argv[] )
{
  fprintf(stdout,"comtestserver: argv[0]= <%s>\n",argv[0]);

  // Enroll in PVM
  netComTest::initialize( ComTestServerExeName );
  ComTestServer::initialize(argv[0]);

  ComTestServerMainLoop();  // This will loop forever

  // Leave PVM
  ComTestServer::shutdown();

}
