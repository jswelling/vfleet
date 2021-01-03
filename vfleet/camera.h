/****************************************************************************
 * camera.h
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
// Header for camera class

#ifndef CAMERA_H_INCLUDED

#define CAMERA_H_INCLUDED

#include "geometry.h"

class Camera {
public:
  Camera( const gPoint lookfm_in, const gPoint lookat_in, const gVector up_in, 
	  const float fovea_in, const float hither_in, const float yon_in,
	  const int parallel_flag_in=0 );
  Camera( const Camera& other );
  virtual ~Camera();
  virtual Camera& operator=( const Camera& other );
  gPoint from() const { return lookfm; }
  gPoint at() const { return lookat; }
  gVector updir() const { return up; }
  gVector pointing_dir() const { return lookdir; }
  float fov() const { return fovea; }
  float hither_dist() const { return hither; }
  float yon_dist() const { return yon; }
  // Caller must free trans returned by screen_projection_matrix() .
  // Frustrum transforms to 0<x<screenx, 0<y<screeny, minz<z<maxz
  // Objects far from the lookfm point map to z values near minz!
  gTransfm* screen_projection_matrix( const int screenx, const int screeny,
				      const int minz, const int maxz );
  int parallel_proj() const { return parallel_flag; }
  void set_parallel_proj() { parallel_flag= 1; }
  void set_perspective_proj() { parallel_flag= 0; }
protected:
  gPoint lookfm;
  gPoint lookat;
  gVector up;
  gVector lookdir;  // normalized
  float fovea;
  float hither;
  float yon;
  float lookdist;   // distance from lookfm to lookat
  gColor background;
  int parallel_flag;
};

#endif // ifdef CAMERA_H_INCLUDED
