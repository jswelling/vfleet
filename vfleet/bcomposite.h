/****************************************************************************
 * bcomposite.h
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

#include "composite.h"

class bCompositor : public Compositor {
public:
  bCompositor( const int split_plane, const gBoundBox& bbox_in, 
	      baseImageHandler *ihandler_in,
	      baseLogger *logger_in= NULL );
  ~bCompositor();
  void set_lookfrom( const gPoint& lookfm_in );
  void set_boundbox( const gBoundBox& bbox_in );
  void add_image( rgbImage *image, const int id_in );
private:
  void calc_centers();
  int split_plane;
  gPoint center[2];
  int plane_0_in;
  int plane_1_in;
  rgbImage *image_0;
  rgbImage *image_1;
};
