/****************************************************************************
 * basedatafile.h
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

#ifndef BASEDATAFILE_H_INCLUDED

#define BASEDATAFILE_H_INCLUDED

#define BASEDATAFILE_INCL 1

enum DataFileType { 
  baseDataFileType, 
  hdfDataFileType, 
  pghMRIDataFileType,
  netDataFileType
};

class baseDataFile {
friend class netDataFile;
public:
  enum DataElementType { 
    InvalidDataElementType,
    Float32, 
    Float64, 
    ByteS, 
    ByteU, 
    IntS16,
    IntU16,
    IntS32,
    IntU32 
    };
  union type_union {
    float float32;
    double float64;
    signed char bytes;
    unsigned char byteu;
    signed short ints16;
    unsigned short intu16;
    signed long ints32;
    unsigned long intu32;
  };
  baseDataFile( const char *fname_in );
  virtual ~baseDataFile();
  const char* filename() const { return fname; }
  virtual DataFileType type() const;
  int valid() const { return file_valid; }
  int xsize() const { return xdim; }
  int ysize() const { return ydim; }
  int zsize() const { return zdim; }
  DataElementType datatype() const { return data_type; }
  virtual void *get_xplane( int which_x )= 0;
  virtual void *get_yplane( int which_y )= 0;
  virtual void *get_zplane( int which_z )= 0;
  virtual void *get_xplane( int which_x, DataElementType desired_type );
  virtual void *get_yplane( int which_y, DataElementType desired_type );
  virtual void *get_zplane( int which_z, DataElementType desired_type );
  virtual int next_dataset()= 0;
  virtual void restart() { (void)close_file(); (void)open_file(); };
  type_union min() const { return minval; }
  type_union max() const { return maxval; }
  const char *data_label() { return label; }
  const char *data_unit() { return unit; }
  const char *coordinate_system() { return coordsys; }
  virtual int hasNamedValue( const char* key ) {
    return 0;
  }
  virtual union type_union getNamedValue( const char* key ) {
    union type_union junk;
    junk.ints32= 0;
    return junk;
  }
  virtual DataElementType getNamedValueType( const char* key ) {
    return InvalidDataElementType;
  }
protected:
  virtual int open_file()= 0;
  virtual int close_file()= 0;
  int convert(void *out, DataElementType out_type,
		     void *in, DataElementType in_type,
		     int num_to_convert) const;// returns non-zero on success
  int file_valid;
  char *fname;
  int xdim;
  int ydim;
  int zdim;
  DataElementType data_type;
  type_union minval;
  type_union maxval;
  char *label; // label string for current dataset
  char *unit;  // unit string for current dataset
  char *coordsys; // coordinate system of current dataset
};

#endif // ifndef BASEDATAFILE_H_INCLUDED 
