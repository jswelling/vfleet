#include <stdio.h>
#include <stdlib.h>
#include <sdsc.h>
#include "netv.h"
#include "pvm.h"
#include "net_messages.h"
#include "logger.h"
#include "servman.h"
#include "vren.h"

int netServiceManager::smcount = 0;

netServiceManager::netServiceManager( char *compname, char *inname ) 
: baseServiceManager( inname )
{
  if (!smcount) {
    smcount++;
    pvminstance= enroll( compname );
    if (pvminstance<0) {
      fprintf(stderr,"%s: Failed to enroll in PVM!\n", name);
      exit(2);
    }
    initsend();
    if (!status(SMName,SMInstance)) {
      fprintf(stderr,"%s: Service manager not running!\n",name);
      leave();
      exit(2);
    }
  }
  else {
    fprintf(stderr,
	    "%s: A net service manager already exists for this process\n", 
	    name);
    exit(2);
  }
}

netServiceManager::~netServiceManager()
{
  smcount--;
  leave();
}

baseLogger *netServiceManager::get_logger()
{
  return( new netLogger );
}

baseVRen *netServiceManager::get_vrenderer( const int xdim, const int ydim, const int zdim, 
					    const char *infostr, 
					    baseImageHandler *ihandler,
					    void (*error)(int error_id, 
							  baseVRen *renderer), 
					    void (*fatal)(int error_id, 
							  baseVRen *renderer) )
{
  snd(SMName, SMInstance, SMREQVREN);
  return( new netVRen( xdim, ydim, zdim, ihandler, error, fatal ) );
}

