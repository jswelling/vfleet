/****************************************************************************
 * datavolume.cc
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
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "datafile.h"

GridInfo::GridInfo()
  : boundbox()
{
  xdim= ydim= zdim= 1;
  deltax= deltay= deltaz= 1.0;
}

GridInfo::GridInfo( const int xdim_in, const int ydim_in, const int zdim_in,
		    const gBoundBox& bbox_in )
  : boundbox( bbox_in )
{
  xdim= xdim_in;
  ydim= ydim_in;
  zdim= zdim_in;

  // Sometimes this constructor is called with dims of 1, to produce
  // essentially empty volumes.  We initialize deltas, taking this
  // possibility into account.
  if (xdim>1) deltax= (boundbox.xmax() - boundbox.xmin())/xdim;
  else deltax= 1.0;
  if (ydim>1) deltay= (boundbox.ymax() - boundbox.ymin())/ydim;
  else deltay= 1.0;
  if (zdim>1) deltaz= (boundbox.zmax() - boundbox.zmin())/zdim;
  else deltaz= 1.0;
}

DataVolume::DataVolume( int xdim_in, int ydim_in, int zdim_in,
			const gBoundBox& bbox_in, DataVolume *parent_in ) 
: grid( xdim_in, ydim_in, zdim_in, bbox_in )
{
  max_grad_mag= 0.0;

  parent= parent_in;

  data= new DataType[xsize()*ysize()*zsize()];
  int i;
  for (i=0; i<xsize()*ysize()*zsize(); i++) data[i]= DataTypeMin;
}

DataVolume::~DataVolume()
{
  delete [] data;
}

void DataVolume::finish_init()
{
  // This does all the things we couldn\'t do in the constructor because
  // the data had not yet been loaded.
  
  set_max_gradient( find_max_grad_mag() );
}

float DataVolume::find_max_grad_mag()
{
  float result= 0;
  float val;

  for (int i=0; i<xsize(); i++)
    for (int j=0; j<ysize(); j++)
      for (int k=0; k<zsize(); k++) {
	val= gradient(i,j,k).lengthsqr();
	if (val>result) result= val;
      }

  return( sqrt(result) );
}

void DataVolume::set_max_gradient( const float value, DataVolume *caller )
{
  max_grad_mag= value;
  if (caller != parent) parent->set_max_gradient(value, this);
}

void DataVolume::load_xplane( const DataType *data_in, int which_x )
{
  int j, k;
  for (j=0; j<ysize(); j++)
    for (k=0; k<zsize(); k++) 
      *(access(which_x,j,k))= *data_in++;
}

void DataVolume::load_yplane( const DataType *data_in, int which_y )
{
  int k, i;
  for (i=0; i<xsize(); i++) 
    for (k=0; k<zsize(); k++)
      *(access(i,which_y,k))= *data_in++;
}

void DataVolume::load_zplane( const DataType *data_in, int which_z )
{
  int i, j;
  for (i=0; i<xsize(); i++)
    for (j=0; j<ysize(); j++) 
      *(access(i,j,which_z))= *data_in++;
}

gVector DataVolume::gradient( const int i, const int j, const int k ) const
{
  float gradx, grady, gradz;

  if ((i!=0)&&(i!=xsize()-1)) gradx= 
    deriv_centered( fval(i-1,j,k), fval(i+1,j,k), grid.dx() );
  else if (i==0) gradx= 
    deriv_forwards( fval(0,j,k), fval(1,j,k), 
		    fval(2,j,k), grid.dx() );
  else gradx=
    deriv_forwards( fval(xsize()-1,j,k), fval(xsize()-2,j,k), 
		    fval(xsize()-3,j,k), -grid.dx() );
  
  if ((j!=0)&&(j!=ysize()-1)) grady=
    deriv_centered( fval(i,j-1,k), fval(i,j+1,k), grid.dy() );
  else if (j==0) grady= 
    deriv_forwards( fval(i,0,k), fval(i,1,k), 
		    fval(i,2,k), grid.dy() );
  else grady=
    deriv_forwards( fval(i,ysize()-1,k), fval(i,ysize()-2,k), 
		    fval(i,ysize()-3,k), -grid.dy() );
  
  if ((k!=0)&&(k!=zsize()-1)) gradz= 
    deriv_centered( fval(i,j,k-1), fval(i,j,k+1), grid.dz() );
  else if (k==0) gradz= 
    deriv_forwards( fval(i,j,0), fval(i,j,1), 
		    fval(i,j,2), grid.dz() );
  else gradz=
    deriv_forwards( fval(i,j,zsize()-1), fval(i,j,zsize()-2), 
		    fval(i,j,zsize()-3), -grid.dz() );

  return( gVector( gradx, grady, gradz ) );
}

void DataVolume::load_datafile( baseDataFile* datafile )
{
  unsigned char *buf;
  for (int i=0; i<datafile->xsize(); i++) {
    buf= (unsigned char *)(datafile->get_xplane(i, baseDataFile::ByteU));
    load_xplane( buf, i );
    delete [] buf;
  }
    
  finish_init();
}
