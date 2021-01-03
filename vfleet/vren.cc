/****************************************************************************
 * vren.cc
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
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "tfun.h"
#include "datafile.h"

/*
  Notes:
 */

baseVRen::baseVRen( baseLogger *logger_in, baseImageHandler *imagehandler, 
		    void (*ready_handler)(baseVRen *renderer, void* cb_data),
		    void* ready_cb_data_in,
		    void (*error_handler)(int error_id, baseVRen *renderer), 
		    void (*fatal_handler)(int error_id, baseVRen *renderer) )
{
  logger= logger_in;
  ihandler= imagehandler;
  ready_proc= ready_handler;
  ready_cb_data= ready_cb_data_in;
  error_proc= error_handler;
  fatal_proc= fatal_handler;
  owner= NULL;
  image= NULL;
  options= 0;
  current_camera= NULL;
  current_quality= NULL;
  current_light_info= NULL;
  current_geom= NULL;
}

baseVRen::baseVRen( baseLogger *logger_in, baseImageHandler *imagehandler, 
		    void (*ready_handler)(baseVRen *renderer, void* cb_data),
		    void* ready_cb_data_in,
		    baseVRen *owner_in )
{
  logger= logger_in;
  ihandler= imagehandler;
  ready_proc= ready_handler;
  ready_cb_data= ready_cb_data_in;
  error_proc= NULL;
  fatal_proc= NULL;
  owner= owner_in;
  image= NULL;
  options= 0;
  current_camera= NULL;
  current_quality= NULL;
  current_light_info= NULL;
  current_geom= NULL;
}

baseVRen::~baseVRen()
{
  delete image;
}

void baseVRen::setOptionFlags( const VRenOptions ops )
{
  options= ops;
}

void baseVRen::StartRender( int image_xdim, int image_ydim )
{
  // Check the status of all needed parts.
  if (!current_camera) {
    error( VRENERROR_CAMERA_NOT_SET );
    return;
  }
  if (!current_quality) {
    error( VRENERROR_QUALITY_NOT_SET );
    return;
  }
  if (!current_light_info) {
    error( VRENERROR_LIGHT_INFO_NOT_SET );
    return;
  }
  if (!current_geom) {
    error( VRENERROR_GEOM_NOT_SET );
    return;
  }

  // Record that we are starting the task
  logger->comment("render started\n");
  fprintf(stderr,"raycastVRen::StartRender: %d %d\n",image_xdim, image_ydim);

  fprintf(stderr,"baseVRen::StartRender called; image %d by %d \n",
	image_xdim, image_ydim );
}

void baseVRen::AbortRender()
{
  fprintf(stderr,"baseVRen::AbortRender called!\n");
}

void baseVRen::error( int error_id )
{
  if (owner) owner->error( error_id );
  else (*error_proc)(error_id, this);
}

void baseVRen::fatal( int error_id )
{
  if (owner) owner->fatal( error_id );
  else {
    (*fatal_proc)(error_id, this);
    exit(-1);
  }
}

void baseVRen::setCamera( const gPoint& lookfm, const gPoint& lookat, 
			  const gVector& up, const float fov, 
			  const float hither, const float yon,
			  const int parallel_flag ) 
{
  delete current_camera;
  current_camera= 
    new Camera( lookfm, lookat, up, fov, hither, yon, parallel_flag );
}

void baseVRen::setCamera( const Camera& cam )
{
  delete current_camera;
  current_camera= new Camera( cam );
}

void baseVRen::setQualityMeasure( const QualityMeasure& qual )
{
  delete current_quality;
  current_quality= new QualityMeasure( qual );
}

void baseVRen::setOpacLimit( float what ) { 
  if (!current_quality) current_quality= new QualityMeasure;
  current_quality->set_opacity_limit(what); 
}

void baseVRen::setColorCompError( float what ) { 
  if (!current_quality) current_quality= new QualityMeasure;
  current_quality->set_color_comp_error(what); 
};

void baseVRen::setOpacMinimum( const int what ) {
  if (!current_quality) current_quality= new QualityMeasure;
  current_quality->set_opacity_min( what );
};

void baseVRen::setLightInfo( const LightInfo& linfo_in )
{
  delete current_light_info;
  current_light_info= new LightInfo( linfo_in );
}

baseDataFile *baseVRen::create_data_file( const char* fname_in,
					  const DataFileType dftype_in )
{
  switch (dftype_in) {
#ifdef INCL_HDF
  case hdfDataFileType:
    return( new hdfDataFile( fname_in ) );
#endif
#ifdef INCL_FIASCO
  case pghMRIDataFileType:
    return( new PghMRIDataFile( fname_in ) );
#endif
  default:
    return NULL;
    break;
  }
}

DataVolume *baseVRen::create_data_volume( int xdim_in, int ydim_in, 
					  int zdim_in, 
					  const gBoundBox &bbox_in )
{
  return( new DataVolume( xdim_in, ydim_in, zdim_in, bbox_in ) );
}

baseSampleVolume *baseVRen::create_sample_volume( const GridInfo& grid_in,
						baseTransferFunction& tfun_in,
						  const int ndatavol, 
						  DataVolume** data_table ) 
{
  return( new baseSampleVolume( grid_in, tfun_in, ndatavol, data_table ) );
}

VolGob *baseVRen::create_volgob( baseSampleVolume *vol_in, 
				 const gTransfm& trans ) 
{
  return( new VolGob( vol_in, trans ) );
}

baseTransferFunction *baseVRen::register_tfun( baseTransferFunction *tfun_in )
{
  return( tfun_in->copy() );
}

void baseVRen::setGeometry( VolGob *volgob ) 
{
  delete current_geom;
  current_geom= new gobGeometry( volgob );
}

void baseVRen::update_and_go( const Camera& camera_in, 
			      const LightInfo& lights_in,
			      const int xsize, const int ysize )
{
  setCamera( camera_in );
  setLightInfo( lights_in );
  StartRender( xsize, ysize );
}
