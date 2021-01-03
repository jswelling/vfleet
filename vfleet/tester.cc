#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "basenet.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "logger.h"
#include "netlogger.h"
#include "servman.h"

#ifdef DECCXX
extern "C" unsigned int sleep( unsigned int seconds );
#endif

static void error_handler( int error_id, baseVRen *renderer )
{
  fprintf(stderr,"Just got error %d from a renderer\n", error_id);
}

static void fatal_handler( int error_id, baseVRen *renderer )
{
  fprintf(stderr,"Just got fatal error %d from a renderer; exiting!\n",
	  error_id);
  exit(2);
}

static void MainLoop()
{
  for (int i=0; i<10; i++) {
    baseNet::service();
    fprintf(stderr,"tick: i= %d\n",i);
    sleep(1);
  }
}

main( int argc, char *argv[] )
{
  int my_instance; 
  netLogger *logger;
  baseNet *basenet;

  fprintf(stderr,"Starting...\n");

  netLogger::initialize( argv[0] );
  fprintf(stderr,"point 1\n");
  baseNet::initialize( argv[0] );
  fprintf(stderr,"point 2\n");

//  baseNet::debug_static_on();
  fprintf(stderr,"point 3\n");
  basenet= new baseNet(BASENET_REQBASENET);
  fprintf(stderr,"point 4\n");
//  basenet->debug_on();
  fprintf(stderr,"point 5\n");
  delete basenet;
  fprintf(stderr,"point 6\n");

  // Generate a network object
  logger= new netLogger( argv[0] );
//  logger->debug_on();

  MainLoop();

  logger->comment("About to get deleted.");

  fprintf(stderr,"deleting...\n");
  delete logger;

  // Leave PVM
  fprintf(stderr,"Leaving...\n");
  netLogger::shutdown();

  fprintf(stderr,"done.\n");
}
