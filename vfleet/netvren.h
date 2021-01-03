/****************************************************************************
 * netvren.h
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

// Reference info for netDataVolume
const int datavolume_param_buf_size= 128;

class netDataVolume: public baseNet, public DataVolume {
friend class netVRen;
friend class netSampleVolume;
friend class DataVolumeServer;
public:
  ~netDataVolume(); // *** deletes real_dvol passed to constructor! ***
  void load_xplane( const DataType *data_in, int which_x );
  void load_yplane( const DataType *data_in, int which_y );
  void load_zplane( const DataType *data_in, int which_z );
  void finish_init();
  void load_datafile( baseDataFile* dfile_in );
  void set_max_gradient( const float value, DataVolume *caller= NULL );
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
  static char *pack_param_info( const int xin, const int yin, const int zin, 
				const gBoundBox& bbox_in);
  static void unpack_param_info( char *info, int& xin, int& yin, int& zin,
				 gBoundBox& bbox_in );
  int xsize() const { return real_xdim; }
  int ysize() const { return real_ydim; }
  int zsize() const { return real_zdim; }
  DataVolume *get_real_datavol() const { return dvol; }
protected:
  netDataVolume( RemObjInfo *rem_in, DataVolume *real_dvol );
  netDataVolume( int xdim_in, int ydim_in, int zdim_in, 
		 const gBoundBox& bbox_in, DataVolume *parent_in,
		 RemObjInfo* server= default_server );
  void netputdata( const DataType *databuf, int num ) 
  { netputbytes( (const char *)databuf, num ); }
  void netgetdata( DataType *databuf, int num )
  { netgetbytes( (char *)databuf, num ); }
  static char param_info_buf[ datavolume_param_buf_size ];
  static int initialized;
  static void handle_datavolume_request();
  DataVolume *dvol;
  int real_xdim;
  int real_ydim;
  int real_zdim;
};

/*
*
* DataVolume class has no public constructors, so it doesn't need its
* own server.  This server will work if the constructors are made public
* (I think).
*
*/
#ifdef never

// Reference info for DataVolumeServers
// Name is lower case because it must match info in command lines
const char DataVolumeServerExeName[]="datavolumeserver";

// This server produces netDataVolumes
class DataVolumeServer : public baseServer, public baseNet {
public:
  DataVolumeServer();
  DataVolumeServer( RemObjInfo *rem_in );
  ~DataVolumeServer();
  int netrcv(NetMsgType msg);
  static void initialize( const char *cmd );
  void create_data_volume( RemObjInfo *rem_in, char *param_info );
protected:
  static int initialized;
  static void handle_server_request();
  baseLogger *logger;
};

#endif

class netTransferFunction: public baseNet, public baseTransferFunction {
friend class netVRen;
friend class netSampleVolume;
public:
  ~netTransferFunction();
  TransferFunctionType type() const { return NET_TFUN; }
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
  static char *pack_param_info();
  static void unpack_param_info( const char *info );
  baseTransferFunction *get_real_tfun() const { return tfun; }
private:
  netTransferFunction( RemObjInfo *rem_in, baseVRen *generating_renderer );
  netTransferFunction( baseTransferFunction *tfun_in, 
		       RemObjInfo *server= default_server );
  static int param_buf_size;
  static char *param_info_buf;
  static int initialized;
  static void handle_tfun_request();
  void netput_tfun( baseTransferFunction *tfun );
  baseTransferFunction *netget_tfun();
  baseTransferFunction *tfun;
  baseVRen *renderer;
  int delete_tfun_on_delete;
};

class netSampleVolume: public baseNet, public baseSampleVolume {
friend class netVRen;
public:
  ~netSampleVolume();
  void regenerate( baseTransferFunction& tfun_in,
		   int ndatavol, DataVolume** data_table );
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
  static char *pack_param_info( const GridInfo& grid_in, 
				baseTransferFunction& tfun_in,
				int ndatavol, DataVolume **data_table );
  // unpack_param_info allocates memory for data_table
  static void unpack_param_info( char *info, GridInfo& grid_out, 
				 baseTransferFunction** tfun_in,
				 int& ndatavol, DataVolume*** data_table );
  baseSampleVolume *get_real_svol() const { return svol; }
  void set_size_scale( const float new_scale );
private:
  netSampleVolume( RemObjInfo *rem_in,
		   const GridInfo& grid_in, baseTransferFunction& tfun_in,
		   int ndatavol, DataVolume** data_table,
		   baseVRen *owner_in );
  netSampleVolume( const GridInfo& grid_in, baseTransferFunction& tfun_in,
		   int ndatavol, DataVolume** data_table,
		   RemObjInfo *server= default_server );
  void construct();  // Finish building- allows construction in parallel
  static int param_buf_size;
  static char *param_info_buf;
  static int initialized;
  static void handle_samplevolume_request();
  baseSampleVolume *svol;
  baseVRen *owner;
};

// netSampleVolume has no public constructors, and hence needs no server

class netVolGob: public baseNet, public VolGob {
friend class netVRen;
friend class netSampleVolume;
public:
  ~netVolGob();
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
  static char *pack_param_info( const gTransfm& trans, 
				netSampleVolume *svol );
  static void unpack_param_info( char *info, gTransfm& trans, 
				 baseSampleVolume** svol );
  VolGob *get_real_volgob() const { return volgob; }
  void update_trans( const gTransfm& trans_in );
private:
  netVolGob( RemObjInfo *rem_in, VolGob *real_vol );
  netVolGob( baseSampleVolume *vol_in, const gTransfm& trans,
		   RemObjInfo *server= default_server );
  static int param_buf_size;
  static char *param_info_buf;
  static int initialized;
  static void handle_volgob_request();
  VolGob *volgob;
};

// netVolGob has no public constructors, and hence needs no server

// Reference info for netVRen
const int vren_param_buf_size= 32;

class netVRen: public baseNet, public baseVRen {
friend class VRenServer;
public:
  netVRen( RemObjInfo *rem_in, const int type );
  netVRen( const int type, 
	   baseLogger *logger_in, baseImageHandler *imagehandler, 
	   void (*ready_handler)(baseVRen *renderer, void* cb_data),
	   void* ready_cb_data_in,
	   void (*error_handler)(int error_id, baseVRen *renderer), 
	   void (*fatal_handler)(int error_id, baseVRen *renderer) );
  netVRen( const int type,
	   baseLogger *logger_in, baseImageHandler *imagehandler, 
	   void (*ready_handler)(baseVRen *renderer, void* cb_data),
	   void* ready_cb_data_in,
	   baseVRen *owner );
  ~netVRen();
  void setOptionFlags( const VRenOptions ops );
  void StartRender( int image_xdim, int image_ydim );
  void AbortRender();
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
  static char *pack_param_info( const int type);
  static void unpack_param_info( const char *info, int &type );
  void error( int error_id );
  void fatal( int error_id );
  void setCamera( const gPoint& lookfm, const gPoint& lookat, 
		  const gVector& up, const float fov, 
		  const float hither, const float yon,
		  const int parallel_flag );
  void setCamera( const Camera& cam );
  void setQualityMeasure( const QualityMeasure& qual );
  void setOpacLimit( float what );
  void setColorCompError( float what );
  void setOpacMinimum( const int what );
  void setLightInfo( const LightInfo& linfo_in );
  void setGeometry( VolGob *volgob );
  baseDataFile* create_data_file( const char* fname_in, 
				  const DataFileType dftype_in );
  DataVolume *create_data_volume( int xdim, int ydim, int zdim, 
				  const gBoundBox &bbox );
  baseSampleVolume *create_sample_volume( const GridInfo& grid_in,
					  baseTransferFunction& tfun,
					  const int ndatavol, 
					  DataVolume** data_table );
  VolGob *create_volgob( baseSampleVolume *vol, const gTransfm& trans );
  baseTransferFunction *register_tfun( baseTransferFunction *tfun_in );
  void update_and_go( const Camera& camera_in, const LightInfo& lights_in,
		      const int xsize, const int ysize );
  int remote_file_readable( const char* fname );
  static int encode_type( const int split_plane, const int num_procs, 
			  const int num_threads );
  static void decode_type( const int type, int* split_plane, int* num_procs,
			   int* num_threads );
protected:
  static int initialized;
  static void handle_netvren_request();
  static char param_info_buf[ vren_param_buf_size ];
  baseVRen *out_vren;
  int created_own_logger;
  void transmit_quality_measure();
  static void ready_callback( baseVRen* renderer, void* cb_data );
  void handle_ready( baseVRen* renderer );
};

// Reference info for VRenServers
// Name is lower case because it must match info in command lines
extern const char VRenServerExeName[];

// This server produces netVRens
class VRenServer : public baseServer, public baseNet {
public:
  VRenServer( char *arch= NULL );
  VRenServer( RemObjInfo *rem_in );
  ~VRenServer();
  int netrcv(NetMsgType msg);
  static void initialize( const char *cmd );
  void create_vren( RemObjInfo *rem_in, char *param_info );
protected:
  static int initialized;
  static void handle_server_request();
};

