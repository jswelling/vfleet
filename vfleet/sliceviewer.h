/****************************************************************************
 * sliceviewer.h 
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

class baseSliceViewer {
public:
  baseSliceViewer( const GridInfo& grid_in, baseTransferFunction *tfun_in, 
		   const int ndata_in, DataVolume** dtbl_in,
		   const int which_dir_in, const int manage_dvols_in= 0 );
  virtual ~baseSliceViewer();
  virtual void set_tfun( const GridInfo& grid_in, 
			 baseTransferFunction *new_tfun,
			 const int ndata_in, DataVolume** dtbl_in );
  virtual void set_plane( const int new_plane );
protected:
  GridInfo grid;
  void prepare_plane( int which_plane );
  virtual void update_image()= 0;
  gBColor calc( const int i, const int j, const int k )
  {
    Sample sample;
    tfun->apply( sample, i, j, k, dummy_svol, ndata, dtbl );
    return sample.clr;
  }
  int which_dir;
  int current_plane;
  baseTransferFunction *tfun;
  int ndata;
  DataVolume** dtbl;
  baseSampleVolume *dummy_svol;
  int image_xsize;
  int image_ysize;
  int nslices;
  int manage_dvols;
};

