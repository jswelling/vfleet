/****************************************************************************
 * datafile.cc
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

#include <string.h>
#include <limits.h>
#include <stdio.h>

/* Notes:
 * -Update the damn disclaimer!
 * -Min and max are only being found over the first volume!
 */

#ifdef INCL_HDF

#define PROTOTYPE
extern "C" {
#include <hdf.h>
#include <netcdf.h>
#include <mfhdf.h>
}
#undef PROTOTYPE

#endif

#ifdef INCL_FIASCO
extern "C" {
#include <mri.h>
}
#endif

#include "vren.h"
#include "datafile.h"

// This zero value is added to the values returned from SDgetrange to
// assure that they are valid floats.  This work-around was found necessary
// on the Cray C90.  Things are defined this way to keep the compiler from
// optimizing the operation away.
static float realzero= 0.0;

baseDataFile::baseDataFile( const char *fname_in )
{
  fname= new char[strlen(fname_in)+1];
  strcpy(fname,fname_in);
  xdim= ydim= zdim= 0;
  file_valid= 0;
  data_type= InvalidDataElementType;
  minval.ints32= 0;
  maxval.ints32= 0;
  label= new char[8];
  label[0]= '\0';
  unit= new char[8];
  unit[0]= '\0';
  coordsys= new char[8];
  coordsys[0]= '\0';
}

baseDataFile::~baseDataFile()
{
  delete fname;
}

DataFileType baseDataFile::type() const
{
 return baseDataFileType;
}

#define COPY_WITHOUT_RESCALE( out, out_tp, in, in_tp, num ) \
{ \
  register in_tp *in_typed= (in_tp *)in; \
  register out_tp *out_typed= (out_tp *)out; \
  in_tp *end= in_typed + num; \
  for ( ; in_typed < end; ) *out_typed++= *in_typed++; \
}

#define COPY_WITH_RESCALE( out, out_tp, outmax, in, in_tp, max, min, num ) \
{ \
  register in_tp *in_typed= (in_tp *)in; \
  register out_tp *out_typed= (out_tp *)out; \
  for (register int i=0; i<num; i++) { \
    register in_tp inval= *in_typed++; \
    inval= (inval>max) ? max : ((inval < min) ? min : inval); \
    *out_typed++= (out_tp)(outmax*((inval) - min)/(max-min)); \
   } \
}

#define COPY_WITH_SHIFT_LEFT( out, out_tp, in, in_tp, shift, num ) \
{ \
  register in_tp *in_typed= (in_tp *)in; \
  register out_tp *out_typed= (out_tp *)out; \
  in_tp *end= in_typed + num; \
  for ( ; in_typed < end; ) \
    *out_typed++= ((out_tp)(*in_typed++)) << shift; \
}

#define COPY_WITH_SHIFT_RIGHT( out, out_tp, in, in_tp, shift, num ) \
{ \
  register in_tp *in_typed= (in_tp *)in; \
  register out_tp *out_typed= (out_tp *)out; \
  in_tp *end= in_typed + num; \
  for ( ; in_typed < end; ) \
    *out_typed++= (*in_typed++) >> shift; \
}

int baseDataFile::convert(void *out, DataElementType out_type,
			   void *in, DataElementType in_type,
			   int num_to_convert) const {

  switch (in_type) {

    case Float32:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, float, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, float, num_to_convert );
	break;
      case ByteS:
	COPY_WITH_RESCALE( out, signed char, SCHAR_MAX, 
			   in, float, max().float32, min().float32, 
			   num_to_convert );
	break;
      case ByteU:
	COPY_WITH_RESCALE( out, unsigned char, UCHAR_MAX, 
			   in, float, max().float32, min().float32, 
			   num_to_convert );
	break;
      case IntS16:
	COPY_WITH_RESCALE( out, signed short, SHRT_MAX, 
			   in, float, max().float32, min().float32, 
			   num_to_convert );
	break;
      case IntU16:
	COPY_WITH_RESCALE( out, unsigned short, USHRT_MAX, 
			   in, float, max().float32, min().float32, 
			   num_to_convert);
	break;
      case IntS32:
	COPY_WITH_RESCALE( out, signed long, LONG_MAX, 
			   in, float, max().float32, min().float32, 
			   num_to_convert );
	break;
      case IntU32:
	COPY_WITH_RESCALE( out, unsigned long, ULONG_MAX, 
			   in, float, max().float32, min().float32, 
			   num_to_convert );
	break;
      default: 
	return(0); // cannot handle these cases
	break;
      }
      break;

    case Float64:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, double, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, double, num_to_convert );
	break;
      case ByteS:
	COPY_WITH_RESCALE( out, signed char, SCHAR_MAX, 
			   in, double, max().float64, min().float64, 
			   num_to_convert );
	break;
      case ByteU:
	COPY_WITH_RESCALE( out, unsigned char, UCHAR_MAX, 
			   in, double, max().float64, min().float64, 
			   num_to_convert );
	break;
      case IntS16:
	COPY_WITH_RESCALE( out, signed short, SHRT_MAX, 
			   in, double, max().float64, min().float64, 
			   num_to_convert );
	break;
      case IntU16:
	COPY_WITH_RESCALE( out, unsigned short, USHRT_MAX, 
			   in, double, max().float64, min().float64, 
			   num_to_convert);
	break;
      case IntS32:
	COPY_WITH_RESCALE( out, signed long, LONG_MAX, 
			   in, double, max().float64, min().float64, 
			   num_to_convert );
	break;
      case IntU32:
	COPY_WITH_RESCALE( out, unsigned long, ULONG_MAX, 
			   in, double, max().float64, min().float64, 
			   num_to_convert );
	break;
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    case ByteS:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, signed char, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, signed char, num_to_convert );
	break;
      case ByteS:
	COPY_WITHOUT_RESCALE( out, signed char,
			      in, signed char, num_to_convert );
	break;
      case IntS16:
	COPY_WITH_SHIFT_LEFT( out, signed short,
			      in, signed char, 8, num_to_convert );
	break;
      case IntS32:
	COPY_WITH_SHIFT_LEFT( out, signed long,
			      in, signed char, 24, num_to_convert );
	break;
      case ByteU:
	COPY_WITHOUT_RESCALE( out, unsigned char, in, unsigned char, 
			      num_to_convert ); // cast them to unsigned
	break;
      case IntU16:
      case IntU32:
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    case ByteU:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, unsigned char, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, unsigned char, num_to_convert );
	break;
      case ByteU:
	COPY_WITHOUT_RESCALE( out, unsigned char,
			      in, unsigned char, num_to_convert );
	break;
      case IntU16:
	COPY_WITH_SHIFT_LEFT( out, unsigned short,
			      in, unsigned char, 8, num_to_convert );
	break;
      case IntU32:
	COPY_WITH_SHIFT_LEFT( out, unsigned long,
			      in, unsigned char, 24, num_to_convert );
	break;
      case ByteS:
      case IntS16:
      case IntS32:
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    case IntS16:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, signed short, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, signed short, num_to_convert );
	break;
      case ByteS:
	COPY_WITH_SHIFT_RIGHT( out, signed char,
			      in, signed short, 8, num_to_convert );
	break;
      case ByteU:
	COPY_WITH_RESCALE( out, unsigned char, UCHAR_MAX, 
			   in, signed short, max().ints16, min().ints16, 
			   num_to_convert );
	break;
      case IntS16:
	COPY_WITHOUT_RESCALE( out, signed short,
			      in, signed short, num_to_convert );
	break;
      case IntS32:
	COPY_WITH_SHIFT_LEFT( out, signed long,
			      in, signed short, 16, num_to_convert );
	break;
      case IntU16:
      case IntU32:
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    case IntU16:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, unsigned short, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, unsigned short, num_to_convert );
	break;
      case ByteU:
	COPY_WITH_SHIFT_RIGHT( out, unsigned char,
			       in, unsigned short, 8, num_to_convert );
	break;
      case IntU16:
	COPY_WITHOUT_RESCALE( out, unsigned short,
			      in, unsigned short, num_to_convert );
	break;
      case IntU32:
	COPY_WITH_SHIFT_LEFT( out, unsigned long,
			      in, unsigned short, 16, num_to_convert );
	break;
      case ByteS:
      case IntS16:
      case IntS32:
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    case IntS32:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, signed long, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, signed long, num_to_convert );
	break;
      case ByteS:
	COPY_WITH_SHIFT_RIGHT( out, signed char,
			      in, signed long, 24, num_to_convert );
	break;
      case ByteU:
	COPY_WITH_RESCALE( out, unsigned char, UCHAR_MAX, 
			   in, signed long, max().ints32, min().ints32, 
			   num_to_convert );
      case IntS16:
	COPY_WITH_SHIFT_RIGHT( out, signed short,
			      in, signed long, 16, num_to_convert );
	break;
      case IntS32:
	COPY_WITHOUT_RESCALE( out, signed char,
			      in, signed long, num_to_convert );
	break;
      case IntU16:
      case IntU32:
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    case IntU32:
      switch (out_type) {
      case Float32:
	COPY_WITHOUT_RESCALE( out, float, 
			      in, unsigned long, num_to_convert );
	break;
      case Float64:
	COPY_WITHOUT_RESCALE( out, double, 
			      in, unsigned long, num_to_convert );
	break;
      case ByteU:
	COPY_WITH_SHIFT_RIGHT( out, unsigned char,
			      in, unsigned long, 24, num_to_convert );
	break;
      case IntU16:
	COPY_WITH_SHIFT_RIGHT( out, unsigned short,
			      in, unsigned long, 16, num_to_convert );
	break;
      case IntU32:
	COPY_WITHOUT_RESCALE( out, unsigned long,
			      in, unsigned long, num_to_convert );
	break;
      case ByteS:
      case IntS16:
      case IntS32:
      default:
	return(0); // cannot handle these cases
	break;
      }
      break;

    default: // Do nothing
      break;    
  }

  return(1);
}

#undef COPY_WITHOUT_RESCALE
#undef COPY_WITH_RESCALE
#undef COPY_WITH_SHIFT_LEFT
#undef COPY_WITH_SHIFT_RIGHT

void *baseDataFile::get_xplane( int which_x, DataElementType desired_type )
{
  void *raw= get_xplane( which_x ); 
  if (!raw) {
    fprintf(stderr,"baseDataFile::get_xplane: failed to get raw data!\n");
    return( NULL );
  }

  if (desired_type != datatype()) {
    // We have to convert

    void *result;
    int size= ysize()*zsize();

    switch (desired_type) {
    case Float32:
      result= (void *)(new float[size]); 
      break;
    case Float64:
      result= (void *)(new double[size]); 
      break;
    case ByteS:
      result= (void *)(new signed char[size]); 
      break;
    case ByteU:
      result= (void *)(new unsigned char[size]); 
      break;
    case IntS16:
      result= (void *)(new signed short[size]); 
      break;
    case IntU16:
      result= (void *)(new unsigned short[size]); 
      break;
    case IntS32:
      result= (void *)(new signed long[size]); 
      break;
    case IntU32:
      result= (void *)(new unsigned long[size]); 
      break;
    default:
      return(NULL);
      break;
    }

    if ( convert(result, desired_type, raw, datatype(), size) ) {
      switch (datatype()) {
      case Float32: delete((float*)raw); break;
      case Float64: delete((double*)raw); break;
      case ByteS: delete((signed char*)raw); break;
      case ByteU: delete((unsigned char*)raw); break;
      case IntS16: delete((signed short*)raw); break;
      case IntU16: delete((unsigned short*)raw); break;
      case IntS32: delete((signed long*)raw); break;
      case IntU32: delete((unsigned long*)raw); break;
      }
      return( result );
    }
    else { // must have been unable to convert
      switch (datatype()) {
      case Float32: delete((float*)raw); break;
      case Float64: delete((double*)raw); break;
      case ByteS: delete((signed char*)raw); break;
      case ByteU: delete((unsigned char*)raw); break;
      case IntS16: delete((signed short*)raw); break;
      case IntU16: delete((unsigned short*)raw); break;
      case IntS32: delete((signed long*)raw); break;
      case IntU32: delete((unsigned long*)raw); break;
      }
      switch (desired_type) {
      case Float32: delete((float*)result); break;
      case Float64: delete((double*)result); break;
      case ByteS: delete((signed char*)result); break;
      case ByteU: delete((unsigned char*)result); break;
      case IntS16: delete((signed short*)result); break;
      case IntU16: delete((unsigned short*)result); break;
      case IntS32: delete((signed long*)result); break;
      case IntU32: delete((unsigned long*)result); break;
      }
      return( NULL );
    }
  }

  return( raw ); // no conversion was necessary
}

void *baseDataFile::get_yplane( int which_y, DataElementType desired_type )
{
  void *raw= get_yplane( which_y ); 
  if (!raw) {
    fprintf(stderr,"baseDataFile::get_yplane: failed to get raw data!\n");
    return( NULL );
  }

  if (desired_type != datatype()) {
    // We have to convert

    void *result;
    int size= xsize()*zsize();

    switch (desired_type) {
    case Float32:
      result= (void *)(new float[size]); 
      break;
    case Float64:
      result= (void *)(new double[size]); 
      break;
    case ByteS:
      result= (void *)(new signed char[size]); 
      break;
    case ByteU:
      result= (void *)(new unsigned char[size]); 
      break;
    case IntS16:
      result= (void *)(new signed short[size]); 
      break;
    case IntU16:
      result= (void *)(new unsigned short[size]); 
      break;
    case IntS32:
      result= (void *)(new signed long[size]); 
      break;
    case IntU32:
      result= (void *)(new unsigned long[size]); 
      break;
    default:
      switch (datatype()) {
      case Float32: delete((float*)raw); break;
      case Float64: delete((double*)raw); break;
      case ByteS: delete((signed char*)raw); break;
      case ByteU: delete((unsigned char*)raw); break;
      case IntS16: delete((signed short*)raw); break;
      case IntU16: delete((unsigned short*)raw); break;
      case IntS32: delete((signed long*)raw); break;
      case IntU32: delete((unsigned long*)raw); break;
      }
      return(NULL);
      break;
    }

    if ( convert(result, desired_type, raw, datatype(), size) ) {
      switch (datatype()) {
      case Float32: delete((float*)raw); break;
      case Float64: delete((double*)raw); break;
      case ByteS: delete((signed char*)raw); break;
      case ByteU: delete((unsigned char*)raw); break;
      case IntS16: delete((signed short*)raw); break;
      case IntU16: delete((unsigned short*)raw); break;
      case IntS32: delete((signed long*)raw); break;
      case IntU32: delete((unsigned long*)raw); break;
      }
      return( result );
    }
    else { // must have been unable to convert
      switch (datatype()) {
      case Float32: delete((float*)raw); break;
      case Float64: delete((double*)raw); break;
      case ByteS: delete((signed char*)raw); break;
      case ByteU: delete((unsigned char*)raw); break;
      case IntS16: delete((signed short*)raw); break;
      case IntU16: delete((unsigned short*)raw); break;
      case IntS32: delete((signed long*)raw); break;
      case IntU32: delete((unsigned long*)raw); break;
      }
      switch (desired_type) {
      case Float32: delete((float*)result); break;
      case Float64: delete((double*)result); break;
      case ByteS: delete((signed char*)result); break;
      case ByteU: delete((unsigned char*)result); break;
      case IntS16: delete((signed short*)result); break;
      case IntU16: delete((unsigned short*)result); break;
      case IntS32: delete((signed long*)result); break;
      case IntU32: delete((unsigned long*)result); break;
      }
      return( NULL );
    }
  }

  return( raw ); // no conversion was necessary
}

void *baseDataFile::get_zplane( int which_z, DataElementType desired_type )
{
  void *raw= get_zplane( which_z ); 
  if (!raw) {
    fprintf(stderr,"baseDataFile::get_zplane: failed to get raw data!\n");
    return( NULL );
  }

  if (desired_type != datatype()) {
    // We have to convert
    int size= xsize()*ysize();

    switch (desired_type) {
    case Float32: 
      {
	float* result= new float[size];
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (float*)raw;
	  return result;
	}
	else {
	  delete (float*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case Float64:
      {
	double* result= new double[size];
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (double*)raw;
	  return result;
	}
	else {
	  delete (double*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case ByteS:
      {
	signed char* result= new signed char[size];
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (signed char*)raw;
	  return result;
	}
	else {
	  delete (signed char*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case ByteU:
      {
	unsigned char* result= new unsigned char[size];
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (unsigned char*)raw;
	  return result;
	}
	else {
	  delete (unsigned char*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case IntS16:
      {
	signed short* result= new signed short[size]; 
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (signed short*)raw;
	  return result;
	}
	else {
	  delete (signed short*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case IntU16:
      {
	unsigned short* result= new unsigned short[size];
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (unsigned short*)raw;
	  return result;
	}
	else {
	  delete (unsigned short*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case IntS32:
      {
	signed long *result= new signed long[size]; 
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (signed long*)raw;
	  return result;
	}
	else {
	  delete (signed long*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    case IntU32:
      {
	unsigned long *result= new unsigned long[size];
	if (convert(result, desired_type, raw, datatype(), size)) {
	  delete (unsigned long*)raw;
	  return result;
	}
	else {
	  delete (unsigned long*)raw;
	  delete[] result;
	  return NULL;
	}
      }
      break;
    }
  }
  else
    return( raw ); // no conversion was necessary
}

#ifdef INCL_HDF

hdfDataFile::hdfDataFile( const char *fname_in )
: baseDataFile(fname_in)
{
  data_unit_format= new char[8];
  data_unit_format[0]= '\0';
  sds_name= new char[MAX_NC_NAME];
  dimsizes= new int32[MAX_VAR_DIMS];
  sds_id= -1; // invalid, to be reset shortly
  (void)open_file();
}

hdfDataFile::~hdfDataFile()
{
  (void)close_file();
  delete [] sds_name;
  delete [] dimsizes;
  delete [] data_unit_format;
}

DataFileType hdfDataFile::type() const 
{ 
  return hdfDataFileType; 
}

void *hdfDataFile::get_xplane( int which_x )
{
  int32 start[3], edge[3];
  start[0]= which_x;
  start[1]= start[2]= 0;
  edge[0]= 1;
  edge[1]= ysize();
  edge[2]= zsize();
  void *result= NULL;
  
  switch (datatype()) {
  case Float32: 
    result= new float[ysize()*zsize()]; break;
  case Float64:
    result= new double[ysize()*zsize()]; break;
  case ByteS:
    result= new signed char[ysize()*zsize()]; break;
  case ByteU:
    result= new unsigned char[ysize()*zsize()]; break;
  case IntS16:
    result= new signed short[ysize()*zsize()]; break;
  case IntU16:
    result= new unsigned short[ysize()*zsize()]; break;
  case IntS32:
    result= new signed long[ysize()*zsize()]; break;
  case IntU32:
    result= new unsigned long[ysize()*zsize()]; break;
  default:
    return(NULL);
    break;
  }

  if (SDreaddata(sds_id, start, NULL, edge, (VOIDP)result) == FAIL) {
    // fetch failed
    delete result;
    return(NULL);
  }

  return(result);
}

void *hdfDataFile::get_yplane( int which_y )
{
  int32 start[3], edge[3];
  start[1]= which_y;
  start[0]= start[2]= 0;
  edge[0]= xsize();
  edge[1]= 1;
  edge[2]= zsize();
  void *result= NULL;
  
  switch (datatype()) {
  case Float32: 
    result= new float[xsize()*zsize()]; break;
  case Float64:
    result= new double[xsize()*zsize()]; break;
  case ByteS:
    result= new signed char[xsize()*zsize()]; break;
  case ByteU:
    result= new unsigned char[xsize()*zsize()]; break;
  case IntS16:
    result= new signed short[xsize()*zsize()]; break;
  case IntU16:
    result= new unsigned short[xsize()*zsize()]; break;
  case IntS32:
    result= new signed long[xsize()*zsize()]; break;
  case IntU32:
    result= new unsigned long[xsize()*zsize()]; break;
  default:
    return(NULL);
    break;
  }

  if (SDreaddata(sds_id, start, NULL, edge, (VOIDP)result) == FAIL) {
    // fetch failed
    delete result;
    return(NULL);
  }

  return(result);
}

void *hdfDataFile::get_zplane( int which_z )
{
  int32 start[3], edge[3];
  start[2]= which_z;
  start[0]= start[1]= 0;
  edge[0]= xsize();
  edge[1]= ysize();
  edge[2]= 1;
  void *result= NULL;
  
  switch (datatype()) {
  case Float32: 
    result= new float[xsize()*ysize()]; break;
  case Float64:
    result= new double[xsize()*ysize()]; break;
  case ByteS:
    result= new signed char[xsize()*ysize()]; break;
  case ByteU:
    result= new unsigned char[xsize()*ysize()]; break;
  case IntS16:
    result= new signed short[xsize()*ysize()]; break;
  case IntU16:
    result= new unsigned short[xsize()*ysize()]; break;
  case IntS32:
    result= new signed long[xsize()*ysize()]; break;
  case IntU32:
    result= new unsigned long[xsize()*ysize()]; break;
  default:
    return(NULL);
    break;
  }

  if (SDreaddata(sds_id, start, NULL, edge, (VOIDP)result) == FAIL) {
    // fetch failed
    delete result;
    return(NULL);
  }

  return(result);
}

int hdfDataFile::next_dataset()
{
  return( next_block() );
}

void hdfDataFile::restart()
{
  sds_index= 0;
  (void)next_block();
}

int hdfDataFile::open_file()
{
  sd_id= SDstart( fname, DFACC_RDONLY );
  sds_index= -1;
  if (sd_id == FAIL) return 0;
  if ( SDfileinfo(sd_id, &ndatasets, &nglobal_attr) == FAIL ) return 0;
  else return(next_block());
}

int hdfDataFile::close_file()
{
  if (SDendaccess(sds_id) == FAIL) return 0;
  if (SDend(sd_id)==FAIL) return 0;
  else return 1;
}

int hdfDataFile::next_block()
{
  file_valid= 0;

  sds_index++;
  if (sds_index >= ndatasets) {
    sds_index--;
    return 0;
  }

  if (sds_id != -1) (void)SDendaccess(sds_id);
  do { sds_id=SDselect( sd_id, sds_index++ ); }  
  while (SDiscoordvar(sds_id)==TRUE);
  sds_index--;

  if ( SDgetinfo( sds_id, sds_name, &rank, dimsizes, &number_type, &nattr )
       == FAIL ) return 0;

  if (rank != 3) return(0);

  file_valid= 1;
  xdim= dimsizes[0];
  ydim= dimsizes[1];
  zdim= dimsizes[2];

  switch (number_type) {
  case DFNT_FLOAT32: data_type= Float32;
    break;
  case DFNT_FLOAT64: data_type= Float64;
    break;
  case DFNT_INT8: data_type= ByteS;
    break;
  case DFNT_UINT8: data_type= ByteU;
    break;
  case DFNT_INT16: data_type= IntS16;
    break;
  case DFNT_UINT16: data_type= IntU16;
    break;
  case DFNT_INT32: data_type= IntS32;
    break;
  case DFNT_UINT32: data_type= IntU32;
    break;
  default: data_type= InvalidDataElementType;
  }

  // Get data and format strings if available
  delete [] label;
  label= new char[256];
  delete [] unit;
  unit= new char[256];
  delete [] coordsys;
  coordsys= new char[256];
  delete [] data_unit_format;
  data_unit_format= new char[256];
  if (SDgetdatastrs(sds_id, label, unit, data_unit_format, coordsys, 255) 
      == FAIL) {
    // No info given
    label[0]= '\0';
    unit[0]= '\0';
    coordsys[0]= '\0';
    data_unit_format[0]= '\0';
  }
  
  // Get the range
  if (SDgetrange(sds_id, (VOIDP)&maxval, (VOIDP)&minval) == FAIL) {
    // Call failed; need to calculate our own range.

#define FIND_MIN_MAX( typename, accessor ) \
{ \
  typename *sheet; \
  register typename min_reg, max_reg; \
  int first_pass= 1; \
  for (int i=0; i<xsize(); i++) { \
    if (sheet= (typename *)get_xplane(i)) { \
      if (first_pass) { \
	first_pass= 0; \
	min_reg= max_reg= *sheet; \
      } \
      for (int j=0; j<ysize()*zsize(); j++) { \
	if (*(sheet+j)<min_reg) min_reg= *(sheet+j); \
	if (*(sheet+j)>max_reg) max_reg= *(sheet+j); \
      } \
      delete sheet; \
    } \
  } \
  minval.accessor= min_reg; \
  maxval.accessor= max_reg; \
}

    switch (datatype()) {
    case Float32: 
      FIND_MIN_MAX( float, float32 );
      break;
    case Float64:
      FIND_MIN_MAX( double, float64 );
      break;
    case ByteS:
      FIND_MIN_MAX( unsigned char, bytes );
      break;
    case ByteU:
      FIND_MIN_MAX( signed char, byteu );
      break;
    case IntS16:
      FIND_MIN_MAX( unsigned short, ints16 );
      break;
    case IntU16:
      FIND_MIN_MAX( signed short, intu16 );
      break;
    case IntS32:
      FIND_MIN_MAX( unsigned long, intu32 );
      break;
    case IntU32:
      FIND_MIN_MAX( signed long, ints32 );
      break;
    default: // Do nothing
      break;
    }

#undef FIND_MIN_MAX

  }

  // The following senseless bit is here to deal with a possible problem
  // in which SDgetrange() returns values that are not valid floats.  This
  // has been seen to happen on Cray C90 systems.
  switch (datatype()) {
  case Float32:
    minval.float32 += realzero;
    maxval.float32 += realzero;
    break;
  case Float64:
    minval.float64 += realzero;
    maxval.float64 += realzero;
    break;
  }

  return( 1 );
}

#endif /* ifdef INCL_HDF */

#ifdef INCL_FIASCO

static void pghMRIErrorMsg(const char* s)
{
  fprintf(stderr,"%s",s);
}

PghMRIDataFile::PghMRIDataFile( const char *fname_in )
: baseDataFile(fname_in)
{
  open_file();
}

PghMRIDataFile::~PghMRIDataFile()
{
  mri_close_dataset(ds);
}

int PghMRIDataFile::open_file()
{
  ds= mri_open_dataset(fname,MRI_READ);
  if (!ds) {
    pghMRIErrorMsg("pghmri_cannot_open_msg");
    return 0;
  }
  
  if (!mri_has(ds,"images.dimensions")) {
    pghMRIErrorMsg( "pghmri_no_images_chunk_msg" );
    mri_close_dataset(ds);
    return 0;
  }
  const char* dimstr= mri_get_string(ds,"images.dimensions");
  if (*dimstr=='v') {
    if (mri_has(ds,"images.extent.v") 
	&& mri_get_int(ds,"images.extent.v") == 1) 
      dimstr++;
    else {
      pghMRIErrorMsg( "pghmri_bad_vdim_msg" );
      mri_close_dataset(ds);
      return 0;
    }
  }
  if (strcmp(dimstr,"xyz") && strcmp(dimstr,"xyzt")) {
    pghMRIErrorMsg( "pghmri_bad_dimensions_msg" );
    mri_close_dataset(ds);
    return 0;
  }

  if (!mri_has(ds,"images.extent.x") || !mri_has(ds,"images.extent.y")
      || !mri_has(ds,"images.extent.z")) {
    pghMRIErrorMsg( "pghmri_cannot_get_dims_msg" );
    mri_close_dataset(ds);
    return 0;
  }
  xdim= mri_get_int(ds,"images.extent.x");
  ydim= mri_get_int(ds,"images.extent.y");
  zdim= mri_get_int(ds,"images.extent.z");
  if (mri_has(ds,"images.extent.t")) 
    tdim= mri_get_int(ds,"images.extent.t");
  else 
    tdim= 1;
  t= -1; /* soon to be updated */

  if (!mri_has(ds,"images.datatype")) {
    pghMRIErrorMsg( "pghmri_cannot_get_datatype_msg" );
    mri_close_dataset(ds);
    return 0;
  }
  const char* typestr= mri_get_string(ds,"images.datatype");
  if (!strcmp(typestr,"uint8")) data_type= ByteU;
  else if (!strcmp(typestr,"int16")) data_type= IntS16;
  else if (!strcmp(typestr,"int32")) data_type= IntS32;
  else if (!strcmp(typestr,"float32")) data_type= Float32;
  else if (!strcmp(typestr,"float64")) data_type= Float64;
  else data_type= InvalidDataElementType;

  data= NULL;

  if (data_type!=InvalidDataElementType && next_dataset()) {
    /* t and data* are now valid */

#define FIND_MIN_MAX( typename, accessor ) \
{ \
  typename *val= (typename*)data; \
  register typename min_reg= *val, max_reg= *val; \
  int first_pass= 1; \
  val++; \
  while ( val-(typename*)data < xsize()*ysize()*zsize() ) { \
    if (*val < min_reg) min_reg= *val; \
    if (*val > max_reg) max_reg= *val; \
    val++; \
  } \
  minval.accessor= min_reg; \
  maxval.accessor= max_reg; \
}

    switch (datatype()) {
    case Float32: 
      FIND_MIN_MAX( float, float32 );
      break;
    case Float64:
      FIND_MIN_MAX( double, float64 );
      break;
    case ByteS:
      FIND_MIN_MAX( unsigned char, bytes );
      break;
    case ByteU:
      FIND_MIN_MAX( signed char, byteu );
      break;
    case IntS16:
      FIND_MIN_MAX( unsigned short, ints16 );
      break;
    case IntU16:
      FIND_MIN_MAX( signed short, intu16 );
      break;
    case IntS32:
      FIND_MIN_MAX( unsigned long, intu32 );
      break;
    case IntU32:
      FIND_MIN_MAX( signed long, ints32 );
      break;
    default: // Do nothing
      break;
    }

    file_valid= (data_type!=InvalidDataElementType);
  }
  else file_valid= 0;

}

int PghMRIDataFile::close_file()
{
  mri_close_dataset(ds);
}

DataFileType PghMRIDataFile::type() const 
{ 
  return pghMRIDataFileType; 
}

int PghMRIDataFile::next_dataset() 
{

  if (t<tdim-1) {
    long blksize= xsize()*ysize()*zsize();
    int mriType;
    t++;
    switch (datatype()) {
    case Float32: mriType= MRI_FLOAT; break;
    case Float64: mriType= MRI_DOUBLE; break;
    case ByteS: 
    case ByteU: mriType= MRI_UNSIGNED_CHAR; break;
    case IntS16: 
    case IntU16: mriType= MRI_SHORT; break;
    case IntS32:
    case IntU32: mriType= MRI_INT; break;
    default:
      pghMRIErrorMsg("PghMRIDataFile::next_dataset(): Unknown datatype!\n");
    break;
    }
    data= mri_get_chunk( ds, "images", blksize, t*blksize, mriType );
    return 1;
  }
  else return 0;
}

void PghMRIDataFile::restart()
{
  t= 0;
}

void *PghMRIDataFile::get_xplane( int which_x )
{
  void *result= NULL;
  long stride= xsize();
  long offset= which_x;
  long i;
  long j;
  long k;

  switch (datatype()) {
  case Float32: 
    result= new float[ysize()*zsize()]; 
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((float*)result)[jbar*zsize()+k]= ((float*)data)[offset];
	offset += stride;
      }
    }
    break;
  case Float64:
    result= new double[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((double*)result)[jbar*zsize()+k]= ((double*)data)[offset];
	offset += stride;
      }
    }
    break;
  case ByteS:
    result= new signed char[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((signed char*)result)[jbar*zsize()+k]= ((signed char*)data)[offset];
	offset += stride;
      }
    }
    break;
  case ByteU:
    result= new unsigned char[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((unsigned char*)result)[jbar*zsize()+k]= ((unsigned char*)data)[offset];
	offset += stride;
      }
    }
    break;
  case IntS16:
    result= new signed short[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((signed short*)result)[jbar*zsize()+k]= ((signed short*)data)[offset];
	offset += stride;
      }
    }
    break;
  case IntU16:
    result= new unsigned short[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((unsigned short*)result)[jbar*zsize()+k]= ((unsigned short*)data)[offset];
	offset += stride;
      }
    }
    break;
  case IntS32:
    result= new signed long[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((signed long*)result)[jbar*zsize()+k]= ((signed long*)data)[offset];
	offset += stride;
      }
    }
    break;
  case IntU32:
    result= new unsigned long[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((unsigned long*)result)[jbar*zsize()+k]= ((unsigned long*)data)[offset];
	offset += stride;
      }
    }
    break;
  default:
    return(NULL);
    break;
  }

  return(result);
}

void *PghMRIDataFile::get_yplane( int which_y )
{
  void *result= NULL;
  long i;
  long j;
  long k;
  long which_ybar= ysize()-(which_y+1);

  switch (datatype()) {
  case Float32: 
    result= new float[zsize()*xsize()]; 
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((float*)result)[k*xsize()+i]= 
	  ((float*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case Float64:
    result= new double[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((double*)result)[k*xsize()+i]= 
	  ((double*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case ByteS:
    result= new signed char[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((signed char*)result)[k*xsize()+i]= 
	  ((signed char*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case ByteU:
    result= new unsigned char[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((unsigned char*)result)[k*xsize()+i]= 
	  ((unsigned char*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case IntS16:
    result= new signed short[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((signed short*)result)[k*xsize()+i]= 
	  ((signed short*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case IntU16:
    result= new unsigned short[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((unsigned short*)result)[k*xsize()+i]= 
	  ((unsigned short*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case IntS32:
    result= new signed long[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((signed long*)result)[k*xsize()+i]= 
	  ((signed long*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  case IntU32:
    result= new unsigned long[ysize()*zsize()];
    for (k=0; k<zsize(); k++) {
      for (i=0; i<xsize(); i++) {
	((unsigned long*)result)[k*xsize()+i]= 
	  ((unsigned long*)data)[(((k*ysize())+which_ybar)*xsize())+i];
      }
    }
    break;
  default:
    return(NULL);
    break;
  }

  return(result);
}

void *PghMRIDataFile::get_zplane( int which_z )
{
  void *result= NULL;
  long i;
  long j;
  long k;

  switch (datatype()) {
  case Float32: 
    result= new float[zsize()*xsize()]; 
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((float*)result)[i*ysize()+j]= 
	  ((float*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case Float64:
    result= new double[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((double*)result)[i*ysize()+j]= 
	  ((double*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case ByteS:
    result= new signed char[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((signed char*)result)[i*ysize()+j]= 
	  ((signed char*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case ByteU:
    result= new unsigned char[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((unsigned char*)result)[i*ysize()+j]= 
	  ((unsigned char*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case IntS16:
    result= new signed short[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((signed short*)result)[i*ysize()+j]= 
	  ((signed short*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case IntU16:
    result= new unsigned short[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((unsigned short*)result)[i*ysize()+j]= 
	  ((unsigned short*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case IntS32:
    result= new signed long[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((signed long*)result)[i*ysize()+j]= 
	  ((signed long*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  case IntU32:
    result= new unsigned long[ysize()*zsize()];
    for (i=0; i<xsize(); i++) {
      for (j=0; j<ysize(); j++) {
	long jbar= ysize()-(j+1);
	((unsigned long*)result)[i*ysize()+j]= 
	  ((unsigned long*)data)[(((which_z*ysize())+jbar)*xsize())+i];
      }
    }
    break;
  default:
    return(NULL);
    break;
  }

}

int 
PghMRIDataFile::hasNamedValue( const char* key ) {
  char keybuf[256];
  snprintf(keybuf,sizeof(keybuf)-1,"images.%s",key);
  keybuf[sizeof(keybuf)-1]= '\0';
  return mri_has(ds,keybuf);
}

union baseDataFile::type_union 
PghMRIDataFile::getNamedValue( const char* key ) {
  union type_union junk;
  char keybuf[256];
  snprintf(keybuf,sizeof(keybuf)-1,"images.%s",key);
  keybuf[sizeof(keybuf)-1]= '\0';
  junk.float32= mri_get_float(ds,keybuf);
  return junk;
}

baseDataFile::DataElementType 
PghMRIDataFile::getNamedValueType( const char* key ) {
  return Float32;
}


#endif /* ifdef INCL_FIASCO */
