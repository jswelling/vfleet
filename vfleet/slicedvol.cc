/****************************************************************************
 * slicedvol.cc
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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "datafile.h"
#include "slicedvol.h"

static const float GRAD_RESCALE= 1.0;

/////
//  This class needs to avoid the base class's access() method, because
//  it can generate references beyond the allocated buffer!
/////

sliceDataVolume::sliceDataVolume( baseDataFile* dfile_in, 
				  const gBoundBox& bbox_in )
: DataVolume( 1, 1, 1, bbox_in )
{
  file= dfile_in;
  delete [] data;
  data= NULL;
  grid= GridInfo( file->xsize(), file->ysize(), file->zsize(), bbox_in );
  which_dir= -1;
  current_plane= -1;
  max_grad_mag= 0;
  idim= jdim= 0;
  deltax= deltay= deltaz= 1.0;
}

sliceDataVolume::~sliceDataVolume()
{
  delete [] data;
}

DataType sliceDataVolume::val( const int i, const int j, const int k ) const
{
  switch (which_dir) {
  case 0: // x plane
    return *(data + j*jdim + k);
    break;
  case 1: // y plane
    return *(data + i*idim + k);  // Need to use transpose
    break;
  case 2: // z plane
    return *(data + i*jdim + j);
    break;
  default: // no plane ready
    return 0;
    break;
  }
}

gVector sliceDataVolume::gradient( const int i, const int j, 
				   const int k ) const
{
  float gradx, grady;
  switch (which_dir) {
  case 0:
    if (j!=0 && j!=(idim-1))
      gradx= deriv_centered( flat_fval(j-1,k), flat_fval(j+1,k), deltay );
    else if (j==0)
      gradx= deriv_forwards( flat_fval(j,k), flat_fval(j+1,k), 
			     flat_fval(j+2,k), deltay );
    else gradx= deriv_forwards( flat_fval(j,k), flat_fval(j-1,k),
				flat_fval(j-2,k), -deltay );
    if (k!=0 && k!=(jdim-1))
      grady= deriv_centered( flat_fval(j,k+1), flat_fval(j,k-1), deltaz );
    else if (k==0)
      grady= deriv_forwards( flat_fval(j,k), flat_fval(j,k+1), 
			     flat_fval(j,k+2), deltaz );
    else grady= deriv_forwards( flat_fval(j,k), flat_fval(j,k-1),
				flat_fval(j,k-2), -deltaz );
    return gVector( 0.5*(fabs(gradx)+fabs(grady)), gradx, grady );
    break;
  case 1:
    // Need to use transpose
    if (i!=0 && i!=(idim-1))
      gradx= deriv_centered( flat_fval(i-1,k), flat_fval(i+1,k), deltaz );
    else if (j==0)
      gradx= deriv_forwards( flat_fval(i,k), flat_fval(i+1,k), 
			     flat_fval(i+2,k), deltaz );
    else gradx= deriv_forwards( flat_fval(i,k), flat_fval(i-1,k),
				flat_fval(i-2,k), -deltaz );
    if (k!=0 && k!=(jdim-1))
      grady= deriv_centered( flat_fval(i,k+1), flat_fval(i,k-1), deltaz );
    else if (k==0)
      grady= deriv_forwards( flat_fval(i,k), flat_fval(i,k+1), 
			     flat_fval(i,k+2), deltax );
    else grady= deriv_forwards( flat_fval(i,k), flat_fval(i,k-1),
				flat_fval(i,k-2), -deltax );
    return gVector( grady, 0.5*(fabs(gradx)+fabs(grady)), gradx );
    break;
  case 2:
    if (i!=0 && i!=(idim-1))
      gradx= deriv_centered( flat_fval(i-1,j), flat_fval(i+1,j), deltax );
    else if (i==0)
      gradx= deriv_forwards( flat_fval(i,j), flat_fval(i+1,j), 
			     flat_fval(i+2,j), deltax );
    else gradx= deriv_forwards( flat_fval(i,j), flat_fval(i-1,j),
				flat_fval(i-2,j), -deltax );
    if (j!=0 && j!=(jdim-1))
      grady= deriv_centered( flat_fval(i,j+1), flat_fval(i,j-1), deltay );
    else if (j==0)
      grady= deriv_forwards( flat_fval(i,j), flat_fval(i,j+1), 
			     flat_fval(i,j+2), deltay );
    else grady= deriv_forwards( flat_fval(i,j), flat_fval(i,j-1),
				flat_fval(i,j-2), -deltay );
    return gVector( gradx, grady, 0.5*(fabs(gradx)+fabs(grady)) );
    break;
  default:
    return gVector(0.0,0.0,0.0);
    break;
  }
}

void sliceDataVolume::prep_xplane( const int which_x )
{
  if ( (which_dir != 0) || (current_plane != which_x) ) {
    which_dir= 0;
    current_plane= which_x;
    idim= file->ysize();
    jdim= file->zsize();
    deltax= 1.0;
    deltay= (grid.bbox().ymax()-grid.bbox().ymin())/(idim-1);
    deltaz= (grid.bbox().zmax()-grid.bbox().zmin())/(jdim-1);
    delete [] data;
    data= (unsigned char *)(file->get_xplane(which_x, baseDataFile::ByteU));
  }
}

void sliceDataVolume::prep_yplane( const int which_y )
{
  if ( (which_dir != 1) || (current_plane != which_y) ) {
    which_dir= 1;
    current_plane= which_y;
    idim= file->zsize();
    jdim= file->xsize();
    deltax= (grid.bbox().xmax()-grid.bbox().xmin())/(jdim-1);
    deltay= 1.0;
    deltaz= (grid.bbox().zmax()-grid.bbox().zmin())/(idim-1);
    delete [] data;
    data= (unsigned char *)(file->get_yplane(which_y, baseDataFile::ByteU));
  }
}

void sliceDataVolume::prep_zplane( const int which_z )
{
  if ( (which_dir != 2) || (current_plane != which_z) ) {
    which_dir= 2;
    current_plane= which_z;
    idim= file->xsize();
    jdim= file->ysize();
    deltax= (grid.bbox().xmax()-grid.bbox().xmin())/(idim-1);
    deltay= (grid.bbox().ymax()-grid.bbox().ymin())/(jdim-1);
    deltaz= 1.0;
    delete [] data;
    data= (unsigned char *)(file->get_zplane(which_z, baseDataFile::ByteU));
  }
}

void sliceDataVolume::finish_init()
{
  // Make a wild stab at the maximum gradient
  gVector max_grad_vec( (float)(DataTypeMax-DataTypeMin)/grid.dx(),
			(float)(DataTypeMax-DataTypeMin)/grid.dy(),
			(float)(DataTypeMax-DataTypeMin)/grid.dz() );

  float max_possible_gradient= max_grad_vec.length();

  set_max_gradient( max_possible_gradient );
}
