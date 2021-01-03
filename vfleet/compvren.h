/****************************************************************************
 * compvren.h
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

class Compositor;

class compDataVolume: public DataVolume {
friend class compVRen;
public:
  ~compDataVolume(); // *** deletes children passed to constructor! ***
  void finish_init();
  void set_max_gradient( const float value, DataVolume *caller= NULL );
  int xsize() const { return real_xdim; }
  int ysize() const { return real_ydim; }
  int zsize() const { return real_zdim; }
  int nkids() const { return nchildren; }
  DataVolume* child( const int i ) const { return children[i]; }
protected:
  compDataVolume( int xdim_in, int ydim_in, int zdim_in,
		   const gBoundBox& bbox_in, 
		   int nchildren_in, DataVolume **children_in,
		   DataVolume *parent_in= NULL );
  int real_xdim;
  int real_ydim;
  int real_zdim;
  int nchildren;
  DataVolume **children;
  int kids_reporting_max_grad;
  int has_outside_max_grad;
  float outside_max_grad;
};

class compTransferFunction: public baseTransferFunction {
friend class compVRen;
public:
  ~compTransferFunction();
  TransferFunctionType type() const { return COMP_TFUN; }
  int nkids() const { return nchildren; }
  baseTransferFunction* child( const int i ) const { return children[i]; }
private:
  compTransferFunction( int ndata_in, int nchildren_in, 
			 baseTransferFunction **kids );
  int nchildren;
  baseTransferFunction **children;
};

class compSampleVolume: public baseSampleVolume {
friend class compVRen;
public:
  ~compSampleVolume();
  void regenerate( baseTransferFunction& tfun_in,
		   int ndatavol, DataVolume** data_table );
  int nkids() const { return nchildren; }
  baseSampleVolume* child( const int i ) const { return children[i]; }
  void set_size_scale( const float new_scale );
protected:
  compSampleVolume( const GridInfo& grid_in, 
		   baseTransferFunction& tfun_in,
		   int ndatavol, DataVolume** data_table,
		   const int nchildren_in );
  int nchildren;
  baseSampleVolume **children;
};

class compVolGob: public VolGob {
friend class compVRen;
public:
  ~compVolGob();
  int nkids() const { return nchildren; }
  VolGob* child( const int i ) const { return children[i]; }  
  void update_trans( const gTransfm& trans_in );
private:
  compVolGob( baseSampleVolume *vol_in, const gTransfm& trans,
	       int nchildren, baseVRen** child_rens );
  int nchildren;
  VolGob** children;
};

class compVRen: public baseVRen {
public:
  compVRen( const int nchildren_in, 
	   baseLogger *logger, baseImageHandler *imagehandler, 
	   void (*ready_handler)(baseVRen *renderer, void* cb_data),
	   void* ready_cb_data_in,
	   void (*error_handler)(int error_id, baseVRen *renderer), 
	   void (*fatal_handler)(int error_id, baseVRen *renderer),
	   void (*service_call)());
  compVRen( const int nchildren_in, 
	   baseLogger *logger, baseImageHandler *imagehandler, 
	   void (*ready_handler)(baseVRen *renderer, void* cb_data),
	   void* ready_cb_data_in,
	   baseVRen *owner,
	   void (*service_call)());
  ~compVRen(); 
  void setOptionFlags( const VRenOptions ops );
  void StartRender( int image_xdim, int image_ydim );
  void AbortRender();
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
  baseTransferFunction *register_tfun( baseTransferFunction *tfun_in );
  VolGob *create_volgob( baseSampleVolume *vol, const gTransfm& trans );
  void update_and_go( const Camera& camera_in,
		      const LightInfo& lights_in,
		      const int xsize, const int ysize );
protected:
  void (*service)();
  int nchildren;
  baseVRen **children;
  Compositor *compositor;
  static void ready_callback( baseVRen* renderer, void* cb_data );
  void handle_ready_child( baseVRen* renderer );
  int num_kids_ready;
};
