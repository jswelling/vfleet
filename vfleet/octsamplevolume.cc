/****************************************************************************
 * octsamplevolume.cc
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
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "tfun.h"
#include "raycastvren.h"

#include <sys/time.h>
#include <sys/resource.h>

static void leaf_initialize( void *cells_in,
			     int i, int j, int kmin, int kmax, void *init_in )
{
  octSample *cells = (octSample *) cells_in;
  octreeSampleVolume::init_info* init=
    (octreeSampleVolume::init_info*)init_in;
  
  init->transfer->apply_row (cells, i, j, kmin, kmax, init->svol,
			 init->ndatavol, init->data_table );
}

void octSample_average_color_and_grad( octSample *cell,
				       Octcell_kids_iter<octSample>& iter )
{
  octSample *next_cell;
  int rave= 0, gave= 0, bave= 0, aave= 0;
  int xave= 0, yave= 0, zave= 0;

  while (next_cell= iter.next()) {
    rave += next_cell->clr.ir();
    gave += next_cell->clr.ig();
    bave += next_cell->clr.ib();
    aave += next_cell->clr.ia();
    xave += next_cell->grad_ix();
    yave += next_cell->grad_iy();
    zave += next_cell->grad_iz();
  }

  cell->clr= gBColor( rave >> 3, gave >> 3, bave >> 3, aave >> 3 );
  cell->set_grad_ix( xave >> 3 );
  cell->set_grad_iy( yave >> 3 );
  cell->set_grad_iz( zave >> 3 );
}

void octSample_calc_values( octSample *cell,
			    const Octcell_kids_iter<octSample>& orig_iter )
// Derive values dependent on those of children.
{
  octSample *next_cell;

  // Leaves already set up; qual initializes to correct values
  if (!orig_iter.leaf()) {

    // Calculate children\'s values
    Octcell_kids_iter<octSample> kids_iter= orig_iter;
    while (next_cell= kids_iter.next()) {
      octSample_calc_values( next_cell, kids_iter.child_iter() );
    }
  }

  // Calculate average color and gradient.
  Octcell_kids_iter<octSample> ave_iter= orig_iter;
  octSample_average_color_and_grad( cell, ave_iter );

  // Calculate maximum possible deviations from average.
  Octcell_kids_iter<octSample> dev_iter= orig_iter;
  QualityMeasure cell_qual;
  while ( next_cell= dev_iter.next() ) {
    cell_qual.add_max_deviation( next_cell->clr, cell->clr,
				 QualityMeasure( 0, next_cell->clr_error(), 
						 0 ) );
  }
  cell->set_clr_error( cell_qual.get_color_comp_ierr() );
}

gColor octSample::calc_color_o0_inline( LightInfo& lights,
					 const VRenOptions options,
					 const int debug)
{
  gColor cell_clr= clr; // convert from gBColor to gColor
  gColor lighted_cell_clr; // may actually compute without lighting

  if (options & OPT_USE_LIGHTING) {
    gColor light_clr;
    // Get gradient, rescaled for voxel aspect ratio
    gBVector gradient= gBVector( grad_ix(), grad_iy(), grad_iz(), 255 );
    
    if (options & OPT_FAST_LIGHTING) {
      light_clr= lights.get_fast_light_clr(gradient);
    }
    else {
      light_clr= lights.calc_light_clr( gradient, debug );
    }

    lighted_cell_clr= light_clr * cell_clr;

    if (options & OPT_SPECULAR_LIGHTING) {
      gColor spec_clr;
      if (options & OPT_FAST_LIGHTING) {
	spec_clr= lights.get_fast_specular_clr( gradient );
      }
      else {
	spec_clr= lights.calc_specular_clr( gradient, debug );
      }
      lighted_cell_clr += spec_clr;
    }
  }
  else { // lighting turned off
    lighted_cell_clr= cell_clr;
  }

  return lighted_cell_clr;
}

gColor octSample::calc_color_o0( LightInfo& lights,
				 const VRenOptions options,
				 const int debug )
{
  return calc_color_o0_inline( lights, options, debug );
}

#ifndef AVOID_THREADS
class octThread: public JThread {
public:
  octThread( octreeSampleVolume* sv_in, int* next_slice_in, int do_grads_in,
	     Octcell_kids_iter<octSample>* iter_in,
	     JMutex* slice_mutex_in, 
	     JSemaphore* wait_semaphore1_in, JSemaphore* wait_semaphore2_in );
  ~octThread() { nInstances--; }
  void run();
protected:
  static int nInstances;
  int id;
  int grabNextSlice();
  octreeSampleVolume* sv;
  Octcell_kids_iter<octSample>* iter;
  int* next_slice;
  JMutex* slice_mutex;
  JSemaphore* wait_semaphore1;
  JSemaphore* wait_semaphore2;
  int do_grads;
};

int octThread::nInstances= 0;

octThread::octThread( octreeSampleVolume* sv_in, int* next_slice_in, 
		      int do_grads_in, Octcell_kids_iter<octSample>* iter_in,
		      JMutex* slice_mutex_in,
		      JSemaphore* wait_semaphore1_in, 
		      JSemaphore* wait_semaphore2_in ) {
  id= nInstances++;
  sv= sv_in;
  iter= iter_in;
  next_slice= next_slice_in;
  do_grads= do_grads_in;
  slice_mutex= slice_mutex_in;
  wait_semaphore1= wait_semaphore1_in;
  wait_semaphore2= wait_semaphore2_in;
};

int octThread::grabNextSlice() {
  slice_mutex->lock();
  int result= (*next_slice)++;
  slice_mutex->unlock();
  return result;
};

void octThread::run() {
  int i, j, k;
  octSample *cell;
  const Octree<octSample>* sample_tree= sv->get_tree();
  int xdim= sv->xsize();
  int ydim= sv->ysize();
  int zdim= sv->zsize();
  int lf_lvl= sample_tree->get_nlevels();

  // Calculate opacity gradients
  if (do_grads) {
    gVector grad_rescale= sv->get_inv_voxel_aspect_ratio();
    
    register int diff_x, diff_y, diff_z;
    while ((i= grabNextSlice()) < xdim) {
      for (j=0; j<ydim; j++) 
	for (k=0; k<zdim; k++) {
	  cell= sample_tree->get_cell( lf_lvl, i, j, k );
	  
	  if (i==0) 
	    // diff_x= cell->clr.ia()/2;
	    diff_x= (-3*sample_tree->get_cell( lf_lvl, 0, j, k )->clr.ia()
		     +4*sample_tree->get_cell( lf_lvl, 1, j, k )->clr.ia()
		     -1*sample_tree->get_cell( lf_lvl, 2, j, k )->clr.ia() 
		     )/2;
	  else if (i==xdim-1) 
	    // diff_x= -cell->clr.ia()/2;
	    diff_x= ( 3*sample_tree->get_cell(lf_lvl, xdim-1, j, k)->clr.ia()
		      -4*sample_tree->get_cell(lf_lvl, xdim-2, j, k)->clr.ia()
		      +1*sample_tree->get_cell(lf_lvl, xdim-3, j, k)->clr.ia() 
		      )/2;
	  else {
	    diff_x= ( sample_tree->get_cell( lf_lvl, i+1, j, k )->clr.ia()
		      - sample_tree->get_cell( lf_lvl, i-1, j, k )->clr.ia() ) 
	      / 2 ;
	  }
	  
	  if (j==0) 
	    // diff_y= cell->clr.ia()/2;
	    diff_y= (-3*sample_tree->get_cell( lf_lvl, i, 0, k )->clr.ia()
		     +4*sample_tree->get_cell( lf_lvl, i, 1, k )->clr.ia()
		     -1*sample_tree->get_cell( lf_lvl, i, 2, k )->clr.ia() 
		     )/2;
	  else if (j==ydim-1) 
	    // diff_y= -cell->clr.ia()/2;
	    diff_y= ( 3*sample_tree->get_cell(lf_lvl, i, ydim-1, k)->clr.ia()
		      -4*sample_tree->get_cell(lf_lvl, i, ydim-2, k)->clr.ia()
		      +1*sample_tree->get_cell(lf_lvl, i, ydim-3, k)->clr.ia() 
		      )/2;
	  else {
	    diff_y= ( sample_tree->get_cell( lf_lvl, i, j+1, k )->clr.ia()
		      - sample_tree->get_cell( lf_lvl, i, j-1, k )->clr.ia() ) 
	      / 2 ;
	  }
	  
	  if (k==0) 
	    // diff_z= cell->clr.ia()/2;
	    diff_z= (-3*sample_tree->get_cell( lf_lvl, i, j, 0 )->clr.ia()
		     +4*sample_tree->get_cell( lf_lvl, i, j, 1 )->clr.ia()
		     -1*sample_tree->get_cell( lf_lvl, i, j, 2 )->clr.ia() 
		     )/2;
	  else if (k==zdim-1)
	    // diff_z= -cell->clr.ia()/2;
	    diff_z= ( 3*sample_tree->get_cell(lf_lvl, i, j, zdim-1)->clr.ia()
		      -4*sample_tree->get_cell(lf_lvl, i, j, zdim-2)->clr.ia()
		      +1*sample_tree->get_cell(lf_lvl, i, j, zdim-3)->clr.ia()
		      )/2;
	  else {
	    diff_z= ( sample_tree->get_cell( lf_lvl, i, j, k+1 )->clr.ia()
		      - sample_tree->get_cell( lf_lvl, i, j, k-1 )->clr.ia() ) 
	      / 2 ;
	  }
	  cell->set_grad_ix( (int)(grad_rescale.x()*diff_x + 0.5) );
	  cell->set_grad_iy( (int)(grad_rescale.y()*diff_y + 0.5) );
	  cell->set_grad_iz( (int)(grad_rescale.z()*diff_z + 0.5) );
	}
    }
  }

  if (id==0) {
    slice_mutex->lock();
    *next_slice= 0;
    slice_mutex->unlock();
    wait_semaphore1->incr(nInstances-1);
    while (wait_semaphore1->waitIfNonzero() != 0) { /* keep waiting */ }
  } else {
    wait_semaphore1->decr();
    while (wait_semaphore1->waitIfNonzero() != 0) { /* keep waiting */ };
  }

  int imin, imax;
  int jmin, jmax;
  int kmin, kmax;

  while ((i= grabNextSlice()) < xdim) {
    imin= (i) ? i-1 : 0;
    imax= (i<xdim-1) ? i+1 : i;
    for (j=0; j<ydim; j++) { 
      jmin= (j) ? j-1 : 0;
      jmax= (j<ydim-1) ? j+1 : j;
      for (k=0; k<zdim; k++) {
	kmin= (k) ? k-1 : 0;
	kmax= (k<zdim-1) ? k+1 : k;
	cell= sample_tree->get_cell( lf_lvl, i, j, k );
	gBColor cclr= cell->clr;
	int error= 0;

	for (int ii=imin; ii<=imax; ii++)
	  for (int jj=jmin; jj<=jmax; jj++)
	    for (int kk=kmin; kk<=kmax; kk++) {
	      if (ii==i && jj==j && kk==k) continue;
	      gBColor oclr= sample_tree->get_cell( lf_lvl, ii, jj, kk )->clr;
	      int tmp= ((oclr.ir()>cclr.ir()) ? 
			oclr.ir()-cclr.ir() : cclr.ir()-oclr.ir());
	      if (tmp>error) error= tmp;
	      tmp= ((oclr.ig()>cclr.ig()) ? 
		    oclr.ig()-cclr.ig() : cclr.ig()-oclr.ig());
	      if (tmp>error) error= tmp;
       	      tmp= ((oclr.ib()>cclr.ib()) ? 
		    oclr.ib()-cclr.ib() : cclr.ib()-oclr.ib());
	      if (tmp>error) error= tmp;
	      tmp= ((oclr.ia()>cclr.ia()) ? 
		    oclr.ia()-cclr.ia() : cclr.ia()-oclr.ia());
	      if (tmp>error) error= tmp;
	    }

	cell->set_clr_error( (unsigned char)error );
      }
    }
  }

  if (id==0) {
    slice_mutex->lock();
    *next_slice= 0;
    slice_mutex->unlock();
    wait_semaphore2->incr(nInstances-1);
    while (wait_semaphore2->waitIfNonzero() != 0) { /* keep waiting */ }
  } else {
    wait_semaphore2->decr();
    while (wait_semaphore2->waitIfNonzero() != 0) { /* keep waiting */ };
  }

  while (1) {
    slice_mutex->lock();
    octSample* child_cell= iter->next();
    Octcell_kids_iter<octSample> child_iter= iter->child_iter();
    slice_mutex->unlock();
    if (!child_cell) break;
    octSample_calc_values( child_cell, child_iter );
  }

};  
#endif

octreeSampleVolume::octreeSampleVolume( const GridInfo& grid_in, 
					baseTransferFunction& tfun_in,
					int ndatavol, DataVolume **data_table,
					int nthreads_in )
: baseSampleVolume( grid_in, tfun_in, ndatavol, data_table )
{
  init_info info;
  info.transfer= tfun;
  info.svol= this;
  info.ndatavol= ndatavol;
  info.data_table= data_table;
  nthreads= nthreads_in;

  // Calculate a mean voxel size for use in mipmapping
  voxel_mean_size= (1.0/3.0)*(grid.dx() + grid.dy() + grid.dz());

  // Create the octree of Sample instances
  sample_tree= new Octree<octSample>( boundbox(), xsize(), ysize(), zsize(), 
				      leaf_initialize, &info );

  // Calculate opacity gradients and error measures for leaves, 
  // and build coarser reps from child data
  calc_tree_vals();

  // Create the (reuseable) ray-octree intersection iterator.  The
  // direction information is arbitrary;  it is reset for each ray.
  gVector ray_setup_dir(0.0, 0.0, -1.0);
  gPoint ray_setup_origin= boundbox().center() + (ray_setup_dir * -10.0);
  oct_iter= new Octcell_intersect_iter<octSample> 
             ( sample_tree, ray_setup_dir, ray_setup_origin, 5.0, 15.0 );
}

octreeSampleVolume::~octreeSampleVolume()
{
  delete oct_iter;
  delete sample_tree;
}

void octreeSampleVolume::calc_tree_vals()
{
#ifdef AVOID_THREADS
  // Calculate gradients of opacity in the leaf nodes
  calc_leaf_gradients();
  smooth_leaf_gradients();
  // Calculate error measures of leaf nodes
  calc_leaf_errors();

  // Each sample must now derive its values from those of its children
  Octcell_kids_iter<octSample> iter(sample_tree);
  octSample_calc_values( sample_tree->top(), iter );
#else

  if (nthreads==0) {
    // Calculate gradients of opacity in the leaf nodes
    calc_leaf_gradients();
    smooth_leaf_gradients();
    // Calculate error measures of leaf nodes
    calc_leaf_errors();
    
    // Each sample must now derive its values from those of its children
    Octcell_kids_iter<octSample> iter(sample_tree);
    octSample_calc_values( sample_tree->top(), iter );
  }
  else {

    int do_grads= 1;
    if ((xsize()<3) || (ysize()<3) || (zsize()<3)) {
      fprintf(stderr,
	      "Cannot calculate leaf gradients; a dim is less than 3!\n");
      // Constructor should have initialized grads to zero
      do_grads= 0;
    }
    
    int i;
    int next_slice= 0;
    octThread** threads= new octThread*[nthreads];
    Octcell_kids_iter<octSample> top_kids_iter(sample_tree);
    Octcell_kids_iter<octSample> kids_iter_copy= top_kids_iter;
    JMutex* slice_mutex= new JMutex();
    JSemaphore* wait_semaphore1= new JSemaphore(0);
    JSemaphore* wait_semaphore2= new JSemaphore(0);
    for (i=0; i<nthreads; i++)
      threads[i]= new octThread( this, &next_slice, do_grads, &kids_iter_copy,
				 slice_mutex, 
				 wait_semaphore1, wait_semaphore2 );
    for (i=0; i<nthreads; i++) threads[i]->start();
    for (i=0; i<nthreads; i++) threads[i]->awaitThreadDeath();
    for (i=0; i<nthreads; i++) delete threads[i];
    delete [] threads;
    delete wait_semaphore1;
    delete wait_semaphore2;
    delete slice_mutex;

    // top level cell averages; leaves and intermediate levels are set up.
    // Calculate average color and gradient.
    octSample *next_cell;
    Octcell_kids_iter<octSample> ave_iter= top_kids_iter;
    octSample_average_color_and_grad( sample_tree->top(), ave_iter );
    
    // Calculate maximum possible deviations from average.
    Octcell_kids_iter<octSample> dev_iter= top_kids_iter;
    QualityMeasure cell_qual;
    while ( next_cell= dev_iter.next() ) {
      cell_qual.add_max_deviation( next_cell->clr, sample_tree->top()->clr,
				   QualityMeasure( 0, next_cell->clr_error(), 
						   0 ) );
    }
    sample_tree->top()->set_clr_error( cell_qual.get_color_comp_ierr() );
    
  }
#endif
}

void octreeSampleVolume::regenerate( baseTransferFunction& tfun_in,
				     int ndatavol, DataVolume** data_table )
{
  // Take care of bookkeeping
  baseSampleVolume::regenerate( tfun_in, ndatavol, data_table );

  // Regenerate the octree
  init_info info;
  info.svol= this;
  info.transfer= tfun= &tfun_in;
  info.ndatavol= num_datavols;
  info.data_table= datavols;

  sample_tree->reinitialize( &info );

  // Calculate opacity gradients and error measures for leaves, 
  // and build coarser reps from child data
  calc_tree_vals();
}

void octreeSampleVolume::smooth_leaf_gradients()
{
  int i, j, k;
  octSample *cell;
  int lf_lvl= sample_tree->get_nlevels();
  int* scratch;
  int val;

  rusage start_rusage;
  getrusage(RUSAGE_SELF,&start_rusage);

  // For all x and y, smooth in z
  if (zsize()>3) {
    scratch= new int[zsize()];
    for (i=0; i<xsize(); i++)
      for (j=0; j<ysize(); j++) {

	for (k=0; k<zsize(); k++) scratch[k]= 1; // to fix rounding

	// Calc all smoothed z-direction elements
#ifdef never
	scratch[1] += sample_tree->get_cell( lf_lvl, i, j, 0 )->grad_iz();
#endif
	scratch[1] += (sample_tree->get_cell( lf_lvl, i, j, 0 )->grad_iz())>>1;
	for (k=1; k<zsize()-1; k++) {
	  val= sample_tree->get_cell( lf_lvl, i, j, k )->grad_iz();
	  scratch[k] += val;
#ifdef never
	  scratch[k-1] += val;
	  scratch[k+1] += val;
#endif
	  scratch[k-1] += val>>1;
	  scratch[k+1] += val>>1;

	}
#ifdef never
	scratch[zsize()-2] += 
	  sample_tree->get_cell( lf_lvl, i, j, zsize()-1 )->grad_iz();
#endif
	scratch[zsize()-2] += 
	  (sample_tree->get_cell( lf_lvl, i, j, zsize()-1 )->grad_iz())>>1;

#ifdef never
	// divide by 3
	for (k=1; k<zsize()-1; k++) scratch[k] /= 3;
#endif
#ifdef never
	// divide by 3.  This works for vals < 1024.
	for (k=1; k<zsize(); k++) {
	  int t= scratch[k];
	  scratch[k]= (t>>2) + (t>>4) + (t>>6) + (t>>8);
	}
#endif
	// divide by 2.
	for (k=1; k<zsize()-1; k++) scratch[k]= scratch[k]>>1;
	
	// Replace current values with smoothed
	for (k=1; k<zsize()-1; k++)
	  sample_tree->get_cell( lf_lvl, i, j, k )->set_grad_iz( scratch[k] );

      }
    delete scratch;
  }

  // For all y and z, smooth in x
  if (xsize()>3) {
    scratch= new int[xsize()];
    for (j=0; j<ysize(); j++)
      for (k=0; k<zsize(); k++) {

	for (i=0; i<xsize(); i++) scratch[i]= 1; // to fix rounding

	// Calc all smoothed x-direction elements
#ifdef never
	scratch[1] += sample_tree->get_cell( lf_lvl, 0, j, k )->grad_ix();
#endif
	scratch[1] += (sample_tree->get_cell( lf_lvl, 0, j, k )->grad_ix())>>1;
	for (i=1; i<xsize()-1; i++) {
	  val= sample_tree->get_cell( lf_lvl, i, j, k )->grad_ix();
	  scratch[i] += val;
#ifdef never
	  scratch[i-1] += val;
	  scratch[i+1] += val;
#endif
	  scratch[i-1] += val>>1;
	  scratch[i+1] += val>>1;
	}
#ifdef never
	scratch[xsize()-2] += 
	  sample_tree->get_cell( lf_lvl, xsize()-1, j, k )->grad_ix();
#endif
	scratch[xsize()-2] += 
	  (sample_tree->get_cell( lf_lvl, xsize()-1, j, k )->grad_ix())>>1;

#ifdef never
	// divide by 3
	for (i=1; i<xsize()-1; i++) scratch[i] /= 3;
#endif
#ifdef never
	// divide by 3.  This works for vals < 1024.
	for (i=1; i<xsize()-1; i++) {
	  int t= scratch[i];
	  scratch[i]= (t>>2) + (t>>4) + (t>>6) + (t>>8);
	}
#endif
	for (i=1; i<xsize()-1; i++) scratch[i]= scratch[i]>>1;
	
	// Replace current values with smoothed
	for (i=1; i<xsize()-1; i++)
	  sample_tree->get_cell( lf_lvl, i, j, k )->set_grad_ix( scratch[i] );

      }
    delete scratch;
  }

  // For all z and x, smooth in y
  if (ysize()>3) {
    scratch= new int[ysize()];
    for (k=0; k<zsize(); k++)
      for (i=0; i<xsize(); i++) {

	for (j=0; j<ysize(); j++) scratch[j]= 1; // to fix rounding

	// Calc all smoothed y-direction elements
#ifdef never
	scratch[1] += sample_tree->get_cell( lf_lvl, i, 0, k )->grad_iy();
#endif
	scratch[1] += (sample_tree->get_cell( lf_lvl, i, 0, k )->grad_iy())>>1;
	for (j=1; j<ysize()-1; j++) {
	  val= sample_tree->get_cell( lf_lvl, i, j, k )->grad_iy();
	  scratch[j] += val;
#ifdef never
	  scratch[j-1] += val;
	  scratch[j+1] += val;
#endif
	  scratch[j-1] += val>>1;
	  scratch[j+1] += val>>1;
	}
#ifdef never
	scratch[ysize()-2] += 
	  sample_tree->get_cell( lf_lvl, i, ysize()-1, k )->grad_iy();
#endif
	scratch[ysize()-2] += 
	  (sample_tree->get_cell( lf_lvl, i, ysize()-1, k )->grad_iy())>>1;

#ifdef never
	// divide by 3
	for (j=1; j<ysize()-1; j++) scratch[j] /= 3;
#endif
#ifdef never
	// divide by 3.  This works for vals < 1024.
	for (j=1; j<ysize()-1; j++) {
	  int t= scratch[j];
	  scratch[j]= (t>>2) + (t>>4) + (t>>6) + (t>>8);
	}
#endif
	for (j=1; j<ysize()-1; j++) scratch[j]= scratch[j]>>1;
	
	// Replace current values with smoothed
	for (j=1; j<ysize()-1; j++)
	  sample_tree->get_cell( lf_lvl, i, j, k )->set_grad_iy( scratch[j] );

      }
    delete scratch;
  }

  rusage end_rusage;
  getrusage(RUSAGE_SELF,&end_rusage);
  long s_usec= end_rusage.ru_stime.tv_usec - start_rusage.ru_stime.tv_usec;
  long s_sec= end_rusage.ru_stime.tv_sec - start_rusage.ru_stime.tv_sec;
  if (s_usec<0) {
    s_usec += 1000000;
    s_sec -= 1;
  }
  long u_usec= end_rusage.ru_utime.tv_usec - start_rusage.ru_utime.tv_usec;
  long u_sec= end_rusage.ru_utime.tv_sec - start_rusage.ru_utime.tv_sec;
  if (u_usec<0) {
    u_usec += 1000000;
    u_sec -= 1;
  }
  fprintf(stderr,"Smoothing: u %ld.%06ld s %ld.%06ld\n",
	  u_sec, u_usec, s_sec, s_usec);

}

void octreeSampleVolume::calc_leaf_gradients()
{
  int i, j, k;
  octSample *cell;
  int lf_lvl= sample_tree->get_nlevels();

  gVector grad_rescale= get_inv_voxel_aspect_ratio();
  register int diff_x, diff_y, diff_z;
  for (i=0; i<xsize(); i++)
    for (j=0; j<ysize(); j++) 
      for (k=0; k<zsize(); k++) {
	cell= sample_tree->get_cell( lf_lvl, i, j, k );

	if (i==0) 
	  // diff_x= cell->clr.ia()/2;
	  diff_x= (-3*sample_tree->get_cell( lf_lvl, 0, j, k )->clr.ia()
		   +4*sample_tree->get_cell( lf_lvl, 1, j, k )->clr.ia()
		   -1*sample_tree->get_cell( lf_lvl, 2, j, k )->clr.ia() 
		 )/2;
	else if (i==xsize()-1) 
	  // diff_x= -cell->clr.ia()/2;
	  diff_x= ( 3*sample_tree->get_cell(lf_lvl, xsize()-1, j, k)->clr.ia()
		 -4*sample_tree->get_cell( lf_lvl, xsize()-2, j, k )->clr.ia()
		 +1*sample_tree->get_cell( lf_lvl, xsize()-3, j, k )->clr.ia() 
		 )/2;
	else {
	  diff_x= ( sample_tree->get_cell( lf_lvl, i+1, j, k )->clr.ia()
		  - sample_tree->get_cell( lf_lvl, i-1, j, k )->clr.ia() ) 
	    / 2 ;
	}

	if (j==0) 
	  // diff_y= cell->clr.ia()/2;
	  diff_y= (-3*sample_tree->get_cell( lf_lvl, i, 0, k )->clr.ia()
		 +4*sample_tree->get_cell( lf_lvl, i, 1, k )->clr.ia()
		 -1*sample_tree->get_cell( lf_lvl, i, 2, k )->clr.ia() 
		 )/2;
	else if (j==ysize()-1) 
	  // diff_y= -cell->clr.ia()/2;
	  diff_y= ( 3*sample_tree->get_cell(lf_lvl, i, ysize()-1, k)->clr.ia()
		 -4*sample_tree->get_cell( lf_lvl, i, ysize()-2, k )->clr.ia()
		 +1*sample_tree->get_cell( lf_lvl, i, ysize()-3, k )->clr.ia() 
		 )/2;
	else {
	  diff_y= ( sample_tree->get_cell( lf_lvl, i, j+1, k )->clr.ia()
		  - sample_tree->get_cell( lf_lvl, i, j-1, k )->clr.ia() ) 
	    / 2 ;
	}

	if (k==0) 
	  // diff_z= cell->clr.ia()/2;
	  diff_z= (-3*sample_tree->get_cell( lf_lvl, i, j, 0 )->clr.ia()
		 +4*sample_tree->get_cell( lf_lvl, i, j, 1 )->clr.ia()
		 -1*sample_tree->get_cell( lf_lvl, i, j, 2 )->clr.ia() 
		 )/2;
	else if (k==zsize()-1)
	  // diff_z= -cell->clr.ia()/2;
	  diff_z= ( 3*sample_tree->get_cell(lf_lvl, i, j, zsize()-1)->clr.ia()
		 -4*sample_tree->get_cell( lf_lvl, i, j, zsize()-2 )->clr.ia()
		 +1*sample_tree->get_cell( lf_lvl, i, j, zsize()-3 )->clr.ia()
		 )/2;
	else {
	  diff_z= ( sample_tree->get_cell( lf_lvl, i, j, k+1 )->clr.ia()
		  - sample_tree->get_cell( lf_lvl, i, j, k-1 )->clr.ia() ) 
	    / 2 ;
	}
	cell->set_grad_ix( (int)(grad_rescale.x()*diff_x + 0.5) );
	cell->set_grad_iy( (int)(grad_rescale.y()*diff_y + 0.5) );
	cell->set_grad_iz( (int)(grad_rescale.z()*diff_z + 0.5) );
      }
}

void octreeSampleVolume::calc_leaf_errors()
{
  int i, j, k;
  octSample *cell;
  int lf_lvl= sample_tree->get_nlevels();
  int imin, imax;
  int jmin, jmax;
  int kmin, kmax;

  for (i=0; i<xsize(); i++) {
    imin= (i) ? i-1 : 0;
    imax= (i<xsize()-1) ? i+1 : i;
    for (j=0; j<ysize(); j++) { 
      jmin= (j) ? j-1 : 0;
      jmax= (j<ysize()-1) ? j+1 : j;
      for (k=0; k<zsize(); k++) {
	kmin= (k) ? k-1 : 0;
	kmax= (k<zsize()-1) ? k+1 : k;
	cell= sample_tree->get_cell( lf_lvl, i, j, k );
	gBColor cclr= cell->clr;
	int error= 0;

	for (int ii=imin; ii<=imax; ii++)
	  for (int jj=jmin; jj<=jmax; jj++)
	    for (int kk=kmin; kk<=kmax; kk++) {
	      if (ii==i && jj==j && kk==k) continue;
	      gBColor oclr= sample_tree->get_cell( lf_lvl, ii, jj, kk )->clr;
	      int tmp= ((oclr.ir()>cclr.ir()) ? 
			oclr.ir()-cclr.ir() : cclr.ir()-oclr.ir());
	      if (tmp>error) error= tmp;
	      tmp= ((oclr.ig()>cclr.ig()) ? 
		    oclr.ig()-cclr.ig() : cclr.ig()-oclr.ig());
	      if (tmp>error) error= tmp;
       	      tmp= ((oclr.ib()>cclr.ib()) ? 
		    oclr.ib()-cclr.ib() : cclr.ib()-oclr.ib());
	      if (tmp>error) error= tmp;
	      tmp= ((oclr.ia()>cclr.ia()) ? 
		    oclr.ia()-cclr.ia() : cclr.ia()-oclr.ia());
	      if (tmp>error) error= tmp;
	    }

	cell->set_clr_error( (unsigned char)error );
      }
    }
  }

}

void octreeSampleVolume::walk_one_ray_mm(Ray& ray, LightInfo& lights,
					 const QualityMeasure& required_qual,
					 const VRenOptions options,
					 Octcell_intersect_iter<octSample>& 
					   iter)
{
  octSample *next_cell;
  int mipmap_min_level= sample_tree->get_nlevels();
  int mipmap_max_level;
  float mipmap_frac;
  float mipmap_dist_rescale;
  int mipmap_flag;
  gColor mipmap_upper_color;

  float mipmap_level_sep= voxel_mean_size;
  float ray_sep= ray.initial_separation + ray.length*ray.divergence;
  if (ray_sep <= mipmap_level_sep) {
    // voxels too big;  turn mipmapping off
    mipmap_max_level= mipmap_min_level;
    mipmap_dist_rescale= 0.0;
    mipmap_frac= 0.0;
    mipmap_flag= 0;
  }
  else {
    while ((mipmap_level_sep < ray_sep) && (mipmap_min_level > 0)) {
      mipmap_min_level -= 1;
      mipmap_level_sep *= 2;
    }
    mipmap_max_level= mipmap_min_level + 1;
    mipmap_frac= 2.0*( 1.0 - (ray_sep/mipmap_level_sep));
    mipmap_dist_rescale= 2.0*ray.divergence/mipmap_level_sep;
    mipmap_flag= 1;
  }

  if (!mipmap_flag) {
    // no need for mipmapping
    walk_one_ray(ray, lights, required_qual, options, iter);
  }
  else {
    if (ray.debug_me) {
      fprintf(stderr,
	      "walk_one_ray_mm mipmap setup: min and max levels %d %d\n",
	      mipmap_min_level, mipmap_max_level);
      fprintf(stderr,
	      "                              frac= %f, dist_rescale= %f\n",
	      mipmap_frac,mipmap_dist_rescale);
      
      while (next_cell= iter.next(ray.debug_me)) {
	fprintf(stderr,
		"walk_one_ray_mm level %d length= %f, clr= %f %f %f %f\n",
		iter.get_level(),
		ray.length,
		next_cell->clr.r(),next_cell->clr.g(),next_cell->clr.b(),
		next_cell->clr.a());
	fprintf(stderr,"                mipmap_frac= %f\n",mipmap_frac);
	
	// Escape if opaque enough, or if we have gone far enough.
	if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	    ray.rescale_me= 1;
	    break;
	}
	if ( ray.length >= ray.termination_length )
	    break;
      
	if ((iter.get_level() == mipmap_max_level)
	    || (next_cell->clr_error()
		<= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr())) { 
	  // This level is precise enough.  Leaf nodes are always good enough.
	  float dist= iter.dist_in_cell();
	  mipmap_frac += dist * mipmap_dist_rescale;
	  if (iter.get_level() == mipmap_max_level) {
	    gColor mipmap_color= mipmap_upper_color;
	    mipmap_color.mult_noclamp( mipmap_frac );
	    if (next_cell->clr.ia() 
		&& (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	      gColor mipmap_color_lower= 
		  next_cell->calc_color_o0_inline( lights, options,
						   ray.debug_me );
	      mipmap_color_lower.mult_noclamp( 1.0 - mipmap_frac );
	      mipmap_color.add_noclamp( mipmap_color_lower );
	    }
	    mipmap_color.clamp();
	    integrate_cell_o0( ray, mipmap_color, size_scale*dist );
	  }
	  else integrate_cell_o0( ray, 
				  next_cell->calc_color_o0_inline(lights,
								  options),
				  size_scale*dist );
//      ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	  fprintf(stderr, 
		  "Ray color becomes (%f %f %f %f) over dist %f\n",
		  ray.clr.r(),ray.clr.g(),
		  ray.clr.b(),ray.clr.a(), dist);
	  ray.length += dist;
	}    
	else { // Need to subdivide.
	  if (iter.get_level() == mipmap_min_level) {
	    // Save color contrib at this level for later mipmapping
	    mipmap_upper_color= next_cell->calc_color_o0_inline(lights,
								options,
								ray.debug_me);
	    fprintf(stderr,
		    "walk_one_ray_mm: mipmap_upper_color (%f %f %f %f)\n",
		    mipmap_upper_color.r(),mipmap_upper_color.g(),
		    mipmap_upper_color.b(),mipmap_upper_color.a());
	  }
	  iter.push();
	}
	if ((mipmap_frac > 1.0) && (iter.get_level() != mipmap_max_level)) {
	  // Rays have diverged; move up mipmap level
	  mipmap_frac= 0.5*(mipmap_frac - 1.0);
	  mipmap_dist_rescale *= 0.5;
	  mipmap_upper_color= gColor(); // OK since mipmap_frac approx. 0.0
	  if (mipmap_min_level) {
	    mipmap_min_level--;
	    mipmap_max_level--;
	  }
	  fprintf(stderr,
		"walk_one_ray_mm step up: levels %d %d, frac %f, rescale %f\n",
		  mipmap_min_level, mipmap_max_level, mipmap_frac, 
		  mipmap_dist_rescale);
	}
      }
    }
    else { // Faster version of same thing, without passing debug param
      while (next_cell= iter.next()) {
	  // Escape if opaque enough, or if we have gone far enough.
	if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	  ray.rescale_me= 1;
	  break;
	}
	if ( ray.length >= ray.termination_length )
	  break;
      
	if ((iter.get_level() == mipmap_max_level)
	    || (next_cell->clr_error()
		<= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr())) { 
	  // This level is precise enough.  Leaf nodes are always good enough.
	  float dist= iter.dist_in_cell();
	  mipmap_frac += dist * mipmap_dist_rescale;
	  if (iter.get_level() == mipmap_max_level) {
	    gColor mipmap_color= mipmap_upper_color;
	    mipmap_color.mult_noclamp( mipmap_frac );
	    if (next_cell->clr.ia() 
		&& (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	      gColor mipmap_color_lower= 
		  next_cell->calc_color_o0_inline( lights, options );
	      mipmap_color_lower.mult_noclamp( 1.0 - mipmap_frac );
	      mipmap_color.add_noclamp( mipmap_color_lower );
	    }
	    mipmap_color.clamp();
	    integrate_cell_o0( ray, mipmap_color, size_scale*dist );
//      ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	  }
	  else integrate_cell_o0( ray, 
				  next_cell->calc_color_o0_inline(lights,
								  options),
				  size_scale*dist );
	  ray.length += dist;
	}    
	else { // Need to subdivide.
	  if (iter.get_level() == mipmap_min_level) {
	    // Save color contrib at this level for later mipmapping
	    mipmap_upper_color= next_cell->calc_color_o0_inline(lights,
								options);
	  }
	  iter.push();
	}
	if ((mipmap_frac > 1.0) && (iter.get_level() != mipmap_max_level)) {
	  // Rays have diverged; move up mipmap level
	  mipmap_frac= 0.5*(mipmap_frac - 1.0);
	  mipmap_dist_rescale *= 0.5;
	  mipmap_upper_color= gColor(); // OK since mipmap_frac approx. 0.0
	  if (mipmap_min_level) {
	    mipmap_min_level--;
	    mipmap_max_level--;
	  }
	}
      }
    }
  }
}

void octreeSampleVolume::walk_one_ray(Ray& ray, LightInfo& lights,
				      const QualityMeasure& required_qual,
				      const VRenOptions options,
				      Octcell_intersect_iter<octSample>& iter)
{
  octSample *next_cell;

  if (ray.debug_me) {
    while (next_cell= iter.next(ray.debug_me)) {
      fprintf(stderr,
	      "walk_one_ray level %d length= %f, clr= %f %f %f %f\n",
	      iter.get_level(),
	      ray.length,
	      next_cell->clr.r(),next_cell->clr.g(),next_cell->clr.b(),
	      next_cell->clr.a());
      
      // Escape if opaque enough, or if we have gone far enough.
      if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	ray.rescale_me= 1;
	break;
      }
      if ( ray.length >= ray.termination_length )
	break;
      
      if ((next_cell->clr_error()
	   <= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr()) 
	  || (iter.get_level() == sample_tree->get_nlevels())) { 
	// This level is precise enough.  Leaf nodes are always good enough.
	float dist= iter.dist_in_cell();
	if (next_cell->clr.ia() 
	    && (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	  integrate_cell_o0( ray, 
			     next_cell->calc_color_o0_inline(lights,
							     options,
							     ray.debug_me),
			     size_scale*dist );
//      ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	}
	ray.length += dist;
	
	fprintf(stderr, 
		"Ray color becomes (%f %f %f %f) over dist %f\n",
		ray.clr.r(),ray.clr.g(),
		ray.clr.b(),ray.clr.a(), dist);
      }
      else { // Need to subdivide.
	iter.push();
      }
    }
  }
  else { // Faster version of same thing, without passing debug param
    while (next_cell= iter.next()) {
      // Escape if opaque enough, or if we have gone far enough.
      if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	ray.rescale_me= 1;
	break;
      }
      if ( ray.length >= ray.termination_length )
	break;
      
      if ((next_cell->clr_error()
	   <= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr())
	  || (iter.get_level() == sample_tree->get_nlevels())) { 
	// This level is precise enough.  Leaf nodes are always good enough.
	float dist= iter.dist_in_cell();
	if (next_cell->clr.ia() 
	    && (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	  integrate_cell_o0( ray, next_cell->calc_color_o0_inline(lights,
								  options),
			     size_scale*dist );
//      ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	}
	ray.length += dist;
      }
      else { // Need to subdivide.
	iter.push();
      }
    }
  }
}

void octreeSampleVolume::intersect( Ray *ray_queue, const int nrays, 
		LightInfo& lights, const QualityMeasure& required_qual,
				  const VRenOptions options )
{
  int i;
  gColor clear_black( 0.0, 0.0, 0.0, 0.0 );

  for (i=0; i<nrays; i++) {
    float maxlength= ray_queue[i].termination_length;
    ray_queue[i].active= 0;
    int inside= 0;

    if ((inside=grid.bbox().inside( ray_queue[i].origin 
			  + (ray_queue[i].direction*ray_queue[i].length) ))
	|| (grid.bbox().intersect( ray_queue[i].direction, 
				   ray_queue[i].origin, ray_queue[i].length, 
				   &maxlength ))) {
      // Initialize running variables
      ray_queue[i].clr= clear_black;
      lights.set_viewdir( ray_queue[i].direction );

      // Intersect against the octree.  We don\'t want to reset the
      // ray length until after the iterator is created, because its
      // own bounding box calculation can be thrown off if the given
      // initial point lies right on the boundary.
      oct_iter->reset( ray_queue[i].direction, ray_queue[i].origin,
		      ray_queue[i].length, ray_queue[i].termination_length );
      if (!inside) ray_queue[i].length= maxlength; // actually starting length
      if (options & OPT_THREED_MIPMAP) {
	if (options & OPT_TRILINEAR_INTERP)
          walk_one_ray_trilin_mm( ray_queue[i], lights, required_qual, options,
				  *oct_iter );
        else 
          walk_one_ray_mm( ray_queue[i], lights, required_qual, options, 
			   *oct_iter );
      }
      else {
	if (options & OPT_TRILINEAR_INTERP)
          walk_one_ray_trilin( ray_queue[i], lights, required_qual, options,
				   *oct_iter );
        else 
          walk_one_ray( ray_queue[i], lights, required_qual, options, 
			    *oct_iter );
      }
      ray_queue[i].active= 0; // done walking.

      // If the opacity limit was reached, terminating tracing, we need
      // to rescale the ray coloring.
      if (ray_queue[i].rescale_me) 
	ray_queue[i].clr= ray_queue[i].clr*(1.0/ray_queue[i].clr.a());

      // If we walked far enough to reach some obstacle, add in that
      // obstacle\'s color.
      if (ray_queue[i].length >= ray_queue[i].termination_length) 
	ray_queue[i].clr.add_under( ray_queue[i].termination_color );
    }
    else {
      ray_queue[i].clr= ray_queue[i].termination_color;
      ray_queue[i].length= maxlength;
      ray_queue[i].active= 0;
    }
  }
}

void octreeSampleVolume::intersect( Ray *ray_queue, const int nrays, 
		LightInfo& lights, const QualityMeasure& required_qual,
				    const VRenOptions options,
				    Octcell_intersect_iter<octSample>* iter)
{
  int i;
  gColor clear_black( 0.0, 0.0, 0.0, 0.0 );

  for (i=0; i<nrays; i++) {
    float maxlength= ray_queue[i].termination_length;
    ray_queue[i].active= 0;
    int inside= 0;

    if ((inside=grid.bbox().inside( ray_queue[i].origin 
			  + (ray_queue[i].direction*ray_queue[i].length) ))
	|| (grid.bbox().intersect( ray_queue[i].direction, 
				   ray_queue[i].origin, ray_queue[i].length, 
				   &maxlength ))) {
      // Initialize running variables
      ray_queue[i].clr= clear_black;
      lights.set_viewdir( ray_queue[i].direction );

      // Intersect against the octree.  We don\'t want to reset the
      // ray length until after the iterator is created, because its
      // own bounding box calculation can be thrown off if the given
      // initial point lies right on the boundary.
      iter->reset( ray_queue[i].direction, ray_queue[i].origin,
		   ray_queue[i].length, ray_queue[i].termination_length );
      if (!inside) ray_queue[i].length= maxlength; // actually starting length
      if (options & OPT_THREED_MIPMAP) {
	if (options & OPT_TRILINEAR_INTERP)
          walk_one_ray_trilin_mm( ray_queue[i], lights, required_qual, options,
				  *iter );
        else 
          walk_one_ray_mm( ray_queue[i], lights, required_qual, options, 
			   *iter );
      }
      else {
	if (options & OPT_TRILINEAR_INTERP)
          walk_one_ray_trilin( ray_queue[i], lights, required_qual, options,
				   *iter );
        else 
          walk_one_ray( ray_queue[i], lights, required_qual, options, 
			    *iter );
      }
      ray_queue[i].active= 0; // done walking.

      // If the opacity limit was reached, terminating tracing, we need
      // to rescale the ray coloring.
      if (ray_queue[i].rescale_me) 
	ray_queue[i].clr= ray_queue[i].clr*(1.0/ray_queue[i].clr.a());

      // If we walked far enough to reach some obstacle, add in that
      // obstacle\'s color.
      if (ray_queue[i].length >= ray_queue[i].termination_length) 
	ray_queue[i].clr.add_under( ray_queue[i].termination_color );
    }
    else {
      ray_queue[i].clr= ray_queue[i].termination_color;
      ray_queue[i].length= maxlength;
      ray_queue[i].active= 0;
    }
  }
}

