/****************************************************************************
 * samplevolume.cc
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
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "tfun.h"

baseSampleVolume::baseSampleVolume( const GridInfo& grid_in, 
				    baseTransferFunction& tfun_in,
				    int ndatavol, DataVolume** data_table ) 
: grid(grid_in)
{
  int i;

  if (ndatavol<tfun_in.ndata()) {
    fprintf(stderr,
	 "baseSampleVolume::baseSampleVolume: dataset count mismatch!\n");
    exit(-1);
  }
  num_datavols= ndatavol;
  datavols= new DataVolume*[num_datavols];
  for (i=0; i<num_datavols; i++) datavols[i]= data_table[i];

  tfun= &tfun_in;

  // size_scale gets inverse maximum pixel dimension
  float size_x= 1.0/grid.dx();
  float size_y= 1.0/grid.dy();
  float size_z= 1.0/grid.dz();
  size_scale= (size_y > size_x) ? ((size_y > size_z) ? size_y : size_z)
    : ((size_x > size_z) ? size_x : size_z);
  inv_voxel_aspect_ratio= gVector(size_x/size_scale, size_y/size_scale,
			    size_z/size_scale);

  // Check that all data volumes are commensurate
  for (i=0; i<num_datavols; i++) {
    if (grid.bbox() != datavols[i]->boundbox()) {
      fprintf(stderr,
   "baseSampleVolume::baseSampleVolume: non-commensurate data volume %d!\n", 
	      i);
      exit(-1);
    }
  }
}

baseSampleVolume::baseSampleVolume( const baseSampleVolume& other )
: grid( other.grid )
{
  tfun= other.tfun;
  num_datavols= other.num_datavols;
  size_scale= other.size_scale;
  inv_voxel_aspect_ratio= other.inv_voxel_aspect_ratio;

  int i;
  for (i=0; i<num_datavols; i++) datavols[i]= other.datavols[i];
}

baseSampleVolume::~baseSampleVolume()
{
  delete [] datavols;
}

void baseSampleVolume::regenerate( baseTransferFunction& tfun_in,
				   int ndatavol, DataVolume** data_table )
{
  if (ndatavol<tfun_in.ndata()) {
    fprintf(stderr,
	 "baseSampleVolume::baseSampleVolume: dataset count mismatch!\n");
    exit(-1);
  }
  num_datavols= ndatavol;
  delete [] datavols;
  datavols= new DataVolume*[num_datavols];
  for (int i=0; i<num_datavols; i++) datavols[i]= data_table[i];

  tfun= &tfun_in;
}

void baseSampleVolume::set_size_scale( const float new_scale )
{
  float size_x= 1.0/grid.dx();
  float size_y= 1.0/grid.dy();
  float size_z= 1.0/grid.dz();
  float fac= (size_y > size_x) ? ((size_y > size_z) ? size_y : size_z)
    : ((size_x > size_z) ? size_x : size_z);
  size_scale= fac * new_scale;
}

float baseSampleVolume::get_size_scale() const 
{
  float size_x= 1.0/grid.dx();
  float size_y= 1.0/grid.dy();
  float size_z= 1.0/grid.dz();
  float fac= (size_y > size_x) ? ((size_y > size_z) ? size_y : size_z)
    : ((size_x > size_z) ? size_x : size_z);

  return( size_scale/fac );
}
