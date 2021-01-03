/****************************************************************************
 * bcompvren.cc
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

#ifdef SYSV_TIMING
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
#include "raycastvren.h"
#include "bcompvren.h"
#include "bcomposite.h"

/*
  Notes:
 */

static inline gBoundBox split_bbox( const gBoundBox& bbox, const int id,
				    const int split_plane )
{
  switch (split_plane) {
  case 0: // constant x
    {
      float xhalf= 0.5*(bbox.xmax() + bbox.xmin());
      if (id==0) return gBoundBox( bbox.xmin(), bbox.ymin(), bbox.zmin(),
				   xhalf, bbox.ymax(), bbox.zmax() );
      else return gBoundBox( xhalf, bbox.ymin(), bbox.zmin(),
			     bbox.xmax(), bbox.ymax(), bbox.zmax() );
    }
    break;
  case 1: // constant y
    {
      float yhalf= 0.5*(bbox.ymax() + bbox.ymin());
      if (id==0) return gBoundBox( bbox.xmin(), bbox.ymin(), bbox.zmin(),
				   bbox.xmax(), yhalf, bbox.zmax() );
      else return gBoundBox( bbox.xmin(), yhalf, bbox.zmin(),
			     bbox.xmax(), bbox.ymax(), bbox.zmax() );
    }
    break;
  case 2: // constant z
    {
      float zhalf= 0.5*(bbox.zmax() + bbox.zmin());
      if (id==0) return gBoundBox( bbox.xmin(), bbox.ymin(), bbox.zmin(),
				   bbox.xmax(), bbox.ymax(), zhalf );
      else return gBoundBox( bbox.xmin(), bbox.ymin(), zhalf,
			     bbox.xmax(), bbox.ymax(), bbox.zmax() );
    }
    break;
  }

  return( gBoundBox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 ) ); // not reached
}

bcompDataVolume::bcompDataVolume( const int split_plane_in,
				  int xdim_in, int ydim_in, int zdim_in,
				  const gBoundBox& bbox_in,
				  int nchildren_in, DataVolume **children_in,
				  DataVolume *parent_in )
: compDataVolume( xdim_in, ydim_in, zdim_in, bbox_in, 
		  nchildren_in, children_in, parent_in )
{
  split_plane= split_plane_in;
}

bcompDataVolume::~bcompDataVolume()
{
  // Nothing to delete- parent class handles everything
}

void bcompDataVolume::load_xplane( const DataType *data_in, int which_x )
{
  switch (split_plane) {
  case 0: // split on constant-x plane
    {
      if (which_x < children[0]->xsize())
	children[0]->load_xplane( data_in, which_x );
      else children[1]->load_xplane( data_in, which_x-children[0]->xsize() );
    }
    break;
  case 1: // split on constant-y plane
    {
      int ydim_half= children[0]->ysize();
      DataType *plane0= new DataType[ydim_half*zsize()];
      DataType *plane1= new DataType[(ysize()-ydim_half)*zsize()];
      DataType *runner0= plane0;
      DataType *runner1= plane1;
      for (int j=0; j<ysize(); j++)
	for (int k=0; k<zsize(); k++) {
	  if (j<ydim_half) {
	    *runner0++= data_in[j*zsize()+k];
	  }
	  else {
	    *runner1++= data_in[j*zsize()+k];
	  }
	}
      children[0]->load_xplane( plane0, which_x );
      children[1]->load_xplane( plane1, which_x );
      delete [] plane0;
      delete [] plane1;
    }
    break;
  case 2: // split on constant_z plane
    {
      int zdim_half= children[0]->zsize();
      DataType *plane0= new DataType[ysize()*zdim_half];
      DataType *plane1= new DataType[ysize()*(zsize()-zdim_half)];
      DataType *runner0= plane0;
      DataType *runner1= plane1;
      for (int j=0; j<ysize(); j++)
	for (int k=0; k<zsize(); k++) {
	  if (k<zdim_half) {
	    *runner0++= data_in[j*zsize()+k];
	  }
	  else {
	    *runner1++= data_in[j*zsize()+k];
	  }
	}
      children[0]->load_xplane( plane0, which_x );
      children[1]->load_xplane( plane1, which_x );
      delete [] plane0;
      delete [] plane1;
    }
    break;
  }
}

void bcompDataVolume::load_yplane( const DataType *data_in, int which_y )
{
  switch (split_plane) {
  case 0: // split on constant-x plane
    {
      int xdim_half= children[0]->xsize();
      DataType *plane0= new DataType[zsize()*xdim_half];
      DataType *plane1= new DataType[zsize()*(xsize()-xdim_half)];
      DataType *runner0= plane0;
      DataType *runner1= plane1;

      for (int i=0; i<xsize(); i++)
	for (int k=0; k<zsize(); k++) {
	  if (i<xdim_half) {
	    *runner0++= data_in[i*zsize()+k];
	  }
	  else {
	    *runner1++= data_in[i*zsize()+k];
	  }
	}
      children[0]->load_yplane( plane0, which_y );
      children[1]->load_yplane( plane1, which_y );
      delete [] plane0;
      delete [] plane1;
    }
    break;
  case 1: // split on constant-y plane
    {
      if (which_y < children[0]->ysize())
	children[0]->load_yplane( data_in, which_y );
      else children[1]->load_yplane( data_in, which_y-children[0]->ysize() );
    }
    break;
  case 2: // split on constant_z plane
    {
      int zdim_half= children[0]->zsize();
      DataType *plane0= new DataType[zdim_half*xsize()];
      DataType *plane1= new DataType[(zsize()-zdim_half)*xsize()];
      DataType *runner0= plane0;
      DataType *runner1= plane1;

      for (int i=0; i<xsize(); i++)
	for (int k=0; k<zsize(); k++) {
	  if (k<zdim_half) {
	    *runner0++= data_in[i*zsize()+k];
	  }
	  else {
	    *runner1++= data_in[i*zsize()+k];
	  }
	}
      children[0]->load_yplane( plane0, which_y );
      children[1]->load_yplane( plane1, which_y );
      delete [] plane0;
      delete [] plane1;
    }
    break;
  }
}

void bcompDataVolume::load_zplane( const DataType *data_in, int which_z )
{
  switch (split_plane) {
  case 0: // split on constant-x plane
    {
      int xdim_half= children[0]->xsize();
      DataType *plane0= new DataType[xdim_half*ysize()];
      DataType *plane1= new DataType[(xsize()-xdim_half)*ysize()];
      DataType *runner0= plane0;
      DataType *runner1= plane1;

      for (int i=0; i<xsize(); i++)
	for (int j=0; j<ysize(); j++) {
	  if (i<xdim_half) {
	    *runner0++= data_in[i*ysize()+j];
	  }
	  else {
	    *runner1++= data_in[i*ysize()+j];
	  }
	}
      children[0]->load_zplane( plane0, which_z );
      children[1]->load_zplane( plane1, which_z );
      delete [] plane0;
      delete [] plane1;
    }
    break;
  case 1: // split on constant-y plane
    {
      int ydim_half= children[0]->ysize();
      DataType *plane0= new DataType[xsize()*ydim_half];
      DataType *plane1= new DataType[xsize()*(ysize()-ydim_half)];
      DataType *runner0= plane0;
      DataType *runner1= plane1;

      for (int i=0; i<xsize(); i++)
	for (int j=0; j<ysize(); j++) {
	  if (j<ydim_half) {
	    *runner0++= data_in[i*ysize()+j];
	  }
	  else {
	    *runner1++= data_in[i*ysize()+j];
	  }
	}
      children[0]->load_zplane( plane0, which_z );
      children[1]->load_zplane( plane1, which_z );
      delete [] plane0;
      delete [] plane1;
    }
    break;
  case 2: // split on constant_z plane
    {
      if (which_z < children[0]->zsize())
	children[0]->load_zplane( data_in, which_z );
      else children[1]->load_zplane( data_in, which_z-children[0]->zsize() );
    }
    break;
  }
}

bcompSampleVolume::bcompSampleVolume( const int split_plane_in,
				      const GridInfo& grid_in,
				      baseTransferFunction& tfun_in,
				      int ndatavol, DataVolume** data_table,
				      int nchildren_in, baseVRen** child_rens )
: compSampleVolume( grid_in, tfun_in, ndatavol, data_table, nchildren_in )
{
  split_plane= split_plane_in;

  // Casts work as long as the same VRen created the input and this.
  compTransferFunction* c_tfun= (compTransferFunction *)&tfun_in;
  bcompDataVolume** bc_data_table= (bcompDataVolume **)data_table;

  DataVolume **kid_table;
  if (ndatavol) kid_table= new DataVolume*[ndatavol];
  else kid_table= NULL;

  if (nchildren != 2) {
    fprintf(stderr,"bcompSampleVolume::bcompSampleVolume: not 2 children!\n");
    exit(-1);
  }

  switch (split_plane) {
  case 0: // split x
    {
      int xdim_half= grid.xsize()/2;
      for (int i=0; i<ndatavol; i++) kid_table[i]= bc_data_table[i]->child(0);
      children[0]= 
	child_rens[0]->create_sample_volume(GridInfo(xdim_half,
						     grid.ysize(),
						     grid.zsize(),
						     bbox_half(grid.bbox(),0)),
					    *(c_tfun->child(0)),
					    ndatavol, kid_table);
      for (int i=0; i<ndatavol; i++) kid_table[i]= bc_data_table[i]->child(1);
      children[1]= 
	child_rens[1]->create_sample_volume(GridInfo(grid.xsize()-xdim_half,
						     grid.ysize(),
						     grid.zsize(),
						     bbox_half(grid.bbox(),1)),
					    *(c_tfun->child(1)),
					    ndatavol, kid_table);
    }
    break;
  case 1: // split y
    {
      int ydim_half= grid.ysize()/2;

      for (int i=0; i<ndatavol; i++) kid_table[i]= bc_data_table[i]->child(0);
      children[0]= 
	child_rens[0]->create_sample_volume(GridInfo(grid.xsize(),
						     ydim_half,
						     grid.zsize(),
						     bbox_half(grid.bbox(),0)),
					    *(c_tfun->child(0)),
					    ndatavol, kid_table);
      for (int i=0; i<ndatavol; i++) kid_table[i]= bc_data_table[i]->child(1);
      children[1]= 
	child_rens[1]->create_sample_volume(GridInfo(grid.xsize(),
						     grid.ysize() - ydim_half,
						     grid.zsize(),
						     bbox_half(grid.bbox(),1)),
					    *(c_tfun->child(1)),
					    ndatavol, kid_table);
    }
    break;
  case 2: // split z
    {
      int zdim_half= grid.zsize()/2;

      for (int i=0; i<ndatavol; i++) kid_table[i]= bc_data_table[i]->child(0);
      children[0]= 
	child_rens[0]->create_sample_volume(GridInfo(grid.xsize(),
						     grid.ysize(),
						     zdim_half,
						     bbox_half(grid.bbox(),0)),
					    *(c_tfun->child(0)),
					    ndatavol, kid_table);
      for (int i=0; i<ndatavol; i++) kid_table[i]= bc_data_table[i]->child(1);
      children[1]= 
	child_rens[1]->create_sample_volume(GridInfo(grid.xsize(),
						     grid.ysize(),
						     grid.zsize() - zdim_half,
						     bbox_half(grid.bbox(),1)),
					    *(c_tfun->child(1)),
					    ndatavol, kid_table);
    }
    break;

  }

  delete [] kid_table;


}

bcompSampleVolume::~bcompSampleVolume()
{
  // Nothing to delete; parent class does it all
}

gBoundBox bcompSampleVolume::bbox_half( const gBoundBox& bbox, const int id)
{
  return split_bbox( bbox, id, split_plane );
}

bcompVRen::bcompVRen(const int type,
		     baseLogger *logger, baseImageHandler *imagehandler, 
		     void (*ready_handler)(baseVRen *renderer,
					   void* ready_cb_data_in),
		     void* ready_cb_data_in,
		     void (*error_handler)(int error_id, 
					   baseVRen *renderer), 
		     void (*fatal_handler)(int error_id, 
					   baseVRen *renderer),
		     void (*service_call)() ) 
: compVRen( 2, logger, imagehandler, ready_handler, ready_cb_data_in,
	    error_handler, fatal_handler, service_call )
{

  // Decode type info
  int nthreads;
  int nprocs;
  netVRen::decode_type( type, &split_plane, &nprocs, &nthreads );

  compositor= new bCompositor( split_plane, gBoundBox(), 
			       imagehandler, logger );

  // Create the children.  Any extra processors go in the local side of
  // the tree, in the hopes that it will help keep us from falling behind
  // on compositing.
  int hnprocs= nprocs/2;
  int encoded_kid_type_and_procs= 
    netVRen::encode_type( (split_plane + 1)%3, hnprocs, nthreads );
  children[0]= new netVRen( encoded_kid_type_and_procs,
			    logger, compositor->get_ihandler(0),
			    compVRen::ready_callback, (void*)this,
			    error_handler, fatal_handler );
  if (nprocs-hnprocs > 1) {
    encoded_kid_type_and_procs= 
      netVRen::encode_type( (split_plane + 1)%3, nprocs-hnprocs, nthreads );
    children[1]= new bcompVRen( encoded_kid_type_and_procs,
				logger, compositor->get_ihandler(1),
			        compVRen::ready_callback, (void*)this,
				this, service_call );
  }
  else {
    children[1]= new raycastVRen( logger, compositor->get_ihandler(1),
				  compVRen::ready_callback, (void*)this,
				  this, service_call, nthreads );
  }
}

bcompVRen::bcompVRen(const int type,
		     baseLogger *logger, baseImageHandler *imagehandler, 
		     void (*ready_handler)(baseVRen *renderer,
					   void* ready_cb_data_in),
		     void* ready_cb_data_in,
		     baseVRen *owner, void (*service_call)() )
: compVRen( 2, logger, imagehandler, ready_handler, ready_cb_data_in,
	   owner, service_call )
{
  // Decode type info
  int nthreads;
  int nprocs;
  netVRen::decode_type( type, &split_plane, &nprocs, &nthreads );

  compositor= new bCompositor( split_plane, gBoundBox(), 
			       imagehandler, logger );

  // Create the children.  Any extra processors go in the local side of
  // the tree, in the hopes that it will help keep us from falling behind
  // on compositing.
  int hnprocs= nprocs/2;
  int encoded_kid_type_and_procs= 
    netVRen::encode_type( (split_plane + 1)%3, hnprocs, nthreads );

  children[0]= new netVRen( encoded_kid_type_and_procs,
			    logger, compositor->get_ihandler(0),
			    compVRen::ready_callback, (void*)this,
			    this );
  if (nprocs-hnprocs>1) {
    encoded_kid_type_and_procs= 
      netVRen::encode_type( (split_plane + 1)%3, nprocs-hnprocs, nthreads );
    children[1]= new bcompVRen( encoded_kid_type_and_procs,
				logger, compositor->get_ihandler(1),
			        compVRen::ready_callback, (void*)this,
				this, service_call );
  }
  else {
    children[1]= new raycastVRen( logger, compositor->get_ihandler(1),
				  compVRen::ready_callback, (void*)this,
				  this, service_call, nthreads );
  }
}

bcompVRen::~bcompVRen()
{
  // Nothing to delete; base classes handle it all
}

DataVolume *bcompVRen::create_data_volume( int xdim, int ydim, int zdim, 
					   const gBoundBox& bbox )
{
  DataVolume **v_table= new DataVolume*[nchildren];

  switch (split_plane) {
  case 0: // split x
    {
      int xdim_half= xdim/2;
      v_table[0]= 
	children[0]->create_data_volume( xdim_half, ydim, zdim,
					 bbox_half( bbox, 0 ) );
      v_table[1]= 
	children[1]->create_data_volume( xdim - xdim_half, ydim, zdim,
					 bbox_half( bbox, 1 ) );
    }
    break;
  case 1: // split y
    {
      int ydim_half= ydim/2;
      v_table[0]= 
	children[0]->create_data_volume( xdim, ydim_half, zdim,
					 bbox_half( bbox, 0 ) );
      v_table[1]= 
	children[1]->create_data_volume( xdim, ydim - ydim_half, zdim,
					 bbox_half( bbox, 1 ) );
    }
    break;
  case 2: // split z
    {
      int zdim_half= zdim/2;
      v_table[0]= 
	children[0]->create_data_volume( xdim, ydim, zdim_half,
					 bbox_half( bbox, 0 ) );
      v_table[1]= 
	children[1]->create_data_volume( xdim, ydim, zdim - zdim_half,
					 bbox_half( bbox, 1 ) );
    }
    break;

  }

  bcompDataVolume *result= new bcompDataVolume( split_plane,
						xdim, ydim, zdim,
						bbox,
						nchildren, v_table );

  delete [] v_table;

  return result;
}

baseSampleVolume *bcompVRen::create_sample_volume( const GridInfo& grid_in,
						baseTransferFunction& tfun_in,
						   const int ndatavol, 
						   DataVolume** data_table ) 
{
  baseSampleVolume *result= 
    new bcompSampleVolume( split_plane, grid_in, tfun_in, ndatavol, data_table,
			   nchildren, children );
  return( result );
}

gBoundBox bcompVRen::bbox_half( const gBoundBox& bbox, const int id)
{
  return split_bbox( bbox, id, split_plane );
}

