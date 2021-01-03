/****************************************************************************
 * netdatafile.cc
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include "basenet.h"
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "datafile.h"
#include "netdatafile.h"

int netDataFile::initialized= 0;

char* netDataFile::param_info_buf= NULL;
int netDataFile::param_buf_size= 0;

netDataFile::netDataFile( RemObjInfo* rem_in, baseDataFile* real_dfile )
  : baseNet( rem_in ), baseDataFile( real_dfile->filename() )
{
  /*
  This is the remote copy.  We need to attend to patching up all the
  base class' member values.
  */
  if (!initialized) initialize("NoNameGiven");

  dfile= real_dfile;
  xdim= dfile->xsize();
  ydim= dfile->ysize();
  zdim= dfile->zsize();
  file_valid= dfile->valid();
  data_type= dfile->datatype();
  label= new char[strlen(dfile->data_label())+1]; 
  strcpy(label,dfile->data_label());
  unit= new char[strlen(dfile->data_unit())+1];
  strcpy(unit, dfile->data_unit());
  coordsys= new char[strlen(dfile->coordinate_system()+1)];
  strcpy(coordsys,dfile->coordinate_system());
  minval= dfile->min();
  maxval= dfile->max();
  x_plane_to_fetch= y_plane_to_fetch= z_plane_to_fetch= 0;
  keyToPassToChild= NULL;
}

netDataFile::netDataFile( const char *fname_in, DataFileType type_in, 
			  RemObjInfo* server )
  : baseNet( DATAFILE_REQDATAFILE,
	     pack_param_info( fname_in, type_in ),
	     server ),
    baseDataFile( fname_in )
{
  /* This is the local copy. */

  if (!initialized) initialize("NoNameGiven");

  dfile= NULL;

  data_type= ByteU;
  minval.byteu= 0;
  maxval.byteu= 255;
  x_plane_to_fetch= y_plane_to_fetch= z_plane_to_fetch= 0;

  // OK, we are connected.  Now request specifics on the file;
  // the results return as an ack payload.
  bufsetup();
  if (netsnd(DATAFILE_MATCH)) {
    int ibuf[4];
    netgetnint(ibuf,4);
    xdim= ibuf[0];
    ydim= ibuf[1];
    zdim= ibuf[2];
    file_valid= ibuf[3];
    delete [] label;
    label= netgetstring();
    delete [] unit;
    unit= netgetstring();
    delete [] coordsys;
    coordsys= netgetstring();
  }
}

netDataFile::~netDataFile()
{
  // Note that this was originally passed in as a parameter to the
  // constructor, so we didn\'t allocate it, but there seems no more
  // elegant way to do this.
  delete dfile;
}

DataFileType netDataFile::type() const
{
  return netDataFileType;
}

void *netDataFile::get_xplane( int which_x )
{
  if (dfile) return( dfile->get_xplane( which_x ) );
  else {
    bufsetup();
    netputnint(&which_x,1);
    if (netsnd(DATAFILE_GET_X)) {
      char* result= new char[ ydim*zdim ];
      netgetbytes( result, ydim*zdim );
      return result;
    }
    else return NULL;
  }
}

void *netDataFile::get_yplane( int which_y )
{
  if (dfile) return( dfile->get_yplane( which_y ) );
  else {
    bufsetup();
    netputnint(&which_y,1);
    if (netsnd(DATAFILE_GET_Y)) {
      char* result= new char[ zdim*xdim ];
      netgetbytes( result, zdim*xdim );
      return result;
    }
    else return NULL;
  }
}

void *netDataFile::get_zplane( int which_z )
{
  if (dfile) return( dfile->get_xplane( which_z ) );
  else {
    bufsetup();
    netputnint(&which_z,1);
    if (netsnd(DATAFILE_GET_Z)) {
      char* result= new char[ xdim*ydim ];
      netgetbytes( result, xdim*ydim );
      return result;
    }
    else return NULL;
  }
}

int netDataFile::next_dataset()
{
  if (dfile) return( dfile->next_dataset() );
  else {
    bufsetup();
    return( netsnd(DATAFILE_NEXT_DSET) );
  }
}

int netDataFile::netrcv(NetMsgType msg)
{
  int rcv_ok= 1;

  switch (msg) {

  case DATAFILE_MATCH:
    {
      // Do nothing;  data sent back as ack payload
    }
    break;

  case DATAFILE_GET_X:
    {
      netgetnint(&x_plane_to_fetch, 1);
    }
    break;

  case DATAFILE_GET_Y:
    {
      netgetnint(&y_plane_to_fetch, 1);
    }
    break;

  case DATAFILE_GET_Z:
    {
      netgetnint(&x_plane_to_fetch, 1);
    }
    break;

  case DATAFILE_NEXT_DSET:
    {
      rcv_ok= next_dataset();
    }
    break;

  case DATAFILE_OPEN_FILE:
    {
      rcv_ok= open_file();
    }
    break;

  case DATAFILE_CLOSE_FILE:
    {
      rcv_ok= close_file();
    }
    break;

  case DATAFILE_HAS_NAMED_VALUE:
    {
      delete [] keyToPassToChild;
      keyToPassToChild= netgetstring();
    }
    break;

  case DATAFILE_GET_NAMED_VALUE:
    {
      delete [] keyToPassToChild;
      keyToPassToChild= netgetstring();
    }
    break;

  case DATAFILE_GET_NAMED_VALUE_TYPE:
    {
      delete [] keyToPassToChild;
      keyToPassToChild= netgetstring();
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netDataFile::add_ack_payload( NetMsgType msg, const int netrcv_retcode )
{
  switch (msg) {
  case DATAFILE_GET_X:
    {
      char* result= (char*)dfile->get_xplane( x_plane_to_fetch, ByteU );
      netputbytes( result, ydim*zdim );
      delete [] result;
    }
    break;

  case DATAFILE_GET_Y:
    {
      char* result= (char*)dfile->get_yplane( y_plane_to_fetch, ByteU );
      netputbytes( result, zdim*xdim );
      delete [] result;
    }
    break;

  case DATAFILE_GET_Z:
    {
      char* result= (char*)dfile->get_zplane( z_plane_to_fetch, ByteU );
      netputbytes( result, xdim*ydim );
      delete [] result;
    }
    break;

  case DATAFILE_MATCH:
    {
      int ibuf[4];
      ibuf[0]= xdim;
      ibuf[1]= ydim;
      ibuf[2]= zdim;
      ibuf[3]= file_valid;
      netputnint(ibuf, 4);
      netputstring(label);
      netputstring(unit);
      netputstring(coordsys);
    }
    break;

  case DATAFILE_HAS_NAMED_VALUE:
    {
      int answer= dfile->hasNamedValue(keyToPassToChild);
      netputnint(&answer, 1);
    }
    break;

  case DATAFILE_GET_NAMED_VALUE:
    {
      DataElementType t= dfile->getNamedValueType(keyToPassToChild);
      int t_int= (int)t;
      type_union result= dfile->getNamedValue(keyToPassToChild);
      netputnint(&t_int, 1);
      switch (t) {
      case Float32:
	netputnfloat(&(result.float32),1);
	break;
      case Float64:
	netputndfloat(&(result.float64),1);
	break;
      case ByteS:
	netputbytes((char*)&(result.bytes),1);
	break;
      case ByteU:
	netputbytes((char*)&(result.byteu),1);
	break;
      case IntS16:
	netputnshort((short*)&(result.ints16),1);
	break;
      case IntU16:
	netputnshort((short*)&(result.intu16),1);
	break;
      case IntS32:
	netputnlong((long*)&(result.ints32),1);
	break;
      case IntU32:
	netputnlong((long*)&(result.intu32),1);
	break;
      }
    }
    break;

  case DATAFILE_GET_NAMED_VALUE_TYPE:
    {
      int answer= (int)dfile->getNamedValueType(keyToPassToChild);
      netputnint(&answer, 1);
    }
    break;

  default: baseNet::add_ack_payload(msg, netrcv_retcode);
  }
}

void netDataFile::initialize( const char* name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );
    
    // Add our message handler
    add_msg_handler( DATAFILE_REQDATAFILE, 
		     (void (*)())handle_datafile_request );
    
    initialized= 1;
  }
}

char* netDataFile::pack_param_info( const char* fname_in, 
				    const DataFileType type_in )
{
  if (param_buf_size < strlen(fname_in)+16) {
    delete [] param_info_buf;
    param_buf_size= strlen(fname_in)+32;
    param_info_buf= new char[param_buf_size];
  }

  sprintf(param_info_buf,"%4d <%s> %4d",
	  strlen(fname_in), fname_in, (int)type_in);

  return param_info_buf;
}

void netDataFile::unpack_param_info( char* info, char** fname_out, 
				     DataFileType& type_out )
{
  int length= atoi(info);
  *fname_out= new char[length+1];
  strncpy(*fname_out,info+6,length);
  *(fname_out+length)= '\0';
  type_out= (DataFileType)atoi(info+6+length+2);
}

void netDataFile::handle_datafile_request()
{
  // Handle incoming request for a datafile here

  RemObjInfo rem;
  char* fname;
  DataFileType dftype;
  netgetreminfo(&rem);
  char *info= netgetstring();
  netDataFile::unpack_param_info( info, &fname, dftype );
  delete [] info;
  baseDataFile *real_datafile;
  switch (dftype) {
#ifdef INCL_HDF
  case hdfDataFileType:
    real_datafile= new hdfDataFile( fname );
#endif
#ifdef INCL_FIASCO
  case pghMRIDataFileType:
    real_datafile= new PghMRIDataFile( fname );
#endif
  default:
    {
      fprintf(stderr,
"netDataFile::handle_datafile_request: Unsupported datafile type %d requested!\n",
	      (int)dftype);
      exit(-1);
    }
    break;
  }
  baseDataFile *result= new netDataFile( &rem, real_datafile );
}

int netDataFile::open_file()
{
  if (dfile) return( dfile->open_file() );
  else {
    bufsetup();
    return( netsnd(DATAFILE_OPEN_FILE) );
  }
}

int netDataFile::close_file()
{
  if (dfile) return( dfile->close_file() );
  else {
    bufsetup();
    return( netsnd(DATAFILE_CLOSE_FILE) );
  }
}

int 
netDataFile::hasNamedValue( const char* key ) {
  if (dfile) return dfile->hasNamedValue( key );
  else {
    bufsetup();
    netputstring(key);
    if (netsnd(DATAFILE_HAS_NAMED_VALUE)) {
      int val;
      netgetnint(&val,1);
      return val;
    }
    else return 0;
  }
}

baseDataFile::DataElementType 
netDataFile::getNamedValueType( const char* key ) {
  if (dfile) return dfile->getNamedValueType( key );
  else {
    bufsetup();
    netputstring(key);
    if (netsnd(DATAFILE_GET_NAMED_VALUE_TYPE)) {
      int val;
      netgetnint(&val,1);
      return (DataElementType)val;
    }
    else return InvalidDataElementType;
  }
}

union baseDataFile::type_union 
netDataFile::getNamedValue( const char* key ) {
  if (dfile) return dfile->getNamedValue( key );
  else {
    union type_union result;
    bufsetup();
    netputstring(key);
    if (netsnd(DATAFILE_GET_NAMED_VALUE_TYPE)) {
      int t_int;
      DataElementType t;
      netgetnint(&t_int,1);
      t= (DataElementType)t_int;
      switch (t) {
      case Float32:
	netgetnfloat(&(result.float32),1);
	break;
      case Float64:
	netgetndfloat(&(result.float64),1);
	break;
      case ByteS:
	netgetbytes((char*)&(result.bytes),1);
	break;
      case ByteU:
	netgetbytes((char*)&(result.byteu),1);
	break;
      case IntS16:
	netgetnshort((short*)&(result.ints16),1);
	break;
      case IntU16:
	netgetnshort((short*)&(result.intu16),1);
	break;
      case IntS32:
	netgetnlong((long*)&(result.ints32),1);
	break;
      case IntU32:
	netgetnlong((long*)&(result.intu32),1);
	break;
      }
      return result;
    }
    else {
      result.ints32= 0;
      return result;
    }
  }
}


