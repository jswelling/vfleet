/****************************************************************************
 * bcomposite.cc
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
//#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef SYSV_TIMING
#include <sys/times.h>
#endif

#include "geometry.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "logger.h"
#include "bcomposite.h"

extern "C" void exit( int retval );

bCompositor::bCompositor( const int split_plane_in, 
			  const gBoundBox& bbox_in,
			  baseImageHandler *ihandler_in, 
			  baseLogger *logger_in )
: Compositor( 0, 2, bbox_in, ihandler_in, logger_in )
{
  if ((split_plane_in<0) || (split_plane_in>2)) {
    fprintf(stderr,"bCompositor::bCompositor: invalid split plane %d!\n",
	    split_plane_in);
    exit(-1);
  }

  split_plane= split_plane_in;
  plane_0_in= plane_1_in= 0;

  ihandler_table[0]= new compImageHandler( 0, this );
  ihandler_table[1]= new compImageHandler( 1, this );

  calc_centers();
}

bCompositor::~bCompositor()
{
  // Nothing to delete; base class handles it all
}

void bCompositor::set_lookfrom( const gPoint& lookfm_in )
{
  Compositor::set_lookfrom( lookfm_in );
}

void bCompositor::set_boundbox( const gBoundBox& bbox_in )
{
  Compositor::set_boundbox( bbox_in );
  calc_centers();
}

void bCompositor::calc_centers()
{
  float xhalf= 0.5*(bbox.xmax() + bbox.xmin());
  float yhalf= 0.5*(bbox.ymax() + bbox.ymin());
  float zhalf= 0.5*(bbox.zmax() + bbox.zmin());

  switch (split_plane) {
  case 0: // constant x
    {
      center[0]= gPoint( 0.5*(bbox.xmin() + xhalf), yhalf, zhalf );
      center[1]= gPoint( 0.5*(bbox.xmax() + xhalf), yhalf, zhalf );
    }
    break;
  case 1: // constant y
    {
      center[0]= gPoint( xhalf, 0.5*(bbox.ymin() + yhalf), zhalf );
      center[1]= gPoint( xhalf, 0.5*(bbox.ymax() + yhalf), zhalf );
    }
    break;
  case 2: // constant z
    {
      center[0]= gPoint( xhalf, yhalf, 0.5*(bbox.zmin() + zhalf) );
      center[1]= gPoint( xhalf, yhalf, 0.5*(bbox.zmax() + zhalf) );
    }
    break;
  }
}

void bCompositor::add_image( rgbImage *image, const int id )
{
  if (id==0) {
    plane_0_in= 1;
    image_0= image;
  }
  else {
    plane_1_in= 1;
    image_1= image;
  }

  if (plane_0_in && plane_1_in) {
    // image_0 was delivered by the netVRen, and thus is the compressed
    // image.  image_1 is uncompressed.  So, we want to add image_0
    // to image_1, but whether it goes over or under depends on the
    // front-back ordering of the images.
    if ( (center[0]-lookfm).lengthsqr() > (center[1]-lookfm).lengthsqr() ) {
      // Image 1 is in front
      image_1->add_under( image_0 );
    }
    else {
      // Image 0 is in front
      image_1->add_over( image_0 );
    }
    output_ihandler->display( image_1 );
    log_end_time();
    plane_0_in= plane_1_in= 0;
  }
}
