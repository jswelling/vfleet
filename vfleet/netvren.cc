/****************************************************************************
 * netvren.cc
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
#include <unistd.h>
#include "basenet.h"
#include "logger.h"
#include "netlogger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "netimagehandler.h"
#include "netdatafile.h"
#include "vren.h"
#include "tfun.h"
#include "netvren.h"
#ifndef NO_REAL_VREN
#include "raycastvren.h"
#include "bcompvren.h"
#endif

const char VRenServerExeName[]="vrenserver";

int netDataVolume::initialized= 0;

char netDataVolume::param_info_buf[ datavolume_param_buf_size ];

netDataVolume::netDataVolume( RemObjInfo *rem_in, DataVolume *real_dvol )
: baseNet( rem_in ), DataVolume( 1, 1, real_dvol->zsize(), 
				 real_dvol->boundbox(), NULL )
{
  /*
  This is the remote copy.  By constructing the base class with
  1 for xdim and ydim and resetting them to 0 below we assure 
  that the inline val() method will produce a valid (if pointless) 
  result without wasting much memory.
  */

  if (!initialized) initialize("NoNameGiven");

  dvol= real_dvol; // This is where the data is actually stored
  dvol->set_parent(this); // So it can call back
  real_xdim= dvol->xsize();
  real_ydim= dvol->ysize();
  real_zdim= dvol->zsize();
  grid= GridInfo(0,0,0,real_dvol->boundbox());
}

netDataVolume::netDataVolume( int xdim_in, int ydim_in, int zdim_in, 
			      const gBoundBox& bbox_in, DataVolume *parent_in,
			      RemObjInfo *server )
: baseNet( DATAVOLUME_REQDATAVOLUME, 
	   pack_param_info( xdim_in, ydim_in, zdim_in, bbox_in ),
	   server ), 
  DataVolume( 1, 1, zdim_in, bbox_in, parent_in )
{
  /*
  This is the local copy.  By constructing the base class with
  1 for xdim and ydim and resetting them to 0 below we assure 
  that the inline val() method will produce a valid (if pointless) 
  result without wasting much memory.
  */

  if (!initialized) initialize("NoNameGiven");

  dvol= NULL;
  real_xdim= xdim_in;
  real_ydim= ydim_in;
  real_zdim= zdim_in;
  grid= GridInfo(0,0,0,bbox_in);
}

netDataVolume::~netDataVolume()
{
  // Note that this was originally passed in as a parameter to the
  // constructor, so we didn\'t allocate it, but there seems no more
  // elegant way to do this.
  delete dvol;
}

void netDataVolume::finish_init()
{
  if (dvol) {
    max_grad_mag= 0.0;
    dvol->finish_init();
    // This DataVolume is the parent of that one, so it will call back
    // when through
  }
  else {
    max_grad_mag= 0.0; // updated by returning DATAVOLUME_MAX_GRAD message
    bufsetup();
    netsnd(DATAVOLUME_FINISH_INIT);
  }
}

void netDataVolume::load_datafile( baseDataFile* dfile_in )
{
  if (dvol) dvol->load_datafile( dfile_in );
  else {
    // Set the partner\'s copy.  Cast works as long as this renderer
    // created the volgob.
    if (dfile_in->type() == netDataFileType) {
      bufsetup();
      ((netDataFile*)dfile_in)->netputremoteobj();
      netsnd(DATAVOLUME_LOAD_REMOTE);
    }
    else DataVolume::load_datafile( dfile_in );
  }
}

void netDataVolume::set_max_gradient(const float value, DataVolume* caller )
{
  // Set value and tell partner
  max_grad_mag= value;
  // Tell the child
  if (dvol && (caller != dvol)) {
    dvol->set_max_gradient(value,this);
  }
  if (parent && (parent != caller)) {
    parent->set_max_gradient(value,this);
  }
  bufsetup();
  netputnfloat(&max_grad_mag,1);
  netsnd(DATAVOLUME_MAX_GRAD);    
}

void netDataVolume::load_xplane( const DataType *data_in, int which_x )
{
  if (dvol) dvol->load_xplane( data_in, which_x );
  else {
    bufsetup();
    netputnint(&which_x, 1);
    netputdata(data_in, real_ydim*real_zdim);
    netsnd(DATAVOLUME_LOAD_X);
  }
}

void netDataVolume::load_yplane( const DataType *data_in, int which_y )
{
  if (dvol) dvol->load_yplane( data_in, which_y );
  else {
    bufsetup();
    netputnint(&which_y, 1);
    netputdata(data_in, real_zdim*real_xdim);
    netsnd(DATAVOLUME_LOAD_Y);
  }
}

void netDataVolume::load_zplane( const DataType *data_in, int which_z )
{
  if (dvol) dvol->load_zplane( data_in, which_z );
  else {
    bufsetup();
    netputnint(&which_z, 1);
    netputdata(data_in, real_xdim*real_ydim);
    netsnd(DATAVOLUME_LOAD_Z);
  }
}

int netDataVolume::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case DATAVOLUME_FINISH_INIT:
    {
      finish_init();
    }
    break;

  case DATAVOLUME_MAX_GRAD:
    {
      float val;
      netgetnfloat(&val,1);
      send_ack_and_break_lock(); // next step may require sending messages
      max_grad_mag= val;
      // Tell the child
      if (dvol) dvol->set_max_gradient(val,this);
      if (parent) parent->set_max_gradient(val,this);
    }
    break;

  case DATAVOLUME_LOAD_X:
    {
      int num;
      netgetnint(&num, 1);
      DataType *buf= new DataType[real_ydim * real_zdim];
      netgetdata(buf,real_ydim*real_zdim);
      load_xplane( buf, num );
      delete [] buf;
    }
    break;

  case DATAVOLUME_LOAD_Y:
    {
      int num;
      netgetnint(&num, 1);
      DataType *buf= new DataType[real_zdim * real_xdim];
      netgetdata(buf,real_zdim*real_xdim);
      load_yplane( buf, num );
      delete [] buf;
    }
    break;

  case DATAVOLUME_LOAD_Z:
    {
      int num;
      netgetnint(&num, 1);
      DataType *buf= new DataType[real_xdim * real_ydim];
      netgetdata(buf,real_xdim*real_ydim);
      load_zplane( buf, num );
      delete [] buf;
    }
    break;

  case DATAVOLUME_LOAD_REMOTE:
    {
      if (!dvol) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netDataVolume::netrcv: LOAD_REMOTE recieved at wrong end!\n");
	exit(-1);
      }
      netDataFile *netdfile= (netDataFile *)netgetlocalobj();
      if (!netdfile) {
	fprintf(stderr,"netDataVolume::netrcv: sent a foreign datafile!\n");
	exit(-1);
      }
      dvol->load_datafile( netdfile->get_real_datafile() );
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netDataVolume::handle_datavolume_request()
{
  // Handle incoming request for a datavolume here
  RemObjInfo rem;
  int xin, yin, zin;
  gBoundBox bbox(0.0,0.0,0.0,1.0,1.0,1.0);
  netgetreminfo(&rem);
  char *info= netgetstring();
  unpack_param_info( info, xin, yin, zin, bbox );
  DataVolume *dvol= new netDataVolume( &rem, 
				       new DataVolume(xin, yin, zin, bbox) );
}

void netDataVolume::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );
    
    // Add our message handler
    add_msg_handler( DATAVOLUME_REQDATAVOLUME, 
		     (void (*)())handle_datavolume_request );
    
    initialized= 1;
  }
}

char *netDataVolume::pack_param_info( const int xin, const int yin,
				      const int zin, 
				      const gBoundBox& bbox_in )
{
  sprintf(param_info_buf,
	  "%4d %4d %4d %16f %16f %16f %16f %16f %16f",
	  xin, yin, zin, 
	  bbox_in.xmin(), bbox_in.ymin(), bbox_in.zmin(),
	  bbox_in.xmax(), bbox_in.ymax(), bbox_in.zmax());
  return( param_info_buf );
}

void netDataVolume::unpack_param_info( char *info, int& xin, int& yin,
				       int& zin, gBoundBox& bbox_in )
{
  float xmin, ymin, zmin, xmax, ymax, zmax;
  sscanf(info,"%d %d %d %f %f %f %f %f %f",
	 &xin, &yin, &zin, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
  bbox_in= gBoundBox( xmin, ymin, zmin, xmax, ymax, zmax );
}

/*
*
* DataVolume class has no public constructors, so it doesn't need its
* own server.  This server will work if the constructors are made public
* (I think).
*
*/
#ifdef never

int DataVolumeServer::initialized= 0;

DataVolumeServer::DataVolumeServer()
: baseServer(DataVolumeServerExeName), 
  baseNet( BASENET_REQSERVER, "", server_info )
{
  // The baseServer constructor fires up the executable, and the
  // baseNet constructor fires a requst to it which causes us to
  // get connected.  
  logger= NULL;
}

DataVolumeServer::DataVolumeServer( RemObjInfo *rem_in )
: baseNet(rem_in), baseServer()
{
  logger= new netLogger("DataVolumeServer");
}

DataVolumeServer::~DataVolumeServer()
{
  delete logger;
}

int DataVolumeServer::netrcv(NetMsgType msg)
{
  int rcv_ok= 1;
  switch (msg) {

  case DATAVOLUME_REQDATAVOLUME: // request for new datavol- purpose of server
    {
      RemObjInfo remote;
      int xin, yin, zin;
      gBoundBox bbox_in(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
      netgetreminfo(&remote);
      char *info= netgetstring();
      netDataVolume::unpack_param_info( info, xin, yin, zin, bbox_in );
      fprintf(stderr,
	  "DataVolumeServer::netrcv: sender= <%s> %d %d; info %d %d %d ...\n",
	      remote.component,remote.instance,remote.object,
	      xin, yin, zin );
      if ( remote != *partner_rem() ) {
	// Too many people to talk to to use ACK mechanism
	send_ack_and_break_lock();
      }
      DataVolume *dvol= 
	new netDataVolume( &remote, 
			   new DataVolume(xin, yin, zin, bbox_in) );
    }
    break;
    
  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void DataVolumeServer::handle_server_request()
// This routine catches the initial server request from the partner
{
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  DataVolumeServer *dvolserver= new DataVolumeServer(&rem);
}

void DataVolumeServer::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void DataVolumeServer::create_data_volume( RemObjInfo *rem_in, 
					  char *param_info )
{
  break_ack_lock(); // so we can send to server, which will ack to requestor
  bufsetup();
  netputreminfo(rem_in);
  netputstring( param_info );
  netsnd(DATAVOLUME_REQDATAVOLUME);
}

#endif // around DataVolumeServer class

int netSampleVolume::initialized= 0;

int netSampleVolume::param_buf_size= 0;
char *netSampleVolume::param_info_buf= NULL;

netSampleVolume::netSampleVolume( RemObjInfo *rem_in, 
				  const GridInfo& grid_in, 
				  baseTransferFunction& tfun_in,
				  int ndatavol, DataVolume **data_table,
				  baseVRen *owner_in )
: baseNet(rem_in), 
  baseSampleVolume( grid_in, tfun_in, ndatavol, data_table )
{
  if (!initialized) initialize("NoNameGiven");

  svol= NULL;
  owner= owner_in;
}

// Cast in constructor works as long as same VRen made the tfun and this
// SampleVolume.
netSampleVolume::netSampleVolume( const GridInfo& grid_in, 
				  baseTransferFunction& tfun_in,
				  int ndatavol, DataVolume **data_table,
				  RemObjInfo *server )
: baseNet( SAMPLEVOLUME_REQSAMPLEVOLUME, 
	   pack_param_info( grid_in, tfun_in, ndatavol, data_table ),
	   server ),
  baseSampleVolume( grid_in, tfun_in, ndatavol, data_table )
{
  if (!initialized) initialize("NoNameGiven");

  svol= NULL;
  owner= NULL;
}

netSampleVolume::~netSampleVolume()
{
  // Note that this was originally passed in as a parameter to the
  // constructor, so we didn\'t allocate it, but there seems no more
  // elegant way to do this.
  delete svol;
}

void netSampleVolume::construct()
{
  // If owner is non-null, we are the remote partner and should use
  // that owner to construct a samplevolume from our parts.  Otherwise,
  // there is nothing to be done.
  if (owner) {
    svol= owner->create_sample_volume( gridinfo(), *transferfunction(),
				       ndata(), data_table() );
  }
  else { // Pass the request on to the remote partner
    bufsetup();
    netsnd(SAMPLEVOLUME_CONSTRUCT);
  }
}

void netSampleVolume::regenerate( baseTransferFunction& tfun_in,
				  int ndatavol, DataVolume** data_table )
{
  // Take care of bookkeeping
  baseSampleVolume::regenerate( tfun_in, ndatavol, data_table );

  // Do the job, or transmit the new info
  if (svol) {
    baseTransferFunction* real_tfun= 
      ((netTransferFunction&)tfun_in).get_real_tfun();
    DataVolume** real_data_table= new DataVolume*[ndatavol];
    for (int i=0; i<ndatavol; i++) 
      real_data_table[i]= ((netDataVolume*)data_table[i])->get_real_datavol();
    svol->regenerate( *real_tfun, ndatavol, real_data_table );
    delete [] real_data_table;
  }
  else {
    bufsetup();
    // Casts will work as long as the renderer which created this
    // netSampleVolume also created the tfun object
    ((netTransferFunction *)(&tfun_in))->netputremoteobj();
    netputnint(&ndatavol,1);
    for (int i=0; i<ndatavol; i++)
      ((netDataVolume *)(data_table[i]))->netputremoteobj();
    netsnd(SAMPLEVOLUME_REGENERATE);
  }
}

void netSampleVolume::set_size_scale( const float new_scale )
{
  baseSampleVolume::set_size_scale( new_scale );
  bufsetup();
  float tmp_scale= new_scale;  // Seem to have to force it into memory
  netputnfloat( &tmp_scale, 1 );
  netsnd(SAMPLEVOLUME_SET_SIZE_SCALE);
}

int netSampleVolume::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case SAMPLEVOLUME_REGENERATE:
    {
      int error= 0;
      netTransferFunction *tfun= (netTransferFunction *)netgetlocalobj();
      if (!tfun) error= 1;

      int ndatavol;
      netgetnint(&ndatavol,1);
      
      DataVolume** data_table= new DataVolume*[ndatavol];
      for (int i=0; i<ndatavol; i++) {
	data_table[i]= (netDataVolume *)netgetlocalobj();
	if (!data_table[i]) error= 1;
      }

      if (!error) regenerate( *tfun, ndatavol, data_table );
      else { 
	// Might happen if renderer that made this netSampleVolume
	// did not make the tfun
	fprintf(stderr,"netSampleVolume::netrcv: foreign object received!\n");
	exit(-1);
      }
    }
    break;

  case SAMPLEVOLUME_CONSTRUCT:
    {
      construct();
    }
    break;

  case SAMPLEVOLUME_SET_SIZE_SCALE:
    {
      float new_scale;
      netgetnfloat( &new_scale, 1 );
      baseSampleVolume::set_size_scale( new_scale );
      if (svol) svol->set_size_scale( new_scale );
    }
    break;
    
    default: rcv_ok= baseNet::netrcv(msg);
    }
  return( rcv_ok );
}

void netSampleVolume::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );

    // We do not add a SAMPLEVOLUME_REQSAMPLEVOLUME message handler,
    // because SampleVolumes are only constructed by VRens.

    initialized= 1;
  }
}

char *netSampleVolume::pack_param_info( const GridInfo& grid_in, 
					baseTransferFunction& tfun_in,
					int ndatavol_in, 
					DataVolume **data_table )
{
  // Casts will work as long as the objects were made by the same
  // renderer which made this netSampleVolume.
  char *tfun_string= ((netTransferFunction *)(&tfun_in))->pack_netobj();
  char **data_strings;
  if (ndatavol_in) data_strings= new char*[ndatavol_in];
  else data_strings= NULL;

  int i;
  for (i=0; i<ndatavol_in; i++) 
    data_strings[i]= ((netDataVolume *)(data_table[i]))->pack_netobj();

  int length= 256; // for 6 bbox coordinates + 3 int dims + ndatavol_in int
  length += strlen( tfun_string ) + 6;
  for (i=0; i<ndatavol_in; i++) length += ( strlen( data_strings[i] ) + 6 );

  if (length+1 > param_buf_size) {
    delete param_info_buf;
    param_info_buf= new char[ length+1 ];
    param_buf_size= length+1;
  }

  sprintf(param_info_buf,"%16f %16f %16f %16f %16f %16f %5d %5d %5d %4d ",
	  grid_in.bbox().xmin(), grid_in.bbox().ymin(), grid_in.bbox().zmin(),
	  grid_in.bbox().xmax(), grid_in.bbox().ymax(), grid_in.bbox().zmax(),
	  grid_in.xsize(), grid_in.ysize(), grid_in.zsize(), ndatavol_in);
  char *runner= param_info_buf + strlen(param_info_buf);  // current end
  sprintf(runner,"%4d ", strlen(tfun_string));
  runner += 5;
  for (i=0; i<ndatavol_in; i++) {
    sprintf(runner,"%4d ", strlen(data_strings[i]) );
    runner += 5;
  }
  strcpy(runner, tfun_string);
  runner += strlen(tfun_string);
  for (i=0; i<ndatavol_in; i++) {
    strcpy(runner, data_strings[i]);
    runner += strlen(data_strings[i]);
  }

  delete tfun_string;
  for (i=0; i<ndatavol_in; i++) delete data_strings[i];
  delete [] data_strings;

  return( param_info_buf );
}

void netSampleVolume::unpack_param_info( char *info, GridInfo& grid_out,
					 baseTransferFunction** tfun_in,
					 int& ndatavol, 
					 DataVolume ***data_table )
{
  float xmin, ymin, zmin, xmax, ymax, zmax;
  int tmp_xsize, tmp_ysize, tmp_zsize;
  int tmp_num;

  sscanf( info, "%16f %16f %16f %16f %16f %16f %5d %5d %5d %4d ",
	  &xmin, &ymin, &zmin, &xmax, &ymax, &zmax, 
	  &tmp_xsize, &tmp_ysize, &tmp_zsize, &tmp_num );
  ndatavol= tmp_num;
  grid_out= GridInfo( tmp_xsize, tmp_ysize, tmp_zsize, 
		      gBoundBox( xmin, ymin, zmin, xmax, ymax, zmax ) );

  int tfun_length;
  int *data_lengths= new int[ndatavol];
  int i;

  info += 125; // read so far
  sscanf( info, "%d ", &tfun_length );
  info += 5;
  for (i=0; i<ndatavol; i++) {
    sscanf( info, "%d ", &(data_lengths[i]) );
    info += 5;
  }

  // Casts will work as long as the same renderer that created this
  // netSampleVolume also created these objects
  netTransferFunction *nettfun= (netTransferFunction *)unpack_localobj(info);
  if (!nettfun) {
    fprintf(stderr,
	    "netSampleVolume::unpack_param_info: foreign tfun received!\n");
    exit(-1);
  }
  *tfun_in= nettfun->get_real_tfun();
  info += tfun_length;

  *data_table= new DataVolume*[ndatavol];
  for (i=0; i<ndatavol; i++) {
    netDataVolume *thisnetdvol= (netDataVolume *)unpack_localobj(info);
    if (!thisnetdvol) {
      fprintf(stderr,
	      "netSampleVolume::unpack_param_info: foreign data received!\n");
      exit(-1);
    }
    (*data_table)[i]= thisnetdvol->get_real_datavol();
    info += data_lengths[i];
  }

  delete [] data_lengths;
}

int netVolGob::initialized= 0;
char *netVolGob::param_info_buf= NULL;
int netVolGob::param_buf_size= 0;

netVolGob::netVolGob( RemObjInfo *rem_in, VolGob *real_vol )
: baseNet(rem_in), VolGob( NULL, real_vol->transform() )
{
  if (!initialized) initialize("NoNameGiven");

  volgob= real_vol;
}

// Cast in constructor works as long as same VRen created vol_in and this
netVolGob::netVolGob( baseSampleVolume *vol_in, const gTransfm& trans,
		      RemObjInfo *server ) 
: baseNet( VOLGOB_REQVOLGOB, 
	   pack_param_info( trans, (netSampleVolume *)vol_in ), 
	   server ),
  VolGob( vol_in, trans )
{
  if (!initialized) initialize("NoNameGiven");
  volgob= NULL;
}

netVolGob::~netVolGob()
{
  // Note that this was originally passed in as a parameter to the
  // constructor, so we didn\'t allocate it, but there seems no more
  // elegant way to do this.
  delete volgob;
}

void netVolGob::update_trans( const gTransfm& trans_in )
{
  trans= trans_in;
  bufsetup();
  netputnfloat(trans.floatrep(),16);
  netsnd(VOLGOB_UPDATE_TRANS);
}

int netVolGob::netrcv(NetMsgType msg)
{
  int rcv_ok= 1;
  switch (msg) {

  case VOLGOB_UPDATE_TRANS:
    {
      float data_in[16];
      netgetnfloat(data_in, 16);
      trans= gTransfm(data_in);
      if (volgob) volgob->update_trans( trans );
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netVolGob::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );
    
    // Add our message handler
    add_msg_handler( VOLGOB_REQVOLGOB, 
		     (void (*)())handle_volgob_request );
    
    initialized= 1;
  }
}

char *netVolGob::pack_param_info( const gTransfm& trans,
				  netSampleVolume* svol )
{
  char *svol_string= svol->pack_netobj();
  char *trans_string= trans.tostring();

  int length= strlen(trans_string) + strlen(svol_string) + 1;
  if (length>param_buf_size) {
    delete param_info_buf;
    param_info_buf= new char[length];
  }

  (void)strcpy(param_info_buf, trans_string);
  (void)strcat(param_info_buf, svol_string);

  delete svol_string;
  delete trans_string;

  return(param_info_buf);
}

void netVolGob::unpack_param_info( char *info, gTransfm& trans,
				   baseSampleVolume** svol_in )
{
  trans= gTransfm::fromstring(&info);  // advances info to end of trans data
  // Following cast works as long as same VRen that made this made svol.
  netSampleVolume *netsvol= (netSampleVolume *)unpack_localobj(info);
  if (!netsvol) {
    fprintf(stderr,
	    "netVolGob::unpack_param_info: foreign sample volume received!\n");
      exit(-1);
  }
  *svol_in= netsvol->get_real_svol();
}

void netVolGob::handle_volgob_request()
{
  // Handle incoming request for a datavolume here
  RemObjInfo rem;
  netgetreminfo(&rem);
  char *info= netgetstring();
  gTransfm trans;
  baseSampleVolume *svol;
  unpack_param_info(info, trans, &svol);
  VolGob *vg= new netVolGob( &rem, new VolGob( svol, trans ) );
}

int netTransferFunction::initialized= 0;
int netTransferFunction::param_buf_size= 0;
char *netTransferFunction::param_info_buf= NULL;

static void tfun_method( Sample& smpl, int i, int j, int k, int ndata,
			 DataVolume **data_table )
// Note that generated colors must be multiplied by alpha, because
// the color accumulation algorithm assumes pre-multiplication.
{
  if (ndata<1) {
    fprintf(stderr,"tfun_method: %d is not enough data volumes!\n",ndata);
    exit(-1);
  }
  
  float r, g, b, alpha, value;
  value= (*data_table)->fval(i,j,k);
  if (value < 0.1)
    r= g= b= alpha= 0.0;
  else if (value < 0.3) {
    alpha= 0.3*value;
    b= 1.0;
    r= g= 0.0;
  }
  else if (value < 0.7) {
    alpha= value;
    r= b= 0.0;
    g= 1.0;
  }
  else {
    alpha= 1.0;
    r= b= 1.0;
    g= 0.0;
  }
 
  smpl.clr= gBColor( r, g, b, alpha );
}

netTransferFunction::netTransferFunction( RemObjInfo *rem_in,
					  baseVRen *generating_renderer )
: baseNet(rem_in), baseTransferFunction( 0 )
{
  if (!initialized) initialize("NoNameGiven");

  renderer= generating_renderer;
  delete_tfun_on_delete= 1;
  tfun= NULL;

  // Partner will now send a TFUN_PASSTFUN message, which will
  // put something reasonable in the tfun slot.
}

netTransferFunction::netTransferFunction( baseTransferFunction *tfun_in,
					  RemObjInfo *server )
: baseNet( TFUN_REQTFUN, pack_param_info(), server ),
  baseTransferFunction( tfun_in->ndata() )
{
  if (!initialized) initialize("NoNameGiven");

  tfun= tfun_in;
  renderer= NULL;
  delete_tfun_on_delete= 0;

  // Sending this message will cause the partner\'s tfun to be replaced
  // with something reasonable.
  bufsetup();
  netput_tfun( tfun );
  netsnd(TFUN_PASSTFUN);
}

netTransferFunction::~netTransferFunction()
{
  // Note that this was originally passed in as a parameter to the
  // constructor, so we didn\'t allocate it, but there seems no more
  // elegant way to do this.
  if (delete_tfun_on_delete) delete tfun;  
}

void netTransferFunction::handle_tfun_request()
{
  // Handle incoming request for a TransferFunction here
  RemObjInfo rem;
  netgetreminfo(&rem);
  char *info= netgetstring();
  unpack_param_info(info);
  netTransferFunction *tf= 
    new netTransferFunction( &rem, NULL );
}

int netTransferFunction::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case TFUN_PASSTFUN:
    {
      baseTransferFunction *tmp_tfun= netget_tfun();
      if (renderer) {
	tfun= renderer->register_tfun( tmp_tfun );
	delete tmp_tfun;
      }
      else tfun= tmp_tfun;
      ndatavol= tfun->ndata();
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netTransferFunction::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );
    
    // Add our message handler
    add_msg_handler( TFUN_REQTFUN, 
		     (void (*)())handle_tfun_request );
    
    initialized= 1;
  }
}

char *netTransferFunction::pack_param_info()
{
  return "";
}

void netTransferFunction::unpack_param_info( const char *info )
{
}

void netTransferFunction::netput_tfun( baseTransferFunction *tfun )
{
  int ttype= tfun->type();

  netputnint(&ttype, 1);
  int num_datavol= tfun->ndata();
  netputnint(&num_datavol, 1);

  switch (ttype) {

  case BBOX_TFUN:
    {
      int dims[3];
      ((BoundBoxTransferFunction *)tfun)->get_sizes( dims, dims+1, dims+2 );
      netputnint(dims, 3);
      float bounds[6];
      bounds[0]= ((BoundBoxTransferFunction *)tfun)->boundbox().xmin();
      bounds[1]= ((BoundBoxTransferFunction *)tfun)->boundbox().ymin();
      bounds[2]= ((BoundBoxTransferFunction *)tfun)->boundbox().zmin();
      bounds[3]= ((BoundBoxTransferFunction *)tfun)->boundbox().xmax();
      bounds[4]= ((BoundBoxTransferFunction *)tfun)->boundbox().ymax();
      bounds[5]= ((BoundBoxTransferFunction *)tfun)->boundbox().zmax();
      netputnfloat(bounds,6);
    }
    break;

  case TABLE_TFUN:
  case GRADTABLE_TFUN:
    {
      int tsize= 256;
      netputnint(&tsize, 1);
      unsigned char *buf= new unsigned char[4*256];
      unsigned char *runner= buf;
      gBColor *table= ((TableTransferFunction *)tfun)->get_table();
      for (int i=0; i<256; i++) {
	*runner++= table->ir();
	*runner++= table->ig();
	*runner++= table->ib();
	*runner++= table->ia();
	table++;
      }
      netputbytes((char *)buf, 4*256);
      delete buf;
    }
    break;

  case SUM_TFUN:
    {
      int count= ((SumTransferFunction *)tfun)->tfun_count();
      float *factor_tbl= ((SumTransferFunction *)tfun)->factor_table();
      baseTransferFunction **table= 
	((SumTransferFunction *)tfun)->tfun_table();
      netputnint(&count, 1);
      netputnfloat(factor_tbl, count);
      for (int i=0; i<count; i++) netput_tfun( table[i] );
    }
    break;

  case SSUM_TFUN:
    {
      int count= ((SSumTransferFunction *)tfun)->tfun_count();
      float *factor_tbl= ((SSumTransferFunction *)tfun)->factor_table();
      baseTransferFunction **table= 
	((SSumTransferFunction *)tfun)->tfun_table();
      netputnint(&count, 1);
      netputnfloat(factor_tbl, count);
      for (int i=0; i<count; i++) netput_tfun( table[i] );
    }
    break;

  case METHOD_TFUN:
    {
      // Since this is only a kludge, there is nothing to pass.
    }
    break;

  case MASK_TFUN:
    {
      int ibuf[2];
      float fbuf[2];
      MaskTransferFunction* mtfun= (MaskTransferFunction*)tfun;
      ibuf[0]= ( mtfun->get_mask() != NULL );
      ibuf[1]= ( mtfun->get_input() != NULL );
      fbuf[0]= mtfun->get_mask_weight();
      fbuf[1]= mtfun->get_input_weight();
      netputnint(ibuf, 2);
      netputnfloat(fbuf, 2);
      if (ibuf[0]) netput_tfun( mtfun->get_mask() );
      if (ibuf[1]) netput_tfun( mtfun->get_input() );
    }
    break;

  case BLOCK_TFUN:
    {
      int ibuf[8];
      float fbuf[12];
      gBColor clr;
      ((BlockTransferFunction*)tfun)->get_info( fbuf, fbuf+1, fbuf+2,
					       fbuf+3, fbuf+4, fbuf+5,
					       &clr, ibuf+4 );
      GridInfo grid= ((BlockTransferFunction*)tfun)->get_grid();
      fbuf[6]= grid.bbox().xmin();
      fbuf[7]= grid.bbox().ymin();
      fbuf[8]= grid.bbox().zmin();
      fbuf[9]= grid.bbox().xmax();
      fbuf[10]= grid.bbox().ymax();
      fbuf[11]= grid.bbox().zmax();
      ibuf[0]= clr.ir();
      ibuf[1]= clr.ig();
      ibuf[2]= clr.ib();
      ibuf[3]= clr.ia();
      // ibuf[4] is val. of inside
      ibuf[5]= grid.xsize();
      ibuf[6]= grid.ysize();
      ibuf[7]= grid.zsize();
      netputnint( ibuf, 8 );
      netputnfloat( fbuf, 12 );
    }
    break;

  default: fprintf(stderr,
		   "netTransferFunction::netput_tfun: unencodable type!\n");
  }
}

baseTransferFunction *netTransferFunction::netget_tfun()
{
  int ttype;
  netgetnint(&ttype, 1);
  int ndv;
  netgetnint(&ndv, 1);
  
  switch (ttype) {
  case BBOX_TFUN:
    {
      int dims[3];
      netgetnint(dims, 3);
      float bounds[6];
      netgetnfloat(bounds, 6);
      BoundBoxTransferFunction *result= 
	new BoundBoxTransferFunction( gBoundBox( bounds[0], bounds[1],
						 bounds[2], bounds[3],
						 bounds[4], bounds[5] ),
				      dims[0], dims[1], dims[2] );
      return( result );
    }
    break;

  case TABLE_TFUN:
    {
      int tsize;
      netgetnint(&tsize, 1);
      if (tsize != 256) {
	fprintf(stderr,"netTransferFunction::netget_tfun: incompatibility!\n");
	exit(-1);
      }
      unsigned char *buf= new unsigned char[256*4];
      unsigned char *brunner= buf;
      gBColor *table= new gBColor[256];
      gBColor *trunner= table;
      netgetbytes((char *)buf,256*4);
      for (int i=0; i<256; i++) {
	*trunner= gBColor(*brunner, *(brunner+1), *(brunner+2), *(brunner+3));
	trunner++;
	brunner += 4;
      }
      delete buf;

      TableTransferFunction *result= 
	new TableTransferFunction(ndv, table);

      delete table;

      return( result );
    }
    break;

  case GRADTABLE_TFUN:
    {
      int tsize;
      netgetnint(&tsize, 1);
      if (tsize != 256) {
	fprintf(stderr,"netTransferFunction::netget_tfun: incompatibility!\n");
	exit(-1);
      }
      unsigned char *buf= new unsigned char[256*4];
      unsigned char *brunner= buf;
      gBColor *table= new gBColor[256];
      gBColor *trunner= table;
      netgetbytes((char *)buf,256*4);
      for (int i=0; i<256; i++) {
	*trunner= gBColor(*brunner, *(brunner+1), *(brunner+2), *(brunner+3));
	trunner++;
	brunner += 4;
      }
      delete buf;

      GradTableTransferFunction *result= 
	new GradTableTransferFunction(ndv, table);

      delete table;

      return( result );
    }
    break;

  case SUM_TFUN:
    {
      int count;
      netgetnint( &count, 1 );
      float *factor_tbl= new float[count];
      baseTransferFunction **table= new baseTransferFunction*[count];

      netgetnfloat(factor_tbl, count);
      for (int i=0; i<count; i++) {
	table[i]= netget_tfun();
      }

      baseTransferFunction *result= 
	new SumTransferFunction( ndv, count, factor_tbl, table );

      delete [] factor_tbl;
      delete [] table;

      return( result );

    }
    break;

  case SSUM_TFUN:
    {
      int count;
      netgetnint( &count, 1 );
      float *factor_tbl= new float[count];
      baseTransferFunction **table= new baseTransferFunction*[count];

      netgetnfloat(factor_tbl, count);
      for (int i=0; i<count; i++) {
	table[i]= netget_tfun();
      }

      baseTransferFunction *result= 
	new SSumTransferFunction( ndv, count, factor_tbl, table );

      delete [] factor_tbl;
      delete [] table;

      return( result );

    }
    break;

  case METHOD_TFUN:
    {
      // Since this is only a kludge, there is nothing to get.
      return( new MethodTransferFunction( ndv, tfun_method ) );
    }
    break;

  case MASK_TFUN:
    {
      int ibuf[2];
      float fbuf[2];
      netgetnint(ibuf, 2);
      netgetnfloat(fbuf, 2);
      
      baseTransferFunction* mask;
      if (ibuf[0]) mask= netget_tfun();
      else mask= NULL;

      baseTransferFunction* input;
      if (ibuf[1]) input= netget_tfun();
      else input= NULL;

      return new MaskTransferFunction( ndv, input, fbuf[1], 
				       mask, fbuf[0] );
    }
    break;

  case BLOCK_TFUN:
    {
      int ibuf[8];
      float fbuf[12];
      netgetnint( ibuf, 8 );
      netgetnfloat( fbuf, 12 );
      gBoundBox incoming_bbox( fbuf[6], fbuf[7], fbuf[8],
			       fbuf[9], fbuf[10], fbuf[11] );
      GridInfo grid( ibuf[5], ibuf[6], ibuf[7], incoming_bbox );
      return new BlockTransferFunction( fbuf[0], fbuf[1], fbuf[2],
					fbuf[3], fbuf[4], fbuf[5],
					gBColor( ibuf[0], ibuf[1], 
						 ibuf[2], ibuf[3] ),
					ibuf[4], grid );
    }
    break;

  default: fprintf(stderr,
		   "netTransferFunction::netget_tfun: unknown type %d!\n",
		   ttype);
  }
  return NULL; // not reached
}

int netVRen::initialized= 0;

int VRenServer::initialized= 0;

char netVRen::param_info_buf[ vren_param_buf_size ];

netVRen::netVRen( RemObjInfo *rem_in, const int type )
: baseNet(rem_in), baseVRen( NULL, NULL, NULL, NULL, NULL, NULL )
{
  if (!initialized) initialize("NoNameGiven");
  
  // Generate the logger
  char *loggertxt= new char[ strlen("netVRen on ") + strlen(get_net_name()) 
			     + 1 ];
  strcpy( loggertxt, "netVRen on " );
  strcat( loggertxt, get_net_name() );
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  extern int _MPP_MY_PE;
  if (_MPP_MY_PE)
    logger= new dummyLogger( loggertxt );
  else logger= new netLogger( loggertxt ); // processor 0 talks to logger
#else
  logger= new netLogger( loggertxt );
#endif
  created_own_logger= 1;
  delete loggertxt;
  
  // Ask the partner for the image handler
  ihandler= new netImageHandler("", partner_rem());
  
  // Create the real renderer which we hang on to, according to specs.
#ifdef NO_REAL_VREN
  out_vren= new baseVRen( logger, ihandler, NULL, NULL, this );
#else
  // Decode type information, and use it to create subrenderers
  int nprocs;
  int nthreads;
  int split_plane;
  decode_type(type, &split_plane, &nprocs, &nthreads);
  if (nprocs<=1) 
    out_vren= new raycastVRen( logger, ihandler,
			       netVRen::ready_callback, (void*)this,
			       this, (void (*)())service, nthreads );
  else
    out_vren= new bcompVRen( type, logger, ihandler, 
			     netVRen::ready_callback, (void*)this,
			     this, (void (*)())service );
#endif
}

netVRen::netVRen( const int type,
		  baseLogger *logger_in, baseImageHandler *imagehandler, 
		  void (*ready_handler)(baseVRen* renderer, 
					void* ready_cb_data),
		  void* ready_cb_data_in,
		  void (*error_handler)(int error_id, baseVRen *renderer), 
		  void (*fatal_handler)(int error_id, baseVRen *renderer) )
: baseNet(VREN_REQVREN, pack_param_info(type)), 
  baseVRen( logger_in, imagehandler, ready_handler, ready_cb_data_in,
	   error_handler, fatal_handler )
{
  if (!initialized) initialize("NoNameGiven");
  out_vren= NULL;
  created_own_logger= 0;
}

netVRen::netVRen( const int type,
		  baseLogger *logger_in, baseImageHandler *imagehandler, 
		  void (*ready_handler)(baseVRen* renderer, 
					void* ready_cb_data),
		  void* ready_cb_data_in,
		  baseVRen *owner )
: baseNet(VREN_REQVREN, pack_param_info(type)), 
  baseVRen( logger_in, imagehandler, ready_handler, ready_cb_data_in, owner )
{
  if (!initialized) initialize("NoNameGiven");
  out_vren= NULL;
  created_own_logger= 0;
}

netVRen::~netVRen()
{
  delete out_vren;
  delete ihandler;
  if (created_own_logger) delete logger;
}

void netVRen::ready_callback( baseVRen* renderer, void* cb_data )
{
  netVRen* has_ready_outren= (netVRen*)cb_data;
  has_ready_outren->handle_ready( renderer );
}

int netVRen::encode_type( const int split_plane, const int num_procs,
			  const int num_threads )
{
  // For historical reasons we store all this info in an int.  If we
  // ever run on a machine with more than 2**29 PEs we'll be in trouble,
  // but heck, I'll be retired by then.
  int result= 0;
  if ( (sizeof(int) != 4)
       || (split_plane>3) || (split_plane<0)
       || (num_procs>65535) || (num_procs<0)
       || ( num_threads>8191) || (num_threads<0) ) {
    fprintf(stderr,
	    "netVRen::encode_type: invalid or out of range params: %d %d %d\n",
	    split_plane, num_procs, num_threads);
    exit(-1);
  }
  result |= ((split_plane & 3) << 16);
  result |= (num_procs & 65535);
  result |= ((num_threads & 8191) << 18);
  return result;
}

void netVRen::decode_type( const int type, int* split_plane, int* num_procs,
			   int* num_threads )
{
  // See comments in netVRen::encode_type()
  *split_plane= ((type >> 16) & 3);
  *num_procs= (type & 65535);
  *num_threads= ((type >> 18) & 8191);
}

void netVRen::handle_ready( baseVRen* renderer )
{
  if (ready_proc) (*ready_proc)(this, ready_cb_data);
  else {
    if (!owner) {
      logger->comment("Ready");
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
      // This is a good time for PEs other than 0 to stop 
      // listening to the server
      extern int _MPP_MY_PE;
      if (_MPP_MY_PE) listen_only_to_local_partners();
#endif
    }
    // Send ready message back to partner
    bufsetup();
    (void)netsnd(VREN_READY);
  }
}

void netVRen::initialize( const char *name )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );
    netImageHandler::initialize( name, NULL );
    netDataVolume::initialize( name );
    netSampleVolume::initialize( name );
    netVolGob::initialize( name );
    netTransferFunction::initialize( name );

    // Add our message handler
    add_msg_handler( VREN_REQVREN, 
		     (void (*)())handle_netvren_request );

    initialized= 1;
  }
}

void netVRen::handle_netvren_request()
{
  // Handle incoming request for a vren here
  RemObjInfo rem;
  netgetreminfo(&rem);
  char *info= netgetstring();
  int type;
  unpack_param_info( info, type );
  baseVRen *vren= new netVRen( &rem, type );
}

char *netVRen::pack_param_info( const int type )
{
  sprintf(param_info_buf,"%8d", type);
  return(param_info_buf);
}

void netVRen::unpack_param_info( const char *info, int &type)
{
  sscanf((char *)info, "%d", &type);
}

void netVRen::error( int error_id )
{
  // If we have any error handling capability, we are the "local"
  // half of the pair and we should process the error.  Otherwise
  // we are the "remote" half of the pair and should ask the
  // local version to do it.
  if (owner) owner->error( error_id );
  else if (error_proc) (*error_proc)(error_id, this);
  else {
    bufsetup();
    netputnint(&error_id, 1);
    (void)netsnd(VREN_ERROR);
  }
}

void netVRen::fatal( int error_id )
{
  // If we have any error handling capability, we are the "local"
  // half of the pair and we should process the error.  Otherwise
  // we are the "remote" half of the pair and should ask the
  // local version to do it.
  if (owner) owner->fatal( error_id );
  else if (fatal_proc) (*fatal_proc)(error_id, this);
  else {
    bufsetup();
    netputnint(&error_id, 1);
    (void)netsnd(VREN_FATAL);
  }
}

void netVRen::setOptionFlags( const VRenOptions ops )
{
  options= ops;
  bufsetup();
  netputnint(&options, 1);
  (void)netsnd(VREN_SETOPTFLAGS);
}

void netVRen::StartRender( int image_xdim, int image_ydim )
{
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  // Maybe PE 0 should stop listening to the world now
  extern int _MPP_MY_PE;
  if ((!_MPP_MY_PE) && (!owner)) {
    listen_only_to_local_partners();
  }
#endif
  bufsetup();
  netputnint( &image_xdim, 1 );
  netputnint( &image_ydim, 1 );
  (void)netsnd(VREN_RENDER);
}

void netVRen::AbortRender()
{
  bufsetup();
  (void)netsnd(VREN_ABORT);
}

void netVRen::setCamera( const gPoint& lookfm, const gPoint& lookat, 
			 const gVector& up, const float fov, 
			 const float hither, const float yon,
			 const int parallel_flag ) 
{
  // Set our own camera
  baseVRen::setCamera( lookfm, lookat, up, fov, hither, yon, parallel_flag );

  // Set partner\'s camera or slave\'s camera
  if (out_vren) 
    out_vren->setCamera( lookfm, lookat, up, fov, hither, yon, parallel_flag );
  else {
    float comp[12];
    comp[0]= lookfm.x();
    comp[1]= lookfm.y();
    comp[2]= lookfm.z();
    comp[3]= lookat.x();
    comp[4]= lookat.y();
    comp[5]= lookat.z();
    comp[6]= up.x();
    comp[7]= up.y();
    comp[8]= up.z();
    comp[9]= fov;
    comp[10]= hither;
    comp[11]= yon;
    bufsetup();
    netputnfloat( comp, 12 );
    netputnint( &parallel_flag, 1 );
    netsnd(VREN_SET_CAMERA);
  }
}

void netVRen::setCamera( const Camera& cam ) 
{
  setCamera( cam.from(), cam.at(), cam.updir(), cam.fov(), cam.hither_dist(),
	     cam.yon_dist(), cam.parallel_proj() );
}

void netVRen::transmit_quality_measure()
{
  float comp[2];
  comp[0]= current_quality->get_opacity_limit();
  comp[1]= current_quality->get_color_comp_error();
  int icomp[1];
  icomp[0]= current_quality->get_opacity_min();
  bufsetup();
  netputnfloat( comp, 2 );
  netputnint( icomp, 1 );
  netsnd(VREN_SET_QUALITY);
}

void netVRen::setQualityMeasure( const QualityMeasure& qual ) 
{
  delete current_quality;
  current_quality= new QualityMeasure(qual);
  if (out_vren) out_vren->setQualityMeasure( qual );
  else transmit_quality_measure();
}

void netVRen::setOpacLimit( float what ) 
{
  if (!current_quality) current_quality= new QualityMeasure;
  current_quality->set_opacity_limit(what);
  if (out_vren) out_vren->setOpacLimit(what);
  else transmit_quality_measure();
}

void netVRen::setColorCompError( float what ) 
{
  if (!current_quality) current_quality= new QualityMeasure;
  current_quality->set_color_comp_error(what);
  if (out_vren) out_vren->setColorCompError(what);
  else transmit_quality_measure();
}

void netVRen::setOpacMinimum( const int what ) 
{
  if (!current_quality) current_quality= new QualityMeasure;
  current_quality->set_opacity_min(what);
  if (out_vren) out_vren->setOpacMinimum(what);
  else transmit_quality_measure();
}

void netVRen::setLightInfo( const LightInfo& linfo_in ) 
{
  // Set the local copy
  delete current_light_info;
  current_light_info= new LightInfo( linfo_in );

  if (out_vren) out_vren->setLightInfo( linfo_in );
  else {
    // Set the partner\'s copy
    bufsetup();
    float comp[6];
    comp[0]= linfo_in.clr().r();
    comp[1]= linfo_in.clr().g();
    comp[2]= linfo_in.clr().b();
    comp[3]= linfo_in.amb().r();
    comp[4]= linfo_in.amb().g();
    comp[5]= linfo_in.amb().b();
    netputnfloat( comp, 6 );
    int icomp[4];
    icomp[0]= linfo_in.dir().ix();
    icomp[1]= linfo_in.dir().iy();
    icomp[2]= linfo_in.dir().iz();
    icomp[3]= linfo_in.dir().imag();
    netputnint( icomp, 4 );
    netsnd(VREN_SET_LIGHTINFO);
  }
}

void netVRen::setGeometry( VolGob *volgob ) 
{
  // Set the local copy
  delete current_geom;
  current_geom= new gobGeometry( volgob );

  if (out_vren) out_vren->setGeometry( volgob );
  else {
    // Set the partner\'s copy.  Cast works as long as this renderer
    // created the volgob.
    bufsetup();
    ((netVolGob *)volgob)->netputremoteobj();
    netsnd(VREN_SET_GEOMETRY);
  }
}

baseDataFile* netVRen::create_data_file( const char* fname_in, 
					 const DataFileType dftype_in )
{
  baseDataFile* result;
  if (out_vren) {
    result= out_vren->create_data_file( fname_in, dftype_in );
  }
  else {
    result= new netDataFile( fname_in, dftype_in, partner_rem() );
  }

  return result;
}

DataVolume *netVRen::create_data_volume( int xdim, int ydim, 
					 int zdim, const gBoundBox &bbox ) 
{
  netDataVolume *result;
  if (out_vren) 
    result= (netDataVolume *)
      (out_vren->create_data_volume( xdim, ydim, zdim, bbox ));
  else 
    result= new netDataVolume( xdim, ydim, zdim, bbox, NULL, partner_rem() );

  return result;
}

baseSampleVolume *netVRen::create_sample_volume( const GridInfo& grid_in,
						 baseTransferFunction& tfun,
						 const int ndatavol, 
						 DataVolume** data_table ) 
{
  if (out_vren) 
    return( out_vren->create_sample_volume( grid_in, tfun, ndatavol, 
					    data_table ) );
  else {
    netSampleVolume *result=
      new netSampleVolume( grid_in, tfun, ndatavol, data_table,
			   partner_rem() );
    result->construct();
    return(result);
  }
}

VolGob *netVRen::create_volgob( baseSampleVolume *vol, 
				const gTransfm& trans ) 
{
  if (out_vren) 
    return( out_vren->create_volgob( vol, trans ) ); 

  return( new netVolGob( vol, trans, partner_rem() ) );
}

baseTransferFunction *netVRen::register_tfun( baseTransferFunction *tfun_in )
{
  if (out_vren) return( out_vren->register_tfun(tfun_in) );
  else return( new netTransferFunction( tfun_in, partner_rem() ) );
}

void netVRen::update_and_go( const Camera& camera_in, 
			     const LightInfo& lights_in,
			     const int xsize, const int ysize )
{
  delete current_camera;
  current_camera= new Camera( camera_in );
  delete current_light_info;
  current_light_info= new LightInfo( lights_in );

  if (out_vren) 
    out_vren->update_and_go( camera_in, lights_in, xsize, ysize );
  else { // Ship info out to remote partner
    bufsetup();

    // Pack up the camera
    float cam_comp[12];
    int cam_int[1];
    cam_comp[0]= camera_in.from().x();
    cam_comp[1]= camera_in.from().y();
    cam_comp[2]= camera_in.from().z();
    cam_comp[3]= camera_in.at().x();
    cam_comp[4]= camera_in.at().y();
    cam_comp[5]= camera_in.at().z();
    cam_comp[6]= camera_in.updir().x();
    cam_comp[7]= camera_in.updir().y();
    cam_comp[8]= camera_in.updir().z();
    cam_comp[9]= camera_in.fov();
    cam_comp[10]= camera_in.hither_dist();
    cam_comp[11]= camera_in.yon_dist();
    netputnfloat( cam_comp, 12 );
    cam_int[0]= camera_in.parallel_proj();
    netputnint( cam_int, 1 );

    // Pack up the light info
    float light_comp[6];
    light_comp[0]= lights_in.clr().r();
    light_comp[1]= lights_in.clr().g();
    light_comp[2]= lights_in.clr().b();
    light_comp[3]= lights_in.amb().r();
    light_comp[4]= lights_in.amb().g();
    light_comp[5]= lights_in.amb().b();
    netputnfloat( light_comp, 6 );
    int ilight_comp[4];
    ilight_comp[0]= lights_in.dir().ix();
    ilight_comp[1]= lights_in.dir().iy();
    ilight_comp[2]= lights_in.dir().iz();
    ilight_comp[3]= lights_in.dir().imag();
    netputnint( ilight_comp, 4 );

    // Pack up the dimensions
    netputnint(&xsize, 1);
    netputnint(&ysize, 1);

    // And fire the message downstream
    netsnd( VREN_UPDATE_AND_GO );
  }

}

int netVRen::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {
  case IHANDLER_REQIHANDLER:
    {
      // The partner wants to connect to our ihandler.
      RemObjInfo rem;
      netgetreminfo(&rem);
      baseImageHandler *forgetme= new netImageHandler( &rem, ihandler );
      // Now we can talk to the ihandler directly, and the partner
      // can talk to it via the netImageHandler object.
    }
    break;
    
  case VREN_READY:
    {
      if (ready_proc) {
	(*ready_proc)( this, ready_cb_data );
      }
    }
    break;

  case VREN_ERROR:
    {
      int msgno;
      netgetnint( &msgno, 1 );
      error(msgno);
    }
    break;
    
  case VREN_FATAL:
    {
      int msgno;
      netgetnint( &msgno, 1 );
      fatal(msgno);
    }
    break;
    
  case VREN_RENDER:
    {
      int image_xdim, image_ydim;
      netgetnint( &image_xdim, 1 );
      netgetnint( &image_ydim, 1 );
      out_vren->StartRender( image_xdim, image_ydim );
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
      // At this point, the out_vren->StartRender has caused all ray
      // tracing on this processor to occur, and we are ready to start
      // reeling in images being returned from other procs
      netImageHandler::shmem_display_in_order();
#endif
    }
    break;
    
  case VREN_ABORT:
    out_vren->AbortRender();
    break;
    
  case VREN_SETOPTFLAGS:
    {
      netgetnint( &options, 1 );
      out_vren->setOptionFlags( options );
    }
    break;
    
  case VREN_SET_CAMERA:
    {
      float comp[12];
      int icomp[1];
      netgetnfloat( comp, 12 );
      netgetnint( icomp, 1 );
      setCamera( gPoint( comp[0], comp[1], comp[2] ),
		 gPoint( comp[3], comp[4], comp[5] ),
		 gVector( comp[6], comp[7], comp[8] ),
		 comp[9], comp[10], comp[11], icomp[0] );
    }
    break;
    
  case VREN_SET_QUALITY:
    {
      float comp[2];
      netgetnfloat( comp, 2 );
      int icomp[1];
      netgetnint( icomp, 1 );
      if (!current_quality) current_quality= new QualityMeasure;
      current_quality->set_opacity_limit( comp[0] );
      current_quality->set_color_comp_error( comp[1] );
      current_quality->set_opacity_min( icomp[0] );
      out_vren->setQualityMeasure( *current_quality );
    }
    break;
    
  case VREN_SET_LIGHTINFO:
    {
      float comp[6];
      netgetnfloat( comp, 6 );
      int icomp[4];
      netgetnint( icomp, 4 );
      delete current_light_info;
      current_light_info= new LightInfo( gColor(comp[0], comp[1], comp[2]),
					 gBVector(icomp[0], icomp[1],
						  icomp[2], icomp[3]),
					 gColor(comp[3], comp[4], comp[5]) );
      out_vren->setLightInfo(*current_light_info);
    }
    break;
    
  case VREN_SET_GEOMETRY:
    {
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: SET_GEOMETRY recieved at wrong end!\n");
	exit(-1);
      }
      netVolGob *netgob= (netVolGob *)netgetlocalobj();
      if (!netgob) {
	fprintf(stderr,"netVRen::netrcv: sent a foreign volgob!\n");
	exit(-1);
      }
      delete current_geom;
      current_geom= new gobGeometry( netgob->get_real_volgob() );
      out_vren->setGeometry( netgob->get_real_volgob() );
    }
    break;
    
  case VREN_UPDATE_AND_GO:
    {
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: UPDATE_AND_GO recieved at wrong end!\n");
	exit(-1);
      }
      // Unpack the camera
      float cam_comp[12];
      int cam_int[1];
      netgetnfloat( cam_comp, 12 );
      netgetnint( cam_int, 1 );
      gPoint cam_from( cam_comp[0], cam_comp[1], cam_comp[2] );
      gPoint cam_at( cam_comp[3], cam_comp[4], cam_comp[5] );
      gVector cam_up( cam_comp[6], cam_comp[7], cam_comp[8] );
      Camera cam( cam_from, cam_at, cam_up,
		  cam_comp[9], cam_comp[10], cam_comp[11], cam_int[0] );

      // Unpack the lights
      float light_comp[6];
      netgetnfloat( light_comp, 6 );
      int ilight_comp[4];
      netgetnint( ilight_comp, 4 );
      gColor lt_clr( light_comp[0], light_comp[1], light_comp[2] );
      gBVector lt_vec( ilight_comp[0], ilight_comp[1],
		      ilight_comp[2], ilight_comp[3] );
      gColor amb_clr( light_comp[3], light_comp[4], light_comp[5] );
      LightInfo lights( lt_clr, lt_vec, amb_clr );

      // Unpack the image dimensions
      int xsize, ysize;
      netgetnint( &xsize, 1 );
      netgetnint( &ysize, 1 );

      // Do the update and render
      update_and_go( cam, lights, xsize, ysize );

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
      // At this point, the out_vren->update_and_go has caused all ray
      // tracing on this processor to occur, and we are ready to start
      // reeling in images being returned from other procs
      netImageHandler::shmem_display_in_order();
#endif
    }
    break;

  case VREN_REMOTE_FILE_READABLE:
    {
      char* fname= netgetstring();
      rcv_ok= !access( fname, R_OK );
      delete [] fname;
    }
    break;
    
  case DATAFILE_REQDATAFILE:
    {
      RemObjInfo rem;
      char* fname;
      DataFileType dftype;
      netgetreminfo(&rem);
      char *info= netgetstring();
      netDataFile::unpack_param_info( info, &fname, dftype );
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: REQDATAFILE received at wrong end!\n");
	exit(-1);
      }
      baseDataFile *real_datafile= 
	out_vren->create_data_file( fname, dftype );
      netDataFile* result= new netDataFile( &rem, real_datafile );
    }
    break;

  case DATAVOLUME_REQDATAVOLUME:
    {
      RemObjInfo rem;
      int xin, yin, zin;
      gBoundBox bbox(0.0,0.0,0.0,1.0,1.0,1.0);
      netgetreminfo(&rem);
      char *info= netgetstring();
      netDataVolume::unpack_param_info( info, xin, yin, zin, bbox );
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: REQDATAVOLUME received at wrong end!\n");
	exit(-1);
      }
      DataVolume *real_datavol= 
	out_vren->create_data_volume( xin, yin, zin, bbox );
      DataVolume *result= new netDataVolume( &rem, real_datavol );
    }
    break;
    
  case SAMPLEVOLUME_REQSAMPLEVOLUME:
    {
      RemObjInfo rem;
      GridInfo grid_new;
      baseTransferFunction *tfun;
      int ndatavol;
      DataVolume **data_table= NULL;
      netgetreminfo(&rem);
      char *info= netgetstring();
      netSampleVolume::unpack_param_info( info, grid_new, &tfun, 
					  ndatavol, &data_table );
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: REQSAMPLEVOLUME recieved at wrong end!\n");
	exit(-1);
      }
      logger->comment("Constructing volume");
      baseSampleVolume *result= new netSampleVolume( &rem, 
						     grid_new, *tfun, 
						     ndatavol, data_table,
						     out_vren );
      delete data_table;  // allocated by unpack_param_info
    }
    break;
    
  case VOLGOB_REQVOLGOB:
    {
      RemObjInfo rem;
      netgetreminfo(&rem);
      char *info= netgetstring();
      baseSampleVolume *svol= NULL;
      gTransfm trans;
      netVolGob::unpack_param_info( info, trans, &svol );
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: REQVOLGOB recieved at wrong end!\n");
	exit(-1);
      }
      VolGob *real_vgob= 
	out_vren->create_volgob( svol, trans );
      VolGob *result= new netVolGob( &rem, real_vgob );
    }
    break;
    
  case TFUN_REQTFUN:
    {
      RemObjInfo rem;
      netgetreminfo(&rem);
      char *info= netgetstring();
      if (!out_vren) {
	// This should never happen, since there is noone to send us this msg
	fprintf(stderr,
		"netVRen::netrcv: REQTFUN received at wrong end!\n");
	exit(-1);
      }
      baseTransferFunction *result= new netTransferFunction( &rem, out_vren );
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

int netVRen::remote_file_readable( const char* fname )
{
  // This embarassing little utility is needed to help with loading 
  // remote files
  bufsetup();
  netputstring( fname );
  return( netsnd( VREN_REMOTE_FILE_READABLE ) );
}

VRenServer::VRenServer( char *arch ) 
: baseServer(VRenServerExeName, arch),
  baseNet( BASENET_REQSERVER, "", server_info )
{
  // The baseServer constructor fires up the executable, and the
  // baseNet constructor fires a requst to it which causes us to
  // get connected.
}

VRenServer::VRenServer( RemObjInfo *rem_in )
: baseNet(rem_in), baseServer()
{
  // Nothing left to be done
}

VRenServer::~VRenServer()
{
}

int VRenServer::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case VREN_REQVREN: // request for new vren- purpose of server
    {
      RemObjInfo remote;
      int type;
      netgetreminfo(&remote);
      char *info= netgetstring();
      netVRen::unpack_param_info( info, type );
#ifdef never
      fprintf(stderr,
	      "VRenServer::netrcv: sender= %x %d; info %d, partner %x %d\n",
	      remote.tid, remote.object, type,
	      partner_rem()->tid,partner_rem()->object);
#endif
      if ( remote != *partner_rem() ) {
	// Too many people to talk to to use ACK mechanism
	send_ack_and_break_lock();
      }
      baseVRen *vren= new netVRen( &remote, type );
    }
    break;
    
  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void VRenServer::initialize(const char *cmd)
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( cmd );
    netVRen::initialize( cmd );

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void VRenServer::handle_server_request()
// This routine catches the initial server request from the partner
{
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  VRenServer *vrenerver= new VRenServer(&rem);
}

void VRenServer::create_vren( RemObjInfo *rem_in, char *param_info )
// Create a partner for the netVRen at rem_in
{
  break_ack_lock(); // so we can send to server, which will ack to requestor
  bufsetup();
  netputreminfo(rem_in);
  netputstring( param_info );
  netsnd(VREN_REQVREN);
}

