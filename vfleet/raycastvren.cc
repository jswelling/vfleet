/****************************************************************************
 * raycastvren.cc
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
#include <math.h>
#include <unistd.h>

#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include <time.h>
#include <unistd.h>

#else /* ifdef CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E */

#ifdef DECCXX
extern "C" {
#endif

#ifdef SYSV_TIMING
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#include <sys/resource.h>

#ifdef DECCXX
}
#endif

#endif /* ifdef CRAY_ARCH_C90 */

#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "tfun.h"
#include "raycastvren.h"
#include "polyscan.h"

static const int INITIAL_SPAN_TABLE_SIZE= 500;

/*
  Notes:
 */

#ifndef AVOID_THREADS
// This simple thread class can watch for incoming messages while the
// main thread is waiting for the rendering threads to finish.
class ServiceThread: public JThread {
protected:
  raycastVRen* owner;
  JSemaphore* waitSem;
public:
  ServiceThread( raycastVRen* owner_in, JSemaphore* waitSem_in ) {
    owner= owner_in;
    waitSem= waitSem_in;
  }
  void run() {
    // We watch waitSem, which will be set to zero if the main thread
    // wants this thread active.  If active, check every 1 second for
    // incoming PVM messages.  If an appropriate message happens,
    // processing of the message will cause the render to terminate.
    while (1) {
      int semVal= waitSem->waitIfNonzero();
      sleep(1);
      owner->do_service();
    }
  }
};
#endif

raycastVRen::raycastVRen(baseLogger *logger, baseImageHandler *imagehandler, 
			 void (*ready_handler)(baseVRen *renderer,
					       void* cb_data),
			 void* ready_cb_data_in,
			 void (*error_handler)(int error_id, 
					       baseVRen *renderer), 
			 void (*fatal_handler)(int error_id, 
					       baseVRen *renderer),
			 void (*service_call)(), const int nthreads_in) 
: baseVRen( logger, imagehandler, ready_handler, ready_cb_data_in,
	   error_handler, fatal_handler )
{
  service= service_call;
  current_camera= NULL;
  current_quality= NULL;
  current_light_info= NULL;
  current_geom= NULL;
  render_run= 0;
  image= NULL;
  rotated_lights= NULL;
#ifdef AVOID_THREADS
  if (nthreads_in != 0) 
    fprintf(stderr,
      "raycastVRen: compiled without threads but called with nthreads!=0 !\n");
  nthreads= 0;
#else
  nthreads= nthreads_in;
  thread_init();
#endif
  spans_being_generated= 0;
  span_table_size= INITIAL_SPAN_TABLE_SIZE;
  span_table_top= 0;
  span_table= new Span[span_table_size];
  if (ready_proc) (*ready_proc)(this, ready_cb_data);
}

raycastVRen::raycastVRen(baseLogger *logger, baseImageHandler *imagehandler, 
			 void (*ready_handler)(baseVRen *renderer, 
					       void* cb_data),
			 void* ready_cb_data_in,
			 baseVRen *owner, void (*service_call)(),
			 const int nthreads_in)
: baseVRen( logger, imagehandler, ready_handler, ready_cb_data_in, owner )
{
  service= service_call;
  current_camera= NULL;
  current_quality= NULL;
  current_light_info= NULL;
  current_geom= NULL;
  render_run= 0;
  image= NULL;
  rotated_lights= NULL;
#ifdef AVOID_THREADS
  if (nthreads_in != 0) 
    fprintf(stderr,
      "raycastVRen: compiled without threads but called with nthreads!=0 !\n");
  nthreads= 0;
#else
  nthreads= nthreads_in;
  thread_init();
#endif
  spans_being_generated= 0;
  span_table_size= INITIAL_SPAN_TABLE_SIZE;
  span_table_top= 0;
  span_table= new Span[span_table_size];
  if (ready_proc) (*ready_proc)(this, ready_cb_data);
}

raycastVRen::~raycastVRen()
{
#ifndef AVOID_THREADS
  if (nthreads>0)
    run_ray_workers->set(0); // tell the ray worker threads they're done

  for (int i=0; i<nthreads; i++) {
    ray_workers[i]->awaitThreadDeath(); 
    delete ray_workers[i];
  }
  delete [] ray_workers;
  delete span_mutex;
  delete threads_working;
  delete run_ray_workers;
  delete serviceThread;
  delete serviceWaitSem;
#endif
  delete [] span_table;
  delete current_camera;
  delete current_quality;
  delete current_light_info;
  delete current_geom;
  delete rotated_lights;
}

baseSampleVolume *raycastVRen::create_sample_volume( const GridInfo& grid_in,
						baseTransferFunction& tfun_in,
						     const int ndatavol, 
						     DataVolume** data_table ) 
{
  octreeSampleVolume* sv= 
    new octreeSampleVolume( grid_in, tfun_in, ndatavol, data_table,
			    thread_count() );
  return( sv );
}

void raycastVRen::setCamera( const gPoint& lookfm, const gPoint& lookat, 
			     const gVector& up, const float fov, 
			     const float hither, const float yon,
			     const int parallel_flag ) 
{
  delete current_camera;
  current_camera= new rayCamera( lookfm, lookat, up, fov, hither, yon,
				 parallel_flag );
}

void raycastVRen::setCamera( const Camera& cam )
{
  delete current_camera;
  current_camera= new rayCamera( cam );
}

void raycastVRen::AbortRender()
{
  render_run= 0;
}

// Polygon scanning routine uses this callback to shade a pixel
static int shadepixel_cb( const int i, const int j,
			 Poly::Vert* p, Poly::SCAN step,
			 void* cb_data )
{
  static int msg_check_count= 0;
  raycastVRen* this_ren= (raycastVRen*)cb_data;

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  if (msg_check_count++ >= 1999) { // checking for messages expensive on T3D
#else
  if (msg_check_count++ >= 199) { // Check every 200 rays
#endif
    msg_check_count= 0;
    this_ren->do_service();
  }
  if (!this_ren->render_in_progress()) return 1;

  if (this_ren->thread_count()>0) {
    static int pixcount= 0;
    static int startpix= 0;
    switch (step) {
    case Poly::SCAN_START:
      // fprintf(stderr,"Start scan at %d, %d... ",i,j);
      // if (pixcount) fprintf(stderr,"Broken span!!! %d %d\n",i,j);
      startpix= i;
      pixcount= 1;
      break;
    case Poly::SCAN_MIDDLE:
      pixcount++;
      break;
    case Poly::SCAN_END:
      // fprintf(stderr,"End scan at %d, %d; %d pixels total\n",
      //         i,j,pixcount+1);
      this_ren->add_span(startpix,i,j);
      pixcount= 0;
      break;
    case Poly::SCAN_START_END:
      // fprintf(stderr,"Single pixel at %d, %d\n",i,j);
      // if (pixcount) fprintf(stderr,"Broken span singleton!!! %d %d\n",i,j);
      this_ren->add_span(i,i,j);
      break;
    }
  }
  else this_ren->TraceOneRay(i,j);
  return 0;
}

void raycastVRen::grow_span_table()
{
  Span* old_span_table= span_table;
  int old_size= span_table_size;
  span_table_size= 2*old_size;
  span_table= new Span[span_table_size];
  for (int i=0; i<old_size; i++) span_table[i]= old_span_table[i];
  delete [] old_span_table;
}

void raycastVRen::clear_span_table()
{
  span_table_top= 0;
}

#ifndef AVOID_THREADS
void raycastVRen::thread_init()
{
  if (nthreads>0) {
    run_ray_workers= new JSemaphore(0);
    threads_working= new JSemaphore(0);
    serviceWaitSem= new JSemaphore(1);
    serviceThread= new ServiceThread(this, serviceWaitSem);
    serviceThread->start();
    span_mutex= new JMutex();
    ray_workers= new JSimpleThread*[nthreads];
    for (int i=0; i<nthreads; i++) {
      ray_workers[i]= 
	new JSimpleThread((void*(*)(void*))thread_task,(void*)this);
    }
  }
  else {
    run_ray_workers= NULL;
    threads_working= NULL;
    serviceWaitSem= NULL;
    serviceThread= NULL;
    span_mutex= NULL;
    ray_workers= NULL;
  }
}

void* raycastVRen::thread_task(void* this_ren)
{
  raycastVRen* ren= (raycastVRen*)this_ren;
  Span* s;
  const Octree<octSample>* tree;
  Octcell_intersect_iter<octSample>* iter= NULL;
  while (ren->run_ray_workers->wait() > 0) {
    // Iterator parameters are arbitrary;  they are reset before use
    delete iter; // may have an old one around
    tree= ((octreeSampleVolume *)(ren->current_geom->volgob()
				  ->samplevolume()))->get_tree();
    gVector ray_setup_dir(0.0, 0.0, -1.0);
    gPoint ray_setup_origin= 
      ren->current_geom->volgob()->samplevolume()->boundbox().center()
      + (ray_setup_dir * -10.0);
    iter= new Octcell_intersect_iter<octSample>( tree, 
						 ray_setup_dir, 
						 ray_setup_origin, 
						 5.0, 15.0 );
    while (ren->run_ray_workers->value()) {
      
      int spans_still_appearing= ren->spans_being_generated; // as of now
      for (int ispan=0; ispan<ren->span_table_top; ispan++) {
	s= ren->span_table+ispan;
	if (s->pending) {
	  ren->span_mutex->lock();
	  if (s->pending) {
	    s->pending= 0;
	    ren->span_mutex->unlock();
	    for (int i=s->i_start; i<=s->i_end; i++) 
	      ren->TraceOneRay(i,s->j,iter);
	    if (!ren->render_in_progress()) break;
	  }
	  else {
	    ren->span_mutex->unlock();
	  }
	}
      }
      if (!spans_still_appearing || !ren->render_in_progress()) break;
    }
    ren->threads_working->decr();
  }

  delete iter;
  return NULL;
}
#endif

void raycastVRen::start_tracer_threads()
{
#ifdef AVOID_THREADS
  fprintf(stderr,
	  "raycastVRen: internal error: thread start, but no threads!\n");
#else
  threads_working->set(nthreads);
  run_ray_workers->set(1);
#endif
}

void raycastVRen::await_tracer_threads()
{
#ifdef AVOID_THREADS
  fprintf(stderr,"raycastVRen: internal error: wait, but no threads!\n");
#else
  // Release the thread that makes service calls, to watch for
  // incoming messages while we wait
  serviceWaitSem->set(0);

  // Each thread has to finish one complete pass through the span list
  // after StartRender stops adding new spans.  Once that is done, it
  // decrements threads_working.  When threads_working hits zero, the
  // render is done.
  while (threads_working->waitIfNonzero() > 0) {
    if (!render_in_progress()) break;
  }

  // That's it; all the spans have been rendered. This thread is ready
  // to make service calls now, so cause the service thread to wait.
  serviceWaitSem->set(1);
#endif
}

void raycastVRen::StartRender( int image_xdim, int image_ydim )
{
  // Check the status of all needed parts.
  if (!current_camera) {
    error( VRENERROR_CAMERA_NOT_SET );
    return;
  }
  if (!current_quality) {
    error( VRENERROR_QUALITY_NOT_SET );
    return;
  }
  if (!current_light_info) {
    error( VRENERROR_LIGHT_INFO_NOT_SET );
    return;
  }
  if (!current_geom) {
    error( VRENERROR_GEOM_NOT_SET );
    return;
  }

  // Record that we are starting the task
  char msg_buf[80];
  sprintf(msg_buf,"render started: %4d %4d", image_xdim, image_ydim);
  logger->comment(msg_buf);

  // Get start point timing info
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  long time_start= cpused();
#elif SYSV_TIMING
  struct tms time_start;
  (void)times( &time_start );
#else
  rusage start_rusage;
  getrusage(RUSAGE_SELF,&start_rusage);
#endif

  // Generate and/or clear the image into which the data will be rendered
  if ( !image 
      || ((image->xsize() != image_xdim)
	  || (image->ysize() != image_ydim)) ) {
    delete image;
    image= new rgbImage( image_xdim, image_ydim );
  }
  image->clear();

  // Compensate for object rotation.  Individual rays are rotated just
  // before being traced.
  gTransfm obj_trans= current_geom->volgob()->transform();
  inv_obj_rotation= current_geom->volgob()->transform();
  inv_obj_rotation.transpose_self();
  gVector float_dir( current_light_info->dir().x(),
		     current_light_info->dir().y(),
		     current_light_info->dir().z() );
  gVector rotated_float_dir= inv_obj_rotation * float_dir;
  rotated_float_dir.normalize();
  gBVector rotated_dir( rotated_float_dir, 1.0 );
  delete rotated_lights;
  rotated_lights= new LightInfo( current_light_info->clr(), 
				rotated_dir,
				current_light_info->amb() );

  // If fast lighting is to be used, reset the lighting table and default
  // lighting direction
  if (options & OPT_FAST_LIGHTING) {
    rotated_lights->set_central_viewdir( current_camera->pointing_dir() );
    rotated_lights->reset_lighting_tables();
  }

  if (nthreads) {
    clear_span_table(); 
    ((rayCamera *)current_camera)->initdims(*image);
    start_tracer_threads();
  }
  render_run= 1;
  spans_being_generated= 1;  // only relevant when running multithreaded
    
  // If the lookfrom point is inside the bounding box, we shoot all screen
  // rays.  If not, it's faster to figure out which rays will intersect the
  // bounding box and shoot only those.
  gBoundBox bbox= current_geom->volgob()->samplevolume()->boundbox();
  if ( bbox.inside( inv_obj_rotation*current_camera->from() ) ) {
    int msg_check_count= 0;
    for (int j=0; j<image_ydim; j++) {
      if (nthreads) {
	add_span(0,image_xdim-1,j);
      }
      else {
	for (int i=0; i<image_xdim; i++) {
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
	  if (msg_check_count++ >= 1999) { // checking expensive on T3D
#else
	  if (msg_check_count++ >= 199) { // Check every 200 rays
#endif
	    msg_check_count= 0;
	    do_service();
	  }
	  if (!render_in_progress()) break;
	  TraceOneRay(i,j);
	}
      }
      if (!render_in_progress()) break;
    }
  }
  else {
    // Generate camera matrix, for use in finding bbox faces to scan
    gTransfm* screen_proj= 
      current_camera->screen_projection_matrix( image_xdim, image_ydim,
					     -32768, 32767 );

    // The bounding box has six faces.  Find and render the front-facing
    // ones, of which there may be up to three.
    gPoint corner[2][2][2];
    corner[0][0][0]= obj_trans*gPoint( bbox.xmin(), bbox.ymin(), bbox.zmin() );
    corner[0][0][1]= obj_trans*gPoint( bbox.xmin(), bbox.ymin(), bbox.zmax() );
    corner[0][1][0]= obj_trans*gPoint( bbox.xmin(), bbox.ymax(), bbox.zmin() );
    corner[0][1][1]= obj_trans*gPoint( bbox.xmin(), bbox.ymax(), bbox.zmax() );
    corner[1][0][0]= obj_trans*gPoint( bbox.xmax(), bbox.ymin(), bbox.zmin() );
    corner[1][0][1]= obj_trans*gPoint( bbox.xmax(), bbox.ymin(), bbox.zmax() );
    corner[1][1][0]= obj_trans*gPoint( bbox.xmax(), bbox.ymax(), bbox.zmin() );
    corner[1][1][1]= obj_trans*gPoint( bbox.xmax(), bbox.ymax(), bbox.zmax() );
    
    gVector face_normal;
    gVector to_min_corner= corner[0][0][0] - current_camera->from();
    gVector to_max_corner= corner[1][1][1] - current_camera->from();
    
    Poly_box poly_box;
    poly_box.x0= 0;
    poly_box.y0= 0;
    poly_box.z0= -32768.0;
    poly_box.x1= image_xdim;
    poly_box.y1= image_ydim;
    poly_box.z1= 32767.0;
    
    Poly_window poly_win;
    poly_win.x0= poly_win.y0= 0;
    poly_win.x1= image_xdim-1;
    poly_win.y1= image_ydim-1;
    
    Poly poly;
    poly.mask= POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw);
    
    // x=xmin and x=xmax faces (mutually exclusive)
    poly.n= 4;
    poly.vert[0]= corner[0][0][0];
    poly.vert[1]= corner[0][0][1];
    poly.vert[2]= corner[0][1][1];
    poly.vert[3]= corner[0][1][0];
    poly.trans_to_screen( *screen_proj );
    if (poly.front_facing()) {
      if ( poly.clip_to_box_sides( &poly_box ) != Poly::CLIP_OUT ) {
	poly.homogenize();
	poly.scan( &poly_win, shadepixel_cb, (void*)this );
      }
    }
    else {
      poly.n= 4;
      poly.vert[0]= corner[1][0][0];
      poly.vert[1]= corner[1][1][0];
      poly.vert[2]= corner[1][1][1];
      poly.vert[3]= corner[1][0][1];
      poly.trans_to_screen( *screen_proj );
      if (poly.front_facing()) {
	if ( poly.clip_to_box_sides( &poly_box ) != Poly::CLIP_OUT ) {
	  poly.homogenize();
	  poly.scan( &poly_win, shadepixel_cb, (void*)this );
	}
      }
    }

    // y=ymin and y=ymax faces (mutually exclusive)
    if (render_run) {
      poly.n= 4;
      poly.vert[0]= corner[0][0][0];
      poly.vert[1]= corner[1][0][0];
      poly.vert[2]= corner[1][0][1];
      poly.vert[3]= corner[0][0][1];
      poly.trans_to_screen( *screen_proj );
      if (poly.front_facing()) {
	if ( poly.clip_to_box_sides( &poly_box ) != Poly::CLIP_OUT ) {
	  poly.homogenize();
	  poly.scan( &poly_win, shadepixel_cb, (void*)this );
	}
      }
      else {
	poly.n= 4;
	poly.vert[0]= corner[0][1][0];
	poly.vert[1]= corner[0][1][1];
	poly.vert[2]= corner[1][1][1];
	poly.vert[3]= corner[1][1][0];
	poly.trans_to_screen( *screen_proj );
	if (poly.front_facing()) {
	  if ( poly.clip_to_box_sides( &poly_box ) != Poly::CLIP_OUT ) {
	    poly.homogenize();
	    poly.scan( &poly_win, shadepixel_cb, (void*)this );
	  }
	}
      }
    }

    // z=zmin and z=zmax faces (mutually exclusive)
    if (render_run) {
      poly.n= 4;
      poly.vert[0]= corner[0][0][0];
      poly.vert[1]= corner[0][1][0];
      poly.vert[2]= corner[1][1][0];
      poly.vert[3]= corner[1][0][0];
      poly.trans_to_screen( *screen_proj );
      if (poly.front_facing()) {
	if ( poly.clip_to_box_sides( &poly_box ) != Poly::CLIP_OUT ) {
	  poly.homogenize();
	  poly.scan( &poly_win, shadepixel_cb, (void*)this );
	}
      }
      else {
	poly.n= 4;
	poly.vert[0]= corner[0][0][1];
	poly.vert[1]= corner[1][0][1];
	poly.vert[2]= corner[1][1][1];
	poly.vert[3]= corner[0][1][1];
	poly.trans_to_screen( *screen_proj );
	if (poly.front_facing()) {
	  if ( poly.clip_to_box_sides( &poly_box ) != Poly::CLIP_OUT ) {
	    poly.homogenize();
	    poly.scan( &poly_win, shadepixel_cb, (void*)this );
	  }
	}
      }
    }

    // Clean up
    delete screen_proj;
  }
  spans_being_generated= 0;  // only relevant when running multithreaded

  if (nthreads) await_tracer_threads();
  render_run= 0;

  ihandler->display( image );
  

  // Check rendering time
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  long ticks_per_sec= sysconf(_SC_CLK_TCK);
  long time_end= cpused();
  long s_usec= 0;
  long s_sec= 0;
  long u_sec= (time_end-time_start)/ticks_per_sec;
  long u_usec= ((time_end-time_start)%ticks_per_sec)/(ticks_per_sec/1000000.0);
#elif SYSV_TIMING
  struct tms time_end;
  (void)times( &time_end );
  long s_usec= (long)((1000000.0/CLK_TCK)
		      * (time_end.tms_stime - time_start.tms_stime));
  long s_sec= s_usec / 1000000;
  s_usec= s_usec % 1000000;
  long u_usec= (long)((1000000.0/CLK_TCK) 
		      * (time_end.tms_utime - time_start.tms_utime));
  long u_sec= u_usec / 1000000;
  u_usec= u_usec % 1000000;
#else // not CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E
  rusage end_rusage;
  getrusage(RUSAGE_SELF,&end_rusage);
  long s_usec= end_rusage.ru_stime.tv_usec - start_rusage.ru_stime.tv_usec;
  long s_sec= end_rusage.ru_stime.tv_sec - start_rusage.ru_stime.tv_sec;
  long u_usec= end_rusage.ru_utime.tv_usec - start_rusage.ru_utime.tv_usec;
  long u_sec= end_rusage.ru_utime.tv_sec - start_rusage.ru_utime.tv_sec;
#endif
  if (s_usec<0) {
    s_sec -= 1;
    s_usec += 1000000;
  }
  if (u_usec<0) {
    u_sec -= 1;
    u_usec += 1000000;
  }
  
  // Record that we are done.
  char out_string[80];
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  sprintf(out_string,"render complete: %d.%06d sec total",u_sec,u_usec);
#else
  sprintf(out_string,"render complete: %d.%06du   %d.%06ds",
	  u_sec,u_usec,s_sec,s_usec);
#endif
  logger->comment(out_string);
}

