/****************************************************************************
 * rayimage.cc
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
#include <strings.h>
#include <math.h>
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "raycastvren.h"

rgbImage *RayImage::create_rgbimage()
{
  int i, j;

  rgbImage *result= new rgbImage( xsize(), ysize() );

  // rgbImages have the 0,0 pixel at top left, while rayImages have it
  // at bottom left.  Thus we must flip the image vertically.
  for (j=0; j<ysize(); j++) {
    result->setpix( 0, ysize()-(j+1), pix(0,j).clr );
    for (i=1; i<xsize(); i++)
      result->setnextpix( pix(i,j).clr );
  }

  return result;
}

void RayImage::clear()
{
  gBColor clear_black;
  QualityMeasure perfect_qual;

  for (rayPixel* p=data; p < data+(xdim*ydim); p++) {
    p->clr= clear_black;
    p->qual= perfect_qual;
  }
}
