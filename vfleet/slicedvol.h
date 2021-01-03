/****************************************************************************
 * slicedvol.h
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

class baseDataFile;

class sliceDataVolume : public DataVolume {
public:
  sliceDataVolume( baseDataFile* dfile_in, const gBoundBox& bbox_in );
  ~sliceDataVolume();
  void load_xplane( const DataType *data_in, int which_x ) {}
  void load_yplane( const DataType *data_in, int which_y ) {}
  void load_zplane( const DataType *data_in, int which_z ) {}
  void finish_init();
  DataType val( const int i, const int j, const int k ) const;
  gVector gradient( const int i, const int j, const int k ) const;
  const gBoundBox& boundbox() { return grid.bbox(); }
  virtual void prep_xplane( const int which_x );
  virtual void prep_yplane( const int which_y );
  virtual void prep_zplane( const int which_z );
protected:
  baseDataFile *file;
  int which_dir;
  int current_plane;
  int idim;
  int jdim;
  float deltax;
  float deltay;
  float deltaz;
  float flat_fval( const int i, const int j ) const
  { return( ((float)(*(data + i*jdim + j) - DataTypeMin))
	    / (DataTypeMax-DataTypeMin) ); }

};

