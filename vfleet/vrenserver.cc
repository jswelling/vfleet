/****************************************************************************
 * vrenserver.cc
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
#include <string.h>
#include "basenet.h"
#include "vren.h"
#include "netvren.h"
#include "logger.h"
#include "netlogger.h"
#include "servman.h"

static void VRenServerMainLoop()
{
  while (1) {
    baseNet::service_blocking();
  }
}

main( int argc, char *argv[] )
{
  // Enroll in PVM
  netVRen::initialize( VRenServerExeName );
  VRenServer::initialize( VRenServerExeName );
  netLogger::initialize( argv[0] );
  // netLogger::debug_static_on();
  // netVRen::debug_static_on();

  VRenServerMainLoop();  // This will loop forever

  // Leave PVM
  baseNet::shutdown();
}
