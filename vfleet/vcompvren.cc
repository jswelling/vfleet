/****************************************************************************
 * vcompvren.cc
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
#include <sys/types.h>

#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include <time.h>
#include <unistd.h>
#else /* ifdef CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E */

#ifdef DECCXX
extern "C" {
#endif

#if SYSV_TIMING
#include <sys/times.h>
#else
#include <sys/time.h>
#endif

#include <sys/resource.h>
#ifdef DECCXX
}
#endif

#endif /* if CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E */

#include "basenet.h"
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "tfun.h"
#include "netvren.h"
#include "vcompvren.h"
#include "ocomposite.h"

/*
  Notes:
 */

vcompDataVolume::vcompDataVolume( int xdim_in, int ydim_in, int zdim_in,
				  const gBoundBox& bbox_in,
				  int nchildren_in, DataVolume **children_in,
				  DataVolume *parent_in )
: compDataVolume( xdim_in, ydim_in, zdim_in, bbox_in, 
		  nchildren_in, children_in, parent_in )
{
  // Nothing left to do
}

vcompDataVolume::~vcompDataVolume()
{
  // Nothing to delete- parent class handles everything
}

void vcompDataVolume::load_xplane( const DataType *data_in, int which_x )
{
  int xdim_half= children[0]->xsize();
  int ydim_half= children[0]->ysize();
  int zdim_half= children[0]->zsize();

  DataType *plane0= new DataType[ydim_half*zdim_half];
  DataType *plane1= new DataType[(ysize()-ydim_half)*zdim_half];
  DataType *plane2= new DataType[ydim_half*(zsize()-zdim_half)];
  DataType *plane3= new DataType[(ysize()-ydim_half)*(zsize()-zdim_half)];
  DataType *runner0= plane0;
  DataType *runner1= plane1;
  DataType *runner2= plane2;
  DataType *runner3= plane3;

  for (int j=0; j<ysize(); j++)
    for (int k=0; k<zsize(); k++) {
      if (k<zdim_half) {
	if (j<ydim_half) {
	  *runner0++= data_in[j*zsize()+k];
	}
	else {
	  *runner1++= data_in[j*zsize()+k];
	}
      }
      else {
	if (j<ydim_half) {
	  *runner2++= data_in[j*zsize()+k];
	}
	else {
	  *runner3++= data_in[j*zsize()+k];
	}
      }
    }

  if (which_x<xdim_half) {
    children[0]->load_xplane( plane0, which_x );
    children[2]->load_xplane( plane1, which_x );
    children[4]->load_xplane( plane2, which_x );
    children[6]->load_xplane( plane3, which_x );
  }
  else {
    children[1]->load_xplane( plane0, which_x-xdim_half );
    children[3]->load_xplane( plane1, which_x-xdim_half );
    children[5]->load_xplane( plane2, which_x-xdim_half );
    children[7]->load_xplane( plane3, which_x-xdim_half );
  }

  delete [] plane0;
  delete [] plane1;
  delete [] plane2;
  delete [] plane3;
}

void vcompDataVolume::load_yplane( const DataType *data_in, int which_y )
{
  int xdim_half= children[0]->xsize();
  int ydim_half= children[0]->ysize();
  int zdim_half= children[0]->zsize();

  DataType *plane0= new DataType[zdim_half*xdim_half];
  DataType *plane1= new DataType[(zsize()-zdim_half)*xdim_half];
  DataType *plane2= new DataType[zdim_half*(xsize()-xdim_half)];
  DataType *plane3= new DataType[(zsize()-zdim_half)*(xsize()-xdim_half)];
  DataType *runner0= plane0;
  DataType *runner1= plane1;
  DataType *runner2= plane2;
  DataType *runner3= plane3;

  for (int i=0; i<xsize(); i++) {
    for (int k=0; k<zsize(); k++)
      if (i<xdim_half) {
	if (k<zdim_half) {
	  *runner0++= data_in[i*zsize()+k];
	}
	else {
	  *runner1++= data_in[i*zsize()+k];
	}
      }
      else {
	if (k<zdim_half) {
	  *runner2++= data_in[i*zsize()+k];
	}
	else {
	  *runner3++= data_in[i*zsize()+k];
	}
      }
    }

  if (which_y<xdim_half) {
    children[0]->load_yplane( plane0, which_y );
    children[4]->load_yplane( plane1, which_y );
    children[1]->load_yplane( plane2, which_y );
    children[5]->load_yplane( plane3, which_y );
  }
  else {
    children[2]->load_yplane( plane0, which_y-ydim_half );
    children[6]->load_yplane( plane1, which_y-ydim_half );
    children[3]->load_yplane( plane2, which_y-ydim_half );
    children[7]->load_yplane( plane3, which_y-ydim_half );
  }

  delete [] plane0;
  delete [] plane1;
  delete [] plane2;
  delete [] plane3;
}

void vcompDataVolume::load_zplane( const DataType *data_in, int which_z )
{
  int xdim_half= children[0]->xsize();
  int ydim_half= children[0]->ysize();
  int zdim_half= children[0]->zsize();

  DataType *plane0= new DataType[xdim_half*ydim_half];
  DataType *plane1= new DataType[(xsize()-xdim_half)*ydim_half];
  DataType *plane2= new DataType[xdim_half*(ysize()-ydim_half)];
  DataType *plane3= new DataType[(xsize()-xdim_half)*(ysize()-ydim_half)];
  DataType *runner0= plane0;
  DataType *runner1= plane1;
  DataType *runner2= plane2;
  DataType *runner3= plane3;

  for (int i=0; i<xsize(); i++)
    for (int j=0; j<ysize(); j++) {
      if (j<ydim_half) {
	if (i<xdim_half) {
	  *runner0++= data_in[i*ysize()+j];
	}
	else {
	  *runner1++= data_in[i*ysize()+j];
	}
      }
      else {
	if (i<xdim_half) {
	  *runner2++= data_in[i*ysize()+j];
	}
	else {
	  *runner3++= data_in[i*ysize()+j];
	}
      }
    }

  if (which_z<xdim_half) {
    children[0]->load_zplane( plane0, which_z );
    children[1]->load_zplane( plane1, which_z );
    children[2]->load_zplane( plane2, which_z );
    children[3]->load_zplane( plane3, which_z );
  }
  else {
    children[4]->load_zplane( plane0, which_z-xdim_half );
    children[5]->load_zplane( plane1, which_z-xdim_half );
    children[6]->load_zplane( plane2, which_z-xdim_half );
    children[7]->load_zplane( plane3, which_z-xdim_half );
  }

  delete [] plane0;
  delete [] plane1;
  delete [] plane2;
  delete [] plane3;
}

vcompSampleVolume::vcompSampleVolume( const gBoundBox& boundbox_in,
				      baseTransferFunction& tfun_in,
				      int ndatavol, DataVolume** data_table,
				      int nchildren_in, baseVRen** child_rens )
: compSampleVolume( boundbox_in, tfun_in, ndatavol, data_table, nchildren_in )
{
  // Casts work as long as the same VRen created the input and this.
  compTransferFunction* c_tfun= (compTransferFunction *)&tfun_in;
  vcompDataVolume** vc_data_table= (vcompDataVolume **)data_table;

  DataVolume **kid_table= new DataVolume*[ndatavol];
  for (int i=0; i<nchildren; i++) {

    for (int j=0; j<ndatavol; j++) kid_table[j]= vc_data_table[j]->child(i);
    children[i]= 
      child_rens[i]->create_sample_volume(vcompVRen::bbox_octant(bbox,i), 
					  *(c_tfun->child(i)),
					  ndatavol, kid_table);
  }
  delete [] kid_table;
}

vcompSampleVolume::~vcompSampleVolume()
{
  // Nothing to delete; parent class does it all
}

vcompVRen::vcompVRen(int nprocs, 
		     baseLogger *logger, baseImageHandler *imagehandler, 
		     void (*error_handler)(int error_id, 
					   baseVRen *renderer), 
		     void (*fatal_handler)(int error_id, 
					   baseVRen *renderer),
		     void (*service_call)()) 
: compVRen( 8, logger, imagehandler, 
	    error_handler, fatal_handler, service_call )
{
  compositor= new oCompositor( 2, 2, 2, gBoundBox(), imagehandler, logger );

  // Create children, in Morton index order
  int nvren_type= netVRen::encode_type( 0, nprocs/8, 0);
  for (int i=0; i<8; i++) 
    children[i]= new netVRen( nvren_type, logger, compositor->get_ihandler(i),
			      error_handler, fatal_handler );
}

vcompVRen::vcompVRen(int nprocs, 
		     baseLogger *logger, baseImageHandler *imagehandler, 
		     baseVRen *owner, void (*service_call)())
: compVRen( 8, logger, imagehandler, owner, service_call )
{
  compositor= new oCompositor( 2, 2, 2, gBoundBox(), imagehandler, logger );

  // Create children, in Morton index order
  int nvren_type= netVRen::encode_type( 0, nprocs/8, 0);
  for (int i=0; i<8; i++)
    children[i]= new netVRen( nvren_type, logger, compositor->get_ihandler(i),
			      owner );
}

vcompVRen::~vcompVRen()
{
  // Nothing to delete; base classes handle it all
}

DataVolume *vcompVRen::create_data_volume( int xdim, int ydim, int zdim, 
					   const gBoundBox& bbox )
{
  if (nchildren != 8) {
    fprintf(stderr,
	 "vcompVRen::create_data_volume: forgot to update for more kids!\n");
    exit(-1);
  }

  int xdim_half= xdim/2;
  int ydim_half= ydim/2;
  int zdim_half= zdim/2;

  DataVolume **v_table= new DataVolume*[nchildren];

  // front lower left
  v_table[0]= 
    children[0]->create_data_volume( xdim_half, ydim_half, zdim_half,
				     bbox_octant(bbox,0) );
  // front lower right
  v_table[1]=
    children[1]->create_data_volume( xdim-xdim_half, ydim_half, zdim_half,
				     bbox_octant(bbox,1) );
  // front upper left
  v_table[2]= 
    children[2]->create_data_volume( xdim_half, ydim-ydim_half, zdim_half,
				     bbox_octant(bbox,2) );
  // front upper right
  v_table[3]= 
    children[3]->create_data_volume( xdim-xdim_half, ydim-ydim_half, zdim_half,
				     bbox_octant(bbox,3) );
  // front lower left
  v_table[4]= 
    children[4]->create_data_volume( xdim_half, ydim_half, zdim-zdim_half,
				     bbox_octant(bbox,4) );
  // front lower right
  v_table[5]=
    children[5]->create_data_volume( xdim-xdim_half, ydim_half, zdim-zdim_half,
				     bbox_octant(bbox,5) );
  // front upper left
  v_table[6]= 
    children[6]->create_data_volume( xdim_half, ydim-ydim_half, zdim-zdim_half,
				     bbox_octant(bbox,6) );
  // front upper right
  v_table[7]= 
    children[7]->create_data_volume( xdim-xdim_half, ydim-ydim_half, 
				     zdim-zdim_half, bbox_octant(bbox,7) );

  vcompDataVolume *result= new vcompDataVolume( xdim, ydim, zdim,
						bbox,
						nchildren, v_table );

  delete [] v_table;

  return result;
}

baseSampleVolume *vcompVRen::create_sample_volume( const gBoundBox& bbox_in,
						baseTransferFunction& tfun_in,
						   const int ndatavol, 
						   DataVolume** data_table ) 
{
  baseSampleVolume *result= 
    new vcompSampleVolume( bbox_in, tfun_in, ndatavol, data_table,
			   nchildren, children );
  return( result );
}

gBoundBox vcompVRen::bbox_octant( const gBoundBox& bbox, int octant )
{
  float x_half= (bbox.xmax() + bbox.xmin())/2.0;
  float y_half= (bbox.ymax() + bbox.ymin())/2.0;
  float z_half= (bbox.zmax() + bbox.zmin())/2.0;

  switch (octant) {
  case 0:
    return( gBoundBox( bbox.xmin(),
		       bbox.ymin(),
		       bbox.zmin(),
		       x_half,
		       y_half,
		       z_half ) );
    break;
  case 1:
    return( gBoundBox( x_half,
		       bbox.ymin(),
		       bbox.zmin(),
		       bbox.xmax(),
		       y_half,
		       z_half ) );

    break;
  case 2:
    return( gBoundBox( bbox.xmin(),
		       y_half,
		       bbox.zmin(),
		       x_half,
		       bbox.ymax(),
		       z_half ) );
    break;
  case 3:
    return( gBoundBox( x_half,
		       y_half,
		       bbox.zmin(),
		       bbox.xmax(),
		       bbox.ymax(),
		       z_half ) );
    break;
  case 4:
    return( gBoundBox( bbox.xmin(),
		       bbox.ymin(),
		       z_half,
		       x_half,
		       y_half,
		       bbox.zmax() ) );
    break;
  case 5:
    return( gBoundBox( x_half,
		       bbox.ymin(),
		       z_half,
		       bbox.xmax(),
		       y_half,
		       bbox.zmax() ) );
    break;
  case 6:
    return( gBoundBox( bbox.xmin(),
		       y_half,
		       z_half,
		       x_half,
		       bbox.ymax(),
		       bbox.zmax() ) );
    break;
  case 7:
    return( gBoundBox( x_half,
		       y_half,
		       z_half,
		       bbox.xmax(),
		       bbox.ymax(),
		       bbox.zmax() ) );
    break;
  }

  return( gBoundBox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 ) ); // not reached
}
