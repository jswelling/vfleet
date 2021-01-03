/****************************************************************************
 * vcompvren.h
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

#include "compvren.h"

class Compositor;

class vcompDataVolume: public compDataVolume {
friend class vcompVRen;
public:
  ~vcompDataVolume(); // *** deletes children passed to constructor! ***
  void load_xplane( const DataType *data_in, int which_x );
  void load_yplane( const DataType *data_in, int which_y );
  void load_zplane( const DataType *data_in, int which_z );
protected:
  vcompDataVolume( int xdim_in, int ydim_in, int zdim_in,
		   const gBoundBox& bbox_in, 
		   int nchildren_in, DataVolume **children_in,
		   DataVolume *parent_in= NULL );
};

class vcompSampleVolume: public compSampleVolume {
friend class vcompVRen;
public:
  ~vcompSampleVolume();
private:
  vcompSampleVolume( const gBoundBox& boundbox_in, 
		     baseTransferFunction& tfun_in,
		     int ndatavol, DataVolume** data_table,
		     int nchildren_in, baseVRen** child_rens );
};

class vcompVRen: public compVRen {
public:
  vcompVRen( int nprocs, baseLogger *logger, baseImageHandler *imagehandler, 
	     void (*error_handler)(int error_id, baseVRen *renderer), 
	     void (*fatal_handler)(int error_id, baseVRen *renderer),
	     void (*service_call)());
  vcompVRen( int nprocs, baseLogger *logger, baseImageHandler *imagehandler, 
	     baseVRen *owner,
	     void (*service_call)());
  ~vcompVRen(); 
  DataVolume *create_data_volume( int xdim, int ydim, int zdim, 
				  const gBoundBox &bbox );
  baseSampleVolume *create_sample_volume( const gBoundBox& bbox,
					  baseTransferFunction& tfun,
					  const int ndatavol, 
					  DataVolume** data_table );
  static gBoundBox bbox_octant( const gBoundBox& bbox_in, int octant );
private:
};
