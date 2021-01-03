#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

static char *exename;

main( int argc, char *argv[] )
{
  baseLogger *logger;

  fprintf(stderr,"Starting...\n");

  exename= argv[0];

  logger= new fileLogger("/usr/tmp/loggertester.log",exename);
  logger->comment("This is really pretty trivial");
  delete logger;

  fprintf(stderr,"done.\n");
}
