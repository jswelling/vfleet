/****************************************************************************
 * bcompvren.h
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

#include "compvren.h"

class Compositor;

class bcompDataVolume: public compDataVolume {
friend class bcompVRen;
public:
  ~bcompDataVolume(); // *** deletes children passed to constructor! ***
  void load_xplane( const DataType *data_in, int which_x );
  void load_yplane( const DataType *data_in, int which_y );
  void load_zplane( const DataType *data_in, int which_z );
protected:
  bcompDataVolume( const int split_plane_in,
		   int xdim_in, int ydim_in, int zdim_in,
		   const gBoundBox& bbox_in, 
		   int nchildren_in, DataVolume **children_in,
		   DataVolume *parent_in= NULL );
  int split_plane;
};

class bcompSampleVolume: public compSampleVolume {
friend class bcompVRen;
public:
  ~bcompSampleVolume();
protected:
  bcompSampleVolume( const int split_plane_in,
		     const GridInfo& grid_in, 
		     baseTransferFunction& tfun_in,
		     int ndatavol, DataVolume** data_table,
		     int nchildren_in, baseVRen** child_rens );
  int split_plane;
  gBoundBox bbox_half( const gBoundBox& bbox_in, const int id );
};

class bcompVRen: public compVRen {
public:
  bcompVRen( const int type, 
	     baseLogger *logger, baseImageHandler *imagehandler, 
	     void (*ready_handler)(baseVRen *renderer, void* cb_data),
	     void* ready_cb_data_in,
	     void (*error_handler)(int error_id, baseVRen *renderer), 
	     void (*fatal_handler)(int error_id, baseVRen *renderer),
	     void (*service_call)() );
  bcompVRen( const int type,
	     baseLogger *logger, baseImageHandler *imagehandler, 
	     void (*ready_handler)(baseVRen *renderer, void* cb_data),
	     void* ready_cb_data_in,
	     baseVRen *owner,
	     void (*service_call)() );
  ~bcompVRen(); 
  DataVolume *create_data_volume( int xdim, int ydim, int zdim, 
				  const gBoundBox &bbox );
  baseSampleVolume *create_sample_volume( const GridInfo& grid_in,
					  baseTransferFunction& tfun,
					  const int ndatavol, 
					  DataVolume** data_table );
protected:
  gBoundBox bbox_half( const gBoundBox& bbox_in, const int id );
  int split_plane;
};
