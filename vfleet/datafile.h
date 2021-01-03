/****************************************************************************
 * datafile.h
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

#include "basedatafile.h"

#ifdef INCL_HDF

#ifndef HDFI_H
#define PROTOTYPE
extern "C" {
#include <hdfi.h>
}
#undef PROTOTYPE
#endif

class hdfDataFile : public baseDataFile {
public:
  hdfDataFile( const char *fname_in );
  ~hdfDataFile();
  DataFileType type() const;
  void *get_xplane( int which_x );
  void *get_yplane( int which_y );
  void *get_zplane( int which_z );
  int next_dataset();
  void restart();
  const char *data_print_format() { return data_unit_format; }
protected:
  int32 sd_id;
  int32 sds_index;
  int32 sds_id;
  int32 ndatasets;
  int32 nglobal_attr;
  char *sds_name;
  int32 rank;
  int32* dimsizes;
  int32 number_type;
  int32 nattr;
  int open_file();
  int close_file();
  int next_block();
  char *data_unit_format; // to be used in printing data
};

#endif

#ifdef INCL_FIASCO

class PghMRIDataFile : public baseDataFile {
public:
  PghMRIDataFile( const char *fname_in );
  ~PghMRIDataFile();
  DataFileType type() const;
  void *get_xplane( int which_x );
  void *get_yplane( int which_y );
  void *get_zplane( int which_z );
  int next_dataset();
  void restart();
  int hasNamedValue( const char* key );
  union type_union getNamedValue( const char* key );
  DataElementType getNamedValueType( const char* key );
protected:
  struct MRI_Dataset* ds;
  long t;
  long tdim;
  void* data;
  int open_file();
  int close_file();
};

#endif
