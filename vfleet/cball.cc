/****************************************************************************
 * cball.cc
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
/*
This class provides a 'crystal ball' type 3D motion interface.
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "geometry.h"
#include "cball.h"

/* crystal ball radius (1.0 is full width of viewport) */
static const float CRYSTAL_BALL_RADIUS= 0.8;

CBall::CBall( void (*motion_callback_in)(gTransfm *, void *, CBall *),
	      void *cb_info_in )
{
  motion_callback= motion_callback_in; 
  cb_info= cb_info_in;
  viewmatrix= new gTransfm; // Generates identity transformation
}

CBall::~CBall()
{
  delete viewmatrix;
}

void CBall::mouse_delta( const MousePosition& mouse, float *dx, float *dy )
/* 
This routine calculates mouse coordinates as floating point relative
to the viewport center 
*/
{
  *dx= (float)( mouse.x - (mouse.maxx/2) )/(float)(mouse.maxx/2);
  *dy= (float)( mouse.y - (mouse.maxy/2) )/(float)(mouse.maxy/2);
}

gVector CBall::mouse_to_3d( const MousePosition& mouse )
/* This routine translates mouse coords to 3D crystal ball coords */
{
  float x, y, z, radius_2d, radius_3d= CRYSTAL_BALL_RADIUS;

  mouse_delta( mouse, &x, &y );
  radius_2d= sqrt( x*x + y*y );
  if (radius_2d > CRYSTAL_BALL_RADIUS) {
    x= x*CRYSTAL_BALL_RADIUS/radius_2d;
    y= y*CRYSTAL_BALL_RADIUS/radius_2d;
  }
  if ( radius_2d < CRYSTAL_BALL_RADIUS )  /* avoid round-off errors */
    z= sqrt( CRYSTAL_BALL_RADIUS*CRYSTAL_BALL_RADIUS-radius_2d*radius_2d );
  else z= 0.0;
  return( gVector( x, y, z ) );
}

void CBall::move( const MousePosition& mousedown, 
		  const MousePosition& mouseup )
/* 
This routine calculates a transformation matrix appropriate for a crystal
ball rotation as specified by the given mouse motion, and passes it
to the output callback.
*/
{
  /* Create the inverse viewing transformation */
  gTransfm inv_view= *viewmatrix;
  inv_view.transpose_self();

  /* Convert to 3D coords on crystal ball surface */
  gVector v1= mouse_to_3d( mousedown );
  gVector v2= mouse_to_3d( mouseup );

  /* Align vectors to world coordinates */
  gVector rv1= inv_view * v1;
  gVector rv2= inv_view * v2;

  /* Create transformation which aligns original radius vec with final one */
  gTransfm *result= rv1.aligning_rotation( rv2 );

  /* Pass the result and related info to the callback */
  (*motion_callback)(result, cb_info, this);

  /* Clean up */
  delete result;
}

