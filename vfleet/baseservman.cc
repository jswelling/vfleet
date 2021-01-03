#include <stdio.h>
#include <stdlib.h>
#include <sdsc.h>
#include "netv.h"
#include "servman.h"

baseServiceManager::baseServiceManager( char *inname ) 
{
  name= new char[strlen(inname)+1];
  strcpy(name, inname);
}

baseServiceManager::~baseServiceManager()
{
  delete name;
}

baseLogger *baseServiceManager::get_logger()
{
  fprintf(stderr,"%s: base get_logger called!\n",name);
  return( NULL );
}

baseVRen *baseServiceManager::get_vrenderer( const int xdim, const int ydim, 
					     const int zdim, 
					     const char *infostr,
					     baseImageHandler *ihandler,
					     void (*error)
					      (int id, baseVRen *ren),
					     void (*fatal)
					      (int id, baseVRen *ren))
{
  fprintf(stderr,"%s: base get_vrenderer called!\n",name);
  return( NULL );
}
