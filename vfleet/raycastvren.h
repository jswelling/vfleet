/****************************************************************************
 * raycastvren.h
 * Author Joel Welling, Rob Earhart
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

#include "octree.h"

#ifndef AVOID_THREADS
#include "jthreads.h"
#endif

class octSample : public Sample {
public:
  octSample() { spare_byte= 0; }
  unsigned char clr_error() { return spare_byte; }
  void set_clr_error( const unsigned char value ) { spare_byte= value; }
  inline gColor calc_color_o0_inline( LightInfo& lights, 
				       const VRenOptions options, 
				       const int debug=0 );
  gColor calc_color_o0( LightInfo& lights, 
			const VRenOptions options, 
			const int debug= 0 );
private:
  static const float EXP_APPROX_ACCURACY;
};

void octSample_calc_values( octSample *cell,
			    const Octcell_kids_iter<octSample>& iter );
void octSample_average_color_and_grad( octSample *cell,
				       Octcell_kids_iter<octSample>& iter );

struct Ray {
  gPoint origin;
  gVector direction;
  float length;
  gColor clr;
  int image_x, image_y;
  QualityMeasure qual;
  float termination_length;
  gColor termination_color;
  float initial_separation;
  float divergence;
  char active;
  char debug_me;
  char rescale_me;
};

class TrilinWalker {
public:
  TrilinWalker( Octree<octSample>* tree_in ) { 
    sample_tree= tree_in;
    level= 0;
    i_llf= j_llf= k_llf= -1;
    i_trb= j_trb= k_trb= -1;
    level_old= 0;
    i_llf_old= j_llf_old= k_llf_old= -1;
    i_trb_old= j_trb_old= k_trb_old= -1;
  }
  inline gColor trilinear_color( const int level, const int i, const int j, 
				  const int k, const gVector& offset,
				  LightInfo& lights, const VRenOptions options,
				  const int debug= 0 ); // clr scaled by alpha!
private:
  void recalc_trilin( const int level, const int i_llf_in, const int j_llf_in, 
		     const int k_llf_in, const int i_trb_in, 
		     const int j_trb_in, const int k_trb_in, 
		     LightInfo& lights, 
		     const VRenOptions options, const int debug );
  Octree<octSample> *sample_tree;  // doesn't allocate the tree
  gColor c000;
  gColor c001;
  gColor c010;
  gColor c011;
  gColor c100;
  gColor c101;
  gColor c110;
  gColor c111;
  int level;
  int i_llf, j_llf, k_llf;
  int i_trb, j_trb, k_trb;
  int level_old;
  int i_llf_old, j_llf_old, k_llf_old;
  int i_trb_old, j_trb_old, k_trb_old;
  gColor c000_old;
  gColor c001_old;
  gColor c010_old;
  gColor c011_old;
  gColor c100_old;
  gColor c101_old;
  gColor c110_old;
  gColor c111_old;
};

class octreeSampleVolume
 : public baseSampleVolume {
  friend class Sample;
  friend class raycastVRen;
public:
  struct init_info { 
      baseTransferFunction* transfer;
      baseSampleVolume *svol;
      int ndatavol;
      DataVolume** data_table;
  };
  ~octreeSampleVolume();
  void regenerate( baseTransferFunction& tfun_in,
		   int ndatavol, DataVolume** data_table );
  void intersect( Ray *ray_queue, const int nrays,
		  LightInfo& lights,
		  const QualityMeasure& qual,
		  const VRenOptions options );
  void intersect( Ray *ray_queue, const int nrays,
		  LightInfo& lights,
		  const QualityMeasure& qual,
		  const VRenOptions options,
		  Octcell_intersect_iter<octSample>* iter);
  const Octree<octSample>* get_tree() const { return sample_tree; }
private:
  octreeSampleVolume( const GridInfo& grid_in, baseTransferFunction& tfun_in,
		     int ndatavol, DataVolume** data_table, int nthreads_in );
  Octree<octSample> *sample_tree;
  void calc_tree_vals();
  void calc_leaf_gradients();
  void smooth_leaf_gradients();
  void calc_leaf_errors();
  inline void integrate_cell_o0( Ray& ray, const gColor& cell_clr,
				const float dist )
  {
    float effective_opac= 1.0 - exp( -dist * cell_clr.a() );
    gColor color_incr( cell_clr.r()*effective_opac,
		       cell_clr.g()*effective_opac,
		       cell_clr.b()*effective_opac,
		       effective_opac );
    color_incr.clamp();
    ray.clr.add_under( color_incr );
  }
  void walk_one_ray_mm( Ray& ray, LightInfo& lights, 
			const QualityMeasure& qual,
			const VRenOptions options,
			Octcell_intersect_iter<octSample>& iter );
  void walk_one_ray( Ray& ray, LightInfo& lights, 
		     const QualityMeasure& qual,
		     const VRenOptions options,
		     Octcell_intersect_iter<octSample>& iter );
  inline void step_trilin( gColor* clr, const int level,
			  const int i, const int j, const int k,
			  LightInfo& lights, const VRenOptions options,
			  const gVector& start, const gVector& end,
			  const float step_dist, TrilinWalker& twalker,
			  const int debug );
  void step_trilin_subdiv( gColor* clr, const int level,
			  const int i, const int j, const int k,

			  LightInfo& lights, const VRenOptions options,
			  const gVector& start, const gVector& end,
			  const float step_dist, const gColor& prev_guess, 
			  const int recur_depth, const float tol,
			  TrilinWalker& twalker, const int debug );
  void integrate_cell_trilin( gColor& clr, 
			      Octcell_intersect_iter<octSample>& iter,
			      LightInfo& lights, 
			      const VRenOptions options,
			      const float dist, const float tol,
			      TrilinWalker& twalker, const int debug= 0 );
  void walk_one_ray_trilin_mm( Ray& ray, LightInfo& lights, 
			       const QualityMeasure& qual,
			       const VRenOptions options,
			       Octcell_intersect_iter<octSample>& iter );
  void walk_one_ray_trilin( Ray& ray, LightInfo& lights, 
			   const QualityMeasure& qual,
			   const VRenOptions options,
			   Octcell_intersect_iter<octSample>& iter );
  Octcell_intersect_iter<octSample>* oct_iter;
  float voxel_mean_size; // used in 3D mipmapping
  int nthreads;
};

class rayCamera : public Camera {
  friend class raycastVRen;
public:
  rayCamera( const Camera& other );
  ~rayCamera();
  rayCamera& operator=( const rayCamera& other );
  void initdims( rgbImage& image );
  void initialize_ray( Ray *ray, rgbImage& image, int i, int j );
private:
  rayCamera( const gPoint lookfm_in, const gPoint lookat_in, 
	     const gVector up_in, const float fovea_in, 
	     const float hither_in, const float yon_in,
	     const int parallel_flag_in=0 );
  rayCamera( const rayCamera& other );
  gVector firstray; // direction from lookfm to top left corner
  gPoint par_firstpoint; // starting point for top left parallel proj ray
  gVector screenx;  // image x axis
  gVector screeny;  // image y axis
  gVector screeni;  // normalized screenx
  gVector screenj;  // normalized screeny
  int image_xdim;
  int image_ydim;
  float ray_divergence;
  float par_ray_sep;
};

class raycastVRen: public baseVRen {
public:
  raycastVRen( baseLogger *logger, baseImageHandler *imagehandler, 
	       void (*ready_handler)(baseVRen *renderer, void* cb_data),
	       void* ready_cb_data_in,
	       void (*error_handler)(int error_id, baseVRen *renderer), 
	       void (*fatal_handler)(int error_id, baseVRen *renderer),
	       void (*service_call)(), const int nthreads_in );
  raycastVRen( baseLogger *logger, baseImageHandler *imagehandler, 
	       void (*ready_handler)(baseVRen *renderer, void* cb_data),
	       void* ready_cb_data_in,
	       baseVRen *owner,
	       void (*service_call)(), const int nthreads_in );
  ~raycastVRen(); 
  void StartRender( int image_xdim, int image_ydim );
  void AbortRender();
  void setCamera( const gPoint& lookfm, const gPoint& lookat, 
		  const gVector& up, const float fov, 
		  const float hither, const float yon,
		  const int parallel_flag );
  void setCamera( const Camera& cam );
  baseSampleVolume *create_sample_volume( const GridInfo& grid_in,
					  baseTransferFunction& tfun_in,
					  const int ndatavol, 
					  DataVolume** data_table );
  void TraceOneRay( int x, int y, char debug = 0) {
      Ray trace_me;
      ((rayCamera *)current_camera)->initialize_ray(&trace_me, *image, 
						    x, y);
      trace_me.origin= inv_obj_rotation * trace_me.origin;
      trace_me.direction= inv_obj_rotation * trace_me.direction;
      trace_me.debug_me = debug;
      ((octreeSampleVolume *)(current_geom->volgob()->samplevolume()))
	->intersect(&trace_me, 1, *rotated_lights, *current_quality,
		    options) ;
      image->setpix( x, image->ysize()-(y+1), trace_me.clr );
  };
  void TraceOneRay( const int x, const int y, 
		    Octcell_intersect_iter<octSample>* iter,
		    char debug = 0) {
      Ray trace_me;
      ((rayCamera *)current_camera)->initialize_ray(&trace_me, *image, 
						    x, y);
      trace_me.origin= inv_obj_rotation * trace_me.origin;
      trace_me.direction= inv_obj_rotation * trace_me.direction;
      trace_me.debug_me = debug;
      ((octreeSampleVolume *)(current_geom->volgob()->samplevolume()))
	->intersect(&trace_me, 1, *rotated_lights, *current_quality,
		    options, iter) ;
      image->setpix_nosideeffects( x, image->ysize()-(y+1), trace_me.clr );
  };
  void do_service() { if (service) (*service)(); }
  int render_in_progress() const { return render_run; }
  void add_span( int istart, int iend, int j ) {
    if ((++span_table_top)>=span_table_size) grow_span_table();
    Span* s= span_table+span_table_top-1;
    s->pending= 1;
    if (istart<=iend) {
      s->i_start= istart;
      s->i_end= iend;
    }
    else {
      s->i_start= iend;
      s->i_end= istart;
    }
    s->j= j;
  }
  int thread_count() const { return nthreads; }
private:
  struct Span {
    int pending;
    int j;
    int i_start;
    int i_end;
  };
  Span* span_table;
  int span_table_size;
  int span_table_top;
  void grow_span_table();
  void clear_span_table();
  void start_tracer_threads();
  void await_tracer_threads();
  int spans_being_generated;
  void (*service)();
  LightInfo* rotated_lights;
  int render_run;
  gTransfm inv_obj_rotation;
  int nthreads;
#ifndef AVOID_THREADS
  JThread* serviceThread;
  JSemaphore* serviceWaitSem;
  JSimpleThread** ray_workers;
  JSemaphore* run_ray_workers;
  JMutex* span_mutex;
  JSemaphore* threads_working;
  void thread_init();
  static void* thread_task(void* this_ren);
#endif
};

