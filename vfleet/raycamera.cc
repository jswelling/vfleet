/****************************************************************************
 * camera.cc
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
#include <math.h>
#include "rgbimage.h"
#include "vren.h"
#include "raycastvren.h"

rayCamera::rayCamera( const gPoint lookfm_in, const gPoint lookat_in, 
		      const gVector up_in, const float fovea_in, 
		      const float hither_in, const float yon_in,
		      const int parallel_flag_in ) 
: Camera( lookfm_in, lookat_in, up_in, fovea_in, hither_in, yon_in,
	  parallel_flag_in )
{
  gVector viewcross= lookdir ^ up;
  if ( viewcross.length() == 0.0 ) {
    fprintf(stderr,
	    "Camera::Camera: up parallel to look direction, will use Z!\n");
    up= gVector( 0.0, 0.0, 1.0 );
    viewcross= lookdir ^ up;
    if ( viewcross.length() == 0.0 ) {
      fprintf(stderr,
	      "Camera::Camera: still parallel, will use Y!\n");
      up= gVector( 0.0, 1.0, 0.0 );
    }  
  }
  screeni= lookdir ^ up;
  screeni.normalize();
  screenj= screeni ^ lookdir;
  screenj.normalize();
  image_xdim= image_ydim= 0;
  ray_divergence= 0.0;
  par_ray_sep= 0.0;
  // firstray and par_firstpoint get initialized to nulls, 
  // reset at first initialize_ray
}

rayCamera::rayCamera( const rayCamera& other ) 
: Camera( other )
{
  firstray= other.firstray;
  par_firstpoint= other.par_firstpoint;
  screenx= other.screenx;
  screeny= other.screeny;
  screeni= other.screeni;
  screenj= other.screenj;
  image_xdim= other.image_xdim;
  image_ydim= other.image_ydim;
  ray_divergence= other.ray_divergence;
  par_ray_sep= other.par_ray_sep;
}

rayCamera::rayCamera( const Camera& other )
: Camera( other )
{
  gVector viewcross= lookdir ^ up;
  if ( viewcross.length() == 0.0 ) {
    fprintf(stderr,
	    "Camera::Camera: up parallel to look direction, will use Z!\n");
    up= gVector( 0.0, 0.0, 1.0 );
    viewcross= lookdir ^ up;
    if ( viewcross.length() == 0.0 ) {
      fprintf(stderr,
	      "Camera::Camera: still parallel, will use Y!\n");
      up= gVector( 0.0, 1.0, 0.0 );
    }  
  }
  screeni= lookdir ^ up;
  screeni.normalize();
  screenj= screeni ^ lookdir;
  screenj.normalize();
  image_xdim= 0;
  image_ydim= 0;
  ray_divergence= 0.0;
  par_ray_sep= 0.0;
  // firstray and par_firstpoint gets initialized to nulls, 
  // reset at first initialize_ray
}

rayCamera::~rayCamera()
{
  // Nothing to delete
}

rayCamera& rayCamera::operator=( const rayCamera& other )
{
  if (this != &other) {
    *this= (Camera&)other;
    firstray= other.firstray;
    par_firstpoint= other.par_firstpoint;
    screenx= other.screenx;
    screeny= other.screeny;
    screeni= other.screeni;
    screenj= other.screenj;
    image_xdim= other.image_xdim;
    image_ydim= other.image_ydim;
    ray_divergence= 0.0;
    par_ray_sep= other.par_ray_sep;
  }
  return( *this );
}

void rayCamera::initdims( rgbImage& image_in ) {
  // initialize_ray will test for this condition, but timing problems
  // can arise if that test happens while multiple threads are running.
  // Hence in multithreaded mode we need to call this before the first
  // call to initialize_ray for any given image.
  if ((image_in.xsize() != image_xdim)
      || (image_in.ysize() != image_ydim)) { // Need to adjust for new image
    float hfov, vfov;

    image_xdim= image_in.xsize();
    image_ydim= image_in.ysize();

    // Pick foveal angles in both directions
    if (image_xdim >= image_ydim) {
      vfov= fovea;
      hfov= (vfov * image_xdim) / image_ydim;
      // sin == angle for small angles
      ray_divergence= (DegtoRad * fovea) / image_ydim;
    }
    else {
      hfov= fovea;
      vfov= (hfov * image_ydim) / image_xdim;
      // sin == angle for small angles
      ray_divergence= (DegtoRad * fovea) / image_xdim;
    }

    float xmagnitude= 2.0*lookdist*tan( 0.5*DegtoRad*hfov )/image_xdim;
    screenx= screeni * xmagnitude;

    float ymagnitude= 2.0*lookdist*tan( 0.5*DegtoRad*vfov )/image_ydim;
    screeny= screenj * ymagnitude;

    par_ray_sep= 0.5*(xmagnitude + ymagnitude);

    firstray= (lookdir * lookdist)
      - (((screenx * image_xdim) + (screeny * image_ydim)) * 0.5)
      + ((screenx + screeny)*0.5);

    par_firstpoint= 
      lookfm - (((screenx * image_xdim) + (screeny * image_ydim)) * 0.5)
      + ((screenx + screeny)*0.5);
  }

}

void rayCamera::initialize_ray( Ray *ray, rgbImage& image_in, int i, int j )
{
  if ((image_in.xsize() != image_xdim)
      || (image_in.ysize() != image_ydim)) { // Need to adjust for new image
    float hfov, vfov;

    image_xdim= image_in.xsize();
    image_ydim= image_in.ysize();

    // Pick foveal angles in both directions
    if (image_xdim >= image_ydim) {
      vfov= fovea;
      hfov= (vfov * image_xdim) / image_ydim;
      // sin == angle for small angles
      ray_divergence= (DegtoRad * fovea) / image_ydim;
    }
    else {
      hfov= fovea;
      vfov= (hfov * image_ydim) / image_xdim;
      // sin == angle for small angles
      ray_divergence= (DegtoRad * fovea) / image_xdim;
    }

    float xmagnitude= 2.0*lookdist*tan( 0.5*DegtoRad*hfov )/image_xdim;
    screenx= screeni * xmagnitude;

    float ymagnitude= 2.0*lookdist*tan( 0.5*DegtoRad*vfov )/image_ydim;
    screeny= screenj * ymagnitude;

    par_ray_sep= 0.5*(xmagnitude + ymagnitude);

    firstray= (lookdir * lookdist)
      - (((screenx * image_xdim) + (screeny * image_ydim)) * 0.5)
      + ((screenx + screeny)*0.5);

    par_firstpoint= 
      lookfm - (((screenx * image_xdim) + (screeny * image_ydim)) * 0.5)
      + ((screenx + screeny)*0.5);
  }

  if (parallel_flag) {
    ray->origin= par_firstpoint + ((screenx * i) + (screeny * j));
    ray->direction= lookdir;
    ray->initial_separation= par_ray_sep;
    ray->divergence= 0.0;
  }
  else {
    // Perspective projection
    ray->origin= lookfm;
    ray->direction= firstray + ((screenx * i) + (screeny * j));
    ray->direction.normalize();
    ray->initial_separation= 0.0;
    ray->divergence= ray_divergence;
  }
  ray->length= -hither;
  ray->image_x= i;
  ray->image_y= j;
  ray->active= 1;
  ray->termination_length= -yon;
  ray->termination_color= background;
  ray->debug_me= 0;
  ray->rescale_me= 0;
  // Color stays transparent black, its constructed value
  // Qual stays zero values, its constructed value
}

