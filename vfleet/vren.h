/****************************************************************************
 * Vren.h
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

#include "geometry.h"
#include "camera.h"
#include "basedatafile.h"

class baseLogger;
class baseImageHandler;
class rgbImage;
class baseSampleVolume;
class baseTransferFunction;

typedef int VRenOptions;

const unsigned int OPT_SHOW_INTERMEDIATE_IMAGES= 1;
const unsigned int OPT_USE_LIGHTING= 2;
const unsigned int OPT_FAST_LIGHTING= 4;
const unsigned int OPT_FAST_DISTANCES= 8;
const unsigned int OPT_TRILINEAR_INTERP= 16;
const unsigned int OPT_SPECULAR_LIGHTING= 32;
const unsigned int OPT_THREED_MIPMAP= 64;

enum VRenErrorID {
  VRENERROR_CAMERA_NOT_SET,
  VRENERROR_QUALITY_NOT_SET,
  VRENERROR_LIGHT_INFO_NOT_SET,
  VRENERROR_GEOM_NOT_SET,
  VRENERROR_LAST_ERROR
};

/*
*
*  Note to implementors- if you change the type of DataType below,
*  you will also have to change the methods within the netDataVolume
*  and sliceDataVolume classes.
*
*/
typedef unsigned char DataType;
const DataType DataTypeMax= 255;
const DataType DataTypeMin= 0;

class Ray;
class LightInfo;

class GridInfo {
public:
  GridInfo();
  GridInfo( const int xdim_in, const int ydim_in, const int zdim_in,
	    const gBoundBox& bbox_in );
  GridInfo( const GridInfo& o ) 
    : boundbox( o.boundbox )
  { 
    xdim= o.xdim; ydim= o.ydim; zdim= o.zdim;
    deltax= o.deltax; deltay= o.deltay; deltaz= o.deltaz;
  }
  int xsize() const { return xdim; }
  int ysize() const { return ydim; }
  int zsize() const { return zdim; }
  float dx() const { return deltax; }
  float dy() const { return deltay; }
  float dz() const { return deltaz; }
  const gBoundBox& bbox() const { return boundbox; }
  GridInfo& operator=( const GridInfo& o )
  {
    if (this != &o) {
      xdim= o.xdim; ydim= o.ydim; zdim= o.zdim;
      deltax= o.deltax; deltay= o.deltay; deltaz= o.deltaz;
      boundbox= o.boundbox;
    }
    return *this;
  }
  int operator==( const GridInfo& o ) const
  { return( (bbox()==o.bbox()) && (xsize()==o.xsize())
	    && (ysize()==o.ysize()) && (zsize()==o.zsize()) ); }
  int operator!=( const GridInfo& o ) const
  { return( (bbox()!=o.bbox()) || (xsize()!=o.xsize())
	    || (ysize()!=o.ysize()) || (zsize()!=o.zsize()) ); }
private:
  gBoundBox boundbox;
  int xdim;
  int ydim;
  int zdim;
  float deltax;
  float deltay;
  float deltaz;
};

class DataVolume {
public:
  DataVolume( int xdim_in, int ydim_in, int zdim_in, 
	      const gBoundBox& bbox_in, DataVolume *parent_in= NULL );
  virtual ~DataVolume();
  virtual void load_xplane( const DataType *data_in, int which_x );
  virtual void load_yplane( const DataType *data_in, int which_y );
  virtual void load_zplane( const DataType *data_in, int which_z );
  virtual void finish_init();
  virtual void load_datafile( baseDataFile* dfile_in );
  virtual DataType val( const int i, const int j, const int k ) const 
  { return( *(access(i,j,k)) ); }
  float fval( const int i, const int j, const int k ) const
  { return( ((float)(val(i,j,k) - DataTypeMin))
	    / (DataTypeMax-DataTypeMin) ); }
  virtual gVector gradient( const int i, const int j, const int k ) const;
  virtual int xsize() const { return grid.xsize(); }
  virtual int ysize() const { return grid.ysize(); }
  virtual int zsize() const { return grid.zsize(); }
  const gBoundBox& boundbox() const { return grid.bbox(); }
  GridInfo gridinfo() // some derived classes fake dims, so must make a grid
    { return GridInfo( xsize(), ysize(), zsize(), grid.bbox()); }
  float max_gradient_magnitude() const { return max_grad_mag; }
  virtual void set_max_gradient( const float value, DataVolume *caller= NULL );
  DataVolume *get_parent() const { return parent; }
  void set_parent( DataVolume *parent_in ) { parent= parent_in; }
  virtual void prep_xplane( const int which_x ) {}
  virtual void prep_yplane( const int which_y ) {}
  virtual void prep_zplane( const int which_z ) {}
protected:
  DataType *data;
  GridInfo grid;
  DataType *access( const int i, const int j, const int k ) const 
  { return(data + ((i*ysize()+j)*zsize()+k)); }
  float deriv_forwards( const float v1, const float v2, const float v3,
			const float step ) const
  { return( (-0.5*v3 +2.0*v2 -1.5*v1)/step ); }
  float deriv_centered( const float v1, const float v2, 
			const float step ) const
  { return( (v2-v1)/(2.0*step) ); }
  float max_grad_mag;
  float find_max_grad_mag();
  DataVolume *parent;
};

class QualityMeasure {
public:
  QualityMeasure()
  { opacity_limit= color_comp_error= opacity_min= 0; }
  QualityMeasure( float opac_lim_in, float color_comp_in, int opac_min_in )
  { 
    opacity_limit= (unsigned char)(255*opac_lim_in + 0.5);
    color_comp_error= (unsigned char)(255*color_comp_in + 0.5);
    opacity_min= (unsigned char)opac_min_in;
  }
  QualityMeasure( int opac_lim_in, int color_comp_in, int opac_min_in )
  {
    opacity_limit= opac_lim_in;
    color_comp_error= color_comp_in;
    opacity_min= (unsigned char)opac_min_in;
  }
  QualityMeasure( const QualityMeasure& other );
  QualityMeasure& operator=( const QualityMeasure& other );
  void clear() { opacity_limit= color_comp_error= opacity_min= 0; }
  int opacity_test( const float val ) const
  { return( (int)(255*val + 0.5) < opacity_limit ); }
  int opacity_precise() const { return( color_comp_error==0 ); }
  int better_than( const QualityMeasure& other ) const
  { return( (color_comp_error <= other.color_comp_error) 
	    && (opacity_min <= other.opacity_min) ); }
  void update_with_greater( const QualityMeasure& other )
  {
    if (other.opacity_limit > opacity_limit) 
      opacity_limit= other.opacity_limit;
    if (other.color_comp_error > color_comp_error)
      color_comp_error= other.color_comp_error;
    if (other.opacity_min > opacity_min) 
      opacity_min= other.opacity_min;
  }
  void add_max_deviation( gBColor& clr, gBColor& ref_clr, 
			  const QualityMeasure& qual );
  void set_opacity_limit(float opac_lim_in) {
    opacity_limit= (unsigned char)(255*opac_lim_in + 0.5);
  }
  void set_color_comp_error(float color_comp_in) {
    color_comp_error= (unsigned char)(255*color_comp_in + 0.5);
  }
  void set_opacity_min( const int opacity_min_in )
    { opacity_min= (unsigned char)opacity_min_in; }
  float get_opacity_limit() const { return( (1.0/255) * opacity_limit ); }
  float get_color_comp_error() const { return((1.0/255) * color_comp_error); }
  int get_color_comp_ierr() const { return color_comp_error; }
  int get_opacity_min() const { return opacity_min; }
private:
  unsigned char opacity_limit;
  unsigned char color_comp_error;
  unsigned char opacity_min;
};

class Sample {
public:
  Sample() { grad_data[0]= grad_data[1]= grad_data[2]= 128; }
  gBColor clr;
  int grad_ix() const { return grad_data[0] - 128; }
  int grad_iy() const { return grad_data[1] - 128; }
  int grad_iz() const { return grad_data[2] - 128; }
  float grad_x() const { return (float)(grad_ix()/127.0); };
  float grad_y() const { return (float)(grad_iy()/127.0); };
  float grad_z() const { return (float)(grad_iz()/127.0); };
  void set_grad_ix( int ix ) { grad_data[0]= ix + 128; }
  void set_grad_iy( int iy ) { grad_data[1]= iy + 128; }
  void set_grad_iz( int iz ) { grad_data[2]= iz + 128; }
  int grad_is_zero() 
  { return ((grad_data[0]==128) && (grad_data[1]==128) 
	    && (grad_data[2]==128)); }
  int max_grad_comp()
  {
    int result= (grad_data[0] > 128) ? grad_data[0]-128 : 128 - grad_data[0];
    int tmp= (grad_data[1] > 128) ? grad_data[1]-128 : 128 - grad_data[1];
    if (tmp>result) result= tmp;
    tmp= (grad_data[2] > 128) ? grad_data[2]-128 : 128 - grad_data[2];
    if (tmp>result) result= tmp;
    return result;
  }
protected:
  unsigned char grad_data[3];
  unsigned char spare_byte;
};

enum TransferFunctionType {
  BASE_TFUN, 
  BBOX_TFUN, 
  SUM_TFUN, 
  TABLE_TFUN, 
  METHOD_TFUN, 
  NET_TFUN, 
  COMP_TFUN,
  GRADTABLE_TFUN,
  SSUM_TFUN,
  MASK_TFUN,
  BLOCK_TFUN,
  LAST_TFUN
};  // Insert new transfer function types before this line

class baseTransferFunction {
 public:
  baseTransferFunction( int ndata_in ) { ndatavol= ndata_in; }
  baseTransferFunction( const baseTransferFunction& other ) 
    { ndatavol= other.ndatavol; }
  virtual ~baseTransferFunction() {};
  virtual TransferFunctionType type() const { return BASE_TFUN; }
  virtual baseTransferFunction* copy();
  virtual void apply( Sample &sample, int i, int j, int k, 
		      baseSampleVolume *svol,
		      int ndata_in, DataVolume** data_table )
  { 
    fprintf(stderr, "baseTransferFunction::apply was accidentally called.\n"); 
  }
  virtual void apply_row (Sample *samples, int i, int j, int kmin, int kmax, 
		     baseSampleVolume *svol, int ndata_in, 
		     DataVolume **data_table)
  {
    fprintf(stderr, 
	    "baseTransferFunction::apply_row was accidentally called.\n");
  }
  int ndata() const { return ndatavol; }
protected:
  int ndatavol;
};

class baseSampleVolume {
public:
  baseSampleVolume( const GridInfo& grid_in, baseTransferFunction& tfun_in,
		    int ndatavol, DataVolume** data_table );
  virtual ~baseSampleVolume();
  virtual void regenerate( baseTransferFunction& tfun_in,
			   int ndatavol, DataVolume** data_table );
  int xsize() const { return grid.xsize(); }
  int ysize() const { return grid.ysize(); }
  int zsize() const { return grid.zsize(); }
  const gBoundBox& boundbox() { return grid.bbox(); }
  const GridInfo& gridinfo() const { return grid; }
  baseTransferFunction *transferfunction() const { return tfun; }
  int ndata() const { return num_datavols; }
  DataVolume **data_table() { return datavols; }
  virtual void set_size_scale( const float new_scale );
  float get_size_scale() const;
  const gVector& get_inv_voxel_aspect_ratio() const 
    { return inv_voxel_aspect_ratio; };
protected:
  baseSampleVolume( const baseSampleVolume& other );
  GridInfo grid;
  int num_datavols;
  DataVolume **datavols;
  baseTransferFunction *tfun;
  float size_scale;
  gVector inv_voxel_aspect_ratio; // for rescaling sample gradients
};

class LightInfo {
public:
  LightInfo( const gColor& ltcolor_in, const gBVector& ltdir_in, 
	     const gColor& ambclr_in );
  LightInfo( const LightInfo& other );
  LightInfo& operator=( const LightInfo& other );
  ~LightInfo();
  gColor clr() const { return ltcolor; }
  gBVector dir() const { return ltdir; }
  gColor amb() const { return ambclr; }
  void reset_lighting_tables();  // set fast lighting table based on this info
  void set_viewdir( const gVector& viewing_direction )
  { viewdir= viewing_direction; }
  void set_central_viewdir( const gVector& viewing_direction )
  { central_viewdir= viewing_direction; }
  gColor calc_light_clr( const gBVector neg_normal, const int debug= 0 );
  gColor calc_specular_clr( const gBVector neg_normal, const int debug= 0 );
  gColor calc_specular_clr_central_dir( const gBVector neg_normal );
  gBColor& get_fast_light_clr( const gBVector neg_normal )
  { 
    int index= ((neg_normal.ix() & 248) << 7
		| ((neg_normal.iy() & 248) << 2) 
		| ((neg_normal.iz() & 248) >> 3));
    if (!fast_lighting_table[index].ia()) // slot not valid
      fast_lighting_table[ index ]= calc_light_clr( neg_normal );
    return fast_lighting_table[index];
  }
  gBColor& get_fast_specular_clr( const gBVector neg_normal )
  {
    int index= ((neg_normal.ix() & 248) << 7
		| ((neg_normal.iy() & 248) << 2) 
		| ((neg_normal.iz() & 248) >> 3));
    if (!fast_specular_table[index].ia()) // slot not valid
      fast_specular_table[ index ]= calc_specular_clr_central_dir(neg_normal);
    return fast_specular_table[index];      
  }
  int operator==( const LightInfo& other ) const
  { return ((ltcolor == other.ltcolor) && (ltdir == other.ltdir)
	    && (ambclr == other.ambclr) 
	    && (central_viewdir == other.central_viewdir)); }
  int operator!=( const LightInfo& other ) const
  { return !((ltcolor == other.ltcolor) && (ltdir == other.ltdir)
	     && (ambclr == other.ambclr)
	     && (central_viewdir == other.central_viewdir)); }
private:
  gColor ltcolor;
  gBVector ltdir;
  gColor ambclr;
  gVector viewdir;
  gVector central_viewdir; // used in fast specular calculation
  static gBColor* fast_lighting_table;
  static gBColor* fast_specular_table;
  static LightInfo fast_lighting_info;
  static float* specular_curve;
};

class VolGob {
public:
  VolGob( baseSampleVolume *vol_in, const gTransfm& trans );
  virtual ~VolGob();
  baseSampleVolume *samplevolume() const { return vol; }
  const gTransfm& transform() const { return trans; }
  virtual void update_trans( const gTransfm& trans_in ) { trans= trans_in; }
protected:
  baseSampleVolume *vol;
  gTransfm trans;
};

class gobGeometry {
public:
  gobGeometry( VolGob *volgob_in );
  ~gobGeometry();
  VolGob *volgob() const { return myvolgob; }
private:
  VolGob *myvolgob;
  unsigned char garbage;  // this prevents a malloc bug on some compilers
                          // by forcing longword alignment
// Gob *world;  // primitive-based system goes here  
};

class baseVRen {
public:
  baseVRen( baseLogger *logger_in, baseImageHandler *imagehandler, 
	    void (*ready_handler)(baseVRen *renderer, void* cb_data),
	    void* ready_cb_data_in,
	    void (*error_handler)(int error_id, baseVRen *renderer), 
	    void (*fatal_handler)(int error_id, baseVRen *renderer) );
  baseVRen( baseLogger *logger_in, baseImageHandler *imagehandler, 
	    void (*ready_handler)(baseVRen *renderer, void* cb_data),
	    void* ready_cb_data_in,
	    baseVRen *owner_in );
  virtual ~baseVRen(); 
  virtual void setOptionFlags( const VRenOptions ops );
  virtual void StartRender( int image_xdim, int image_ydim );
  VRenOptions getOptionFlags() const { return options; }
  virtual void AbortRender();
  virtual void error( int error_id );
  virtual void fatal( int error_id );
  virtual void setCamera( const gPoint& lookfm, const gPoint& lookat, 
			  const gVector& up, const float fov, 
			  const float hither, const float yon,
			  const int parallel_flag );
  virtual void setCamera( const Camera& cam );
  virtual void setQualityMeasure( const QualityMeasure& qual );
  virtual void setOpacLimit( float what );
  virtual void setColorCompError( float what );
  virtual void setOpacMinimum( const int what );
  virtual void setLightInfo( const LightInfo& linfo_in );
  virtual void setGeometry( VolGob *volgob );
  virtual baseDataFile* create_data_file( const char* fname_in, 
					  const DataFileType dftype_in );
  virtual DataVolume *create_data_volume( int xdim_in, int ydim_in, 
					  int zdim_in, 
					  const gBoundBox &bbox_in );
  virtual baseSampleVolume *create_sample_volume( const GridInfo& grid_in,
					         baseTransferFunction& tfun_in,
						  const int ndatavol, 
						  DataVolume** data_table );
  virtual VolGob *create_volgob( baseSampleVolume *vol_in, 
				 const gTransfm& trans );
  virtual baseTransferFunction *register_tfun( baseTransferFunction *tfun_in );
  QualityMeasure *quality() { return(current_quality); };
  virtual void update_and_go( const Camera& camera_in,
			      const LightInfo& lights_in,
			      const int xsize, const int ysize );
protected:
  baseVRen *owner;
  baseLogger *logger;
  baseImageHandler *ihandler;
  rgbImage *image;
  void (*ready_proc)(baseVRen *renderer, void* cb_data);
  void* ready_cb_data;
  void (*error_proc)(int error_id, baseVRen *renderer);
  void (*fatal_proc)(int error_id, baseVRen *renderer);
  VRenOptions options;
  Camera *current_camera;
  QualityMeasure *current_quality;
  LightInfo *current_light_info;
  gobGeometry *current_geom;
};

