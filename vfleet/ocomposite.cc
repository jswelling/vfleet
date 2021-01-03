/****************************************************************************
 * ocomposite.cc
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

#include "geometry.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "logger.h"
#include "ocomposite.h"

extern "C" void exit( int retval );

oCompositor::oCompositor( const int xdim_in, const int ydim_in, 
			const int zdim_in, const gBoundBox& bbox_in,
			baseImageHandler *ihandler_in, 
			baseLogger *logger_in )
: Compositor( 4, 8, bbox_in, ihandler_in, logger_in )
{
  xdim= xdim_in;
  ydim= ydim_in;
  zdim= zdim_in;

  if ((xdim != 2) || (ydim != 2) || (zdim != 2)) {
    fprintf(stderr,
	    "oCompositor::oCompositor: %d %d %d are not valid dimensions!\n",
	    xdim, ydim, zdim );
    exit(-1);
  }

  // The images are stored in this array in Morton index order
  for (int i=0; i<8; i++) 
    ihandler_table[i]= new compImageHandler( i, this );

  octs_in= 0;
  entry_octant= 0;
}

oCompositor::~oCompositor()
{
  // Nothing to delete; base class handles it all
}

void oCompositor::set_lookfrom( const gPoint& lookfm_in )
{
  Compositor::set_lookfrom( lookfm_in );
  entry_octant= 
    ( (dir.x() < 0) ? 1 : 0 )
    | ( (dir.y() < 0) ? 2 : 0 )
    | ( (dir.z() < 0) ? 4 : 0 );
}

void oCompositor::set_boundbox( const gBoundBox& bbox_in )
{
  Compositor::set_boundbox( bbox_in );
  entry_octant= 
    ( (dir.x() < 0) ? 1 : 0 )
    | ( (dir.y() < 0) ? 2 : 0 )
    | ( (dir.z() < 0) ? 4 : 0 );
}

void oCompositor::add_image( rgbImage *image, const int id )
{
  // Check entry conditions
  if (!octs_in) { // This is first input image
    int loop;

    // Check for size match
    if ( (!plane) 
	 || (plane[0]->xsize() != image->xsize())
	 || (plane[0]->ysize() != image->ysize()) ) {
      
      // Need to allocate new compositing planes
      if (plane) for (loop=0; loop<4; loop++) delete plane[loop];
      else plane= new rgbImage*[4];
      
      for (loop=0; loop<4; loop++) 
	plane[loop]= new rgbImage(image->xsize(), image->ysize());
    }

    // Clear the planes
    for (loop=0; loop<4; loop++) plane[loop]->clear();
  }
  else { // Is this image of the right size?
    if ( (image->xsize() != plane[0]->xsize())
	 || (image->ysize() != plane[0]->ysize()) ) {
      fprintf(stderr,"oCompositor::add_image: mismatched input image!\n");
      exit(-1);
    }
  }

  // Calculate absolute and view-direction-relative Morton indices
  unsigned char index= id;
  unsigned char relative_index= index ^ entry_octant;
  octs_in= octs_in | (1<<relative_index);

  if (logger) {
    char msgbuf[64];
    sprintf(msgbuf,"Have planes %d",(int)octs_in);
    logger->comment(msgbuf);
  }

  // Matte in the new block.  Plane 0 is the closest plane.
  switch (relative_index) {
  case 0:
    plane[0]->add_under(image);
    break;
  case 1:
  case 2:
  case 4:
    plane[1]->add_under(image);
    break;
  case 3:
  case 5:
  case 6:
    plane[2]->add_under(image);
    break;
  case 7:
    plane[3]->add_under(image);
    break;
  default:
    fprintf(stderr,"oCompositor::add_image: algorithm error!\n");
    exit(-1);
  }

  // Check for final merges.  Bit patterns are:
  // Plane 0:   1
  // Plane 1:  22
  // Plane 2: 104
  // Plane 3: 128

  if (((octs_in & (104 | 128)) == (104 | 128))
      && ( (1<<relative_index) & (104 | 128) )) { // Planes 2 and 3 in
    plane[2]->add_under(plane[3]);
  }

  if (((octs_in & (22 | 104 | 128)) == (22 | 104 | 128)) 
    && ( (1<<relative_index) & (22 | 104 | 128) )) { // Planes 1, 2 and 3 in
    plane[1]->add_under(plane[2]);
  }

  if (octs_in == 255) { // All planes in
    plane[0]->add_under(plane[1]);
  }

  // Display the image and reset
  if (octs_in==255) { // Just added last image
    output_ihandler->display( plane[0] );
    octs_in= 0;
    log_end_time();
  }
}
