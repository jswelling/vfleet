/****************************************************************************
 * composite.cc
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
//#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E || INTEL_LINUX )
#include <time.h>
#include <unistd.h>
#else /* ifdef CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E */

#ifdef DECCXX
extern "C" {
#endif

#ifdef SYSV_TIMING
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#include <sys/resource.h>
#ifdef DECCXX
}
#endif

#endif /* if CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E */

#include "geometry.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "logger.h"
#include "composite.h"


extern "C" {
  void exit( int retval );
#ifndef GNU_CPLUSPLUS
  time_t time( time_t *__tloc );
#if !(CRAY_ARCH_T3D || CRAY_ARCH_C90 || CRAY_ARCH_T3E )
  double difftime( time_t __time1, time_t __time0 );
#endif
#endif
}

Compositor::Compositor( const int nplanes_in, const int nihandlers_in,
			const gBoundBox& bbox_in,
			baseImageHandler *ihandler_in, 
			baseLogger *logger_in )
: bbox( bbox_in )
{
  nplanes= nplanes_in;
  nihandlers= nihandlers_in;
  logger= logger_in;
  output_ihandler= ihandler_in;
  ihandler_table= new compImageHandler*[nihandlers];
  plane= NULL;
}

Compositor::~Compositor()
{
  int i;

  for (i=0; i<nihandlers; i++) delete ihandler_table[i];
  delete [] ihandler_table;

  if (plane) {
    for (i=0; i<nplanes; i++) delete plane[i];
    delete plane;
  }
}

baseImageHandler *Compositor::get_ihandler( const int id )
{
  if (id >= nihandlers) {
    fprintf(stderr,"Compositor::get_ihandler: %d ihandlers, %d requested!\n",
	    nihandlers, id);
    exit(-1);
  }
	    
  return( ihandler_table[ id ] );
}

void Compositor::set_lookfrom( const gPoint& lookfm_in )
{
  lookfm= lookfm_in;
  dir= gVector( (0.5*(bbox.xmax() - bbox.xmin())) - lookfm.x(),
		(0.5*(bbox.ymax() - bbox.ymin())) - lookfm.y(),
		(0.5*(bbox.zmax() - bbox.zmin())) - lookfm.z() );
}

void Compositor::set_boundbox( const gBoundBox& bbox_in )
{
  bbox= bbox_in;
  gVector dir= gVector( (0.5*(bbox.xmax() - bbox.xmin())) - lookfm.x(),
			(0.5*(bbox.ymax() - bbox.ymin())) - lookfm.y(),
			(0.5*(bbox.zmax() - bbox.zmin())) - lookfm.z() );
}

void Compositor::note_start_time()
{
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  wc_long_start= rtclock();
  time_start= cpused();
#elif SYSV_TIMING
  wc_start= time(NULL);
  (void)times( &time_start );
#else
  wc_start= time(NULL);
  getrusage(RUSAGE_SELF,&start_rusage);
#endif
}

void Compositor::log_end_time()
{
    if (logger) {
      char out_string[128];
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
      long ticks_per_sec= sysconf(_SC_CLK_TCK);
      long wc_long_end= rtclock();
      long wc_sec= (wc_long_end-wc_long_start)/ticks_per_sec;
      long wc_usec= ((wc_long_end-wc_long_start)%ticks_per_sec)
	/(ticks_per_sec/1000000.0);
      if (wc_usec < 0) {
	wc_sec -= 1;
	wc_usec += 1000000;
      }
      long time_end= cpused();
      long u_sec= (time_end-time_start)/ticks_per_sec;
      long u_usec= ((time_end-time_start)%ticks_per_sec)/(ticks_per_sec/1000000.0);
      if (u_usec < 0) {
	u_sec -= 1;
	u_usec += 1000000;
      }
      sprintf(out_string,"Compositing done; %d.%06d CPU %d.%06d WC",
		    u_sec,u_usec,wc_sec,wc_usec);
#elif SYSV_TIMING
      double wc_sec= difftime( time(NULL), wc_start );
      struct tms time_end;
      (void)times( &time_end );
      long s_usec= (long)((1000000.0/CLK_TCK)
			  * (time_end.tms_stime - time_start.tms_stime));
      long s_sec= s_usec / 1000000;
      s_usec= s_usec % 1000000;
      long u_usec= (long)((1000000.0/CLK_TCK)
			  * (time_end.tms_utime - time_start.tms_utime));
      long u_sec= u_usec / 1000000;
      u_usec= u_usec % 1000000;
      if (s_usec < 0) {
	s_sec -= 1;
	s_usec += 1000000;
      }
      if (u_usec < 0) {
	u_sec -= 1;
	u_usec += 1000000;
      }
      sprintf(out_string,"Compositing done; %d.%06du %d.%06ds %5.3fWC",
		    u_sec,u_usec,s_sec,s_usec,wc_sec);
#else
      double wc_sec= difftime( time(NULL), wc_start );
      rusage end_rusage;
      getrusage(RUSAGE_SELF,&end_rusage);
      long s_usec= end_rusage.ru_stime.tv_usec - start_rusage.ru_stime.tv_usec;
      long s_sec= end_rusage.ru_stime.tv_sec - start_rusage.ru_stime.tv_sec;
      long u_usec= end_rusage.ru_utime.tv_usec - start_rusage.ru_utime.tv_usec;
      long u_sec= end_rusage.ru_utime.tv_sec - start_rusage.ru_utime.tv_sec;
      if (s_usec < 0) {
	s_sec -= 1;
	s_usec += 1000000;
      }
      if (u_usec < 0) {
	u_sec -= 1;
	u_usec += 1000000;
      }
      sprintf(out_string,"Compositing done; %d.%06du %d.%06ds %5.3fWC",
		    u_sec,u_usec,s_sec,s_usec,wc_sec);
#endif
      logger->comment(out_string);
    }
}

compImageHandler::compImageHandler( const int id_in, Compositor *creator_in )
: baseImageHandler()
{
  id= id_in;
  creator= creator_in;
}

compImageHandler::~compImageHandler()
{
  // Nothing to delete.
}

void compImageHandler::display( rgbImage *image, short refresh )
{
  creator->add_image( image, id );
}

