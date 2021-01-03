/****************************************************************************
 * sliceviewer.cc
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
#ifdef ATTCC
#include <osfcn.h>
#else
#include <unistd.h>
#endif
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "tfun.h"
#include "sliceviewer.h"

baseSliceViewer::baseSliceViewer( const GridInfo& grid_in, 
				  baseTransferFunction *tfun_in,
				  const int ndata_in, 
				  DataVolume** dtbl_in,
				  const int which_dir_in,
				  const int manage_dvols_in )
  : grid(grid_in)
{
  which_dir= which_dir_in;
  tfun= tfun_in;
  ndata= ndata_in;
  manage_dvols= manage_dvols_in;

  if (ndata) {
    dtbl= new DataVolume*[ndata];
    for (int i=0; i<ndata; i++) dtbl[i]= dtbl_in[i];
  }
  else dtbl= NULL;

  switch (which_dir) {
  default: // invalid 
    fprintf(stderr,"baseSliceViewer::baseSliceViewer: invalid dir; using x\n");
    which_dir= 0;
    // Fall through to x case
  case 0: // x held constant
    {
      image_xsize= grid.ysize();
      image_ysize= grid.zsize();
      nslices= grid.xsize();
    }
    break;
  case 1: // y held constant
    {
      image_xsize= grid.zsize();
      image_ysize= grid.xsize();
      nslices= grid.ysize();
    }
    break;
  case 2: // z held constant
    {
      image_xsize= grid.xsize();
      image_ysize= grid.ysize();
      nslices= grid.zsize();
    }
    break;
  };

  current_plane= nslices/2;

  dummy_svol= new baseSampleVolume( grid, *tfun, ndata, dtbl );
}

baseSliceViewer::~baseSliceViewer()
{
  if (manage_dvols) {
    for (int i=0; i<ndata; i++) delete dtbl[i];
  }
  delete [] dtbl;
}

void baseSliceViewer::set_tfun( const GridInfo& grid_in,
				baseTransferFunction *new_tfun,
				const int ndata_in, DataVolume** dtbl_in )
{
  tfun= new_tfun;

  if (grid_in != grid) {
    fprintf(stderr,
	    "baseSliceViewer::set_dtbl: error: datavolume size mismatch!\n");
    exit(-1);
  }

  if (manage_dvols) {
    for (int i=0; i<ndata; i++) delete dtbl[i];
  }
  delete [] dtbl;

  ndata= ndata_in;

  if (ndata) {
    dtbl= new DataVolume*[ndata];
    for (int i=0; i<ndata; i++) dtbl[i]= dtbl_in[i];
  }
  else dtbl= NULL;

  delete dummy_svol;
  dummy_svol= new baseSampleVolume( grid, *tfun, ndata, dtbl );

  update_image();
}

void baseSliceViewer::set_plane( const int new_plane )
{
  current_plane= new_plane;
  update_image();
}

void baseSliceViewer::prepare_plane( int which_plane )
{
  current_plane= which_plane;
  for (int i=0; i<ndata; i++) 
    switch (which_dir) {
    case 0: 
      dtbl[i]->prep_xplane( current_plane );
      break;
    case 1: 
      dtbl[i]->prep_yplane( current_plane );
      break;
    case 2: 
      dtbl[i]->prep_zplane( current_plane );
      break;
    }
}
