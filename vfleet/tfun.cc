/****************************************************************************
 * tfun.cc
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
#include <string.h>
#include "vren.h"
#include "tfun.h"

/*                                               */
/*  SumTransferFunction class member definitions */
/*                                               */
inline 
SumTransferFunction::SumTransferFunction( int ndata_in, int ntfuns_in,
					  baseTransferFunction **tfuns_in )
: baseTransferFunction(ndata_in) {
  // The most important thing to do in this bit is to confirm
  // that the number of datasets we\'ll be later asked to render
  // is the number of datasets our subtfuns can render.
  ndatavol=0;
  tfuns= new baseTransferFunction*[ntfuns_in];
  factors= new float[ntfuns_in];
  results= NULL;
  results_length= 0;
  
  for(int i=0; i<ntfuns_in; i++) {
    factors[i]= 1.0;
    tfuns[i] = tfuns_in[i];
    ndatavol+=tfuns_in[i]->ndata();
  }

  if (ndatavol != ndata_in)
    fprintf(stderr, 
	    "Warning: SumTransferFunction::SumTransferFunction was passed conflicting numbers of renderable datasets.\n");
  // In any event, we trust our calculated ndatavol above all else...
  ntfuns = ntfuns_in;
}

SumTransferFunction::SumTransferFunction( int ndata_in, int ntfuns_in,
					  float *factors_in,
					  baseTransferFunction **tfuns_in )
: baseTransferFunction(ndata_in) {
  // The most important thing to do in this bit is to confirm
  // that the number of datasets we\'ll be later asked to render
  // is the number of datasets our subtfuns can render.
  ndatavol=0;
  tfuns= new baseTransferFunction*[ntfuns_in];
  factors= new float[ntfuns_in];
  results= NULL;
  results_length= 0;

  for(int i=0; i<ntfuns_in; i++) {
    factors[i]= factors_in[i];
    tfuns[i] = tfuns_in[i];
    ndatavol+=tfuns_in[i]->ndata();
  }

  if (ndatavol != ndata_in)
    fprintf(stderr, 
	    "Warning: SumTransferFunction::SumTransferFunction was passed conflicting numbers of renderable datasets.\n");
  // In any event, we trust our calculated ndatavol above all else...
  ntfuns = ntfuns_in;
}

inline SumTransferFunction::SumTransferFunction( const SumTransferFunction& other ) 
: baseTransferFunction( other.ndatavol )
{
  ntfuns= other.ntfuns;
  factors= new float[ntfuns];
  tfuns= new baseTransferFunction*[ntfuns];
  results= NULL;
  results_length= 0;

  for(int i=0; i<ntfuns; i++) {
    factors[i]= other.factors[i];
    tfuns[i] = other.tfuns[i];
  }
}

void SumTransferFunction::apply_row (Sample *samples, int i, int j, int kmin, 
		int kmax, baseSampleVolume *svol, 
		int ndata_in, DataVolume** data_table )
{

     DataVolume **send = data_table;

     if (results_length < kmax+1) {
	  results_length= kmax+1;
	  delete [] results;
	  results= new Sample[kmax+1];
     }
	  
     for (int k = 0; k <= kmax; k++)
	  samples[k].clr.clear();
     
     for (int lupe = 0; lupe < ntfuns; lupe++) {
	  tfuns[lupe]->apply_row (results, i, j, kmin, kmax, svol, 
				  tfuns[lupe]->ndata(), send);
	  
	  for (int k = kmin; k <= kmax; k++) 
	       samples[k].clr += results[k].clr * factors[lupe];
	  
	  send += tfuns[lupe]->ndata();
     }
}

baseTransferFunction* SumTransferFunction::copy()
{
  SumTransferFunction *result= new SumTransferFunction( *this );

  for (int i=0; i<ntfuns; i++) 
    result->tfuns[i]= tfuns[i]->copy();

  return result;
}

void SumTransferFunction::edit( int ndata_new, int ntfuns_new,
				float *factors_new,
				baseTransferFunction **tfuns_new )
{
  ndatavol= ndata_new;
  ntfuns= ntfuns_new;

  delete [] tfuns;
  delete [] factors;

  factors= new float[ntfuns];
  tfuns= new baseTransferFunction*[ntfuns];

  for(int i=0; i<ntfuns; i++) {
    factors[i]= factors_new[i];
    tfuns[i] = tfuns_new[i];
  }
}

/*                                                 */
/*  TableTransferFunction class member definitions */
/*                                                 */

TableTransferFunction::TableTransferFunction( int ndata_in,
			gBColor *table_in) /* Note: due to some bogosities,
					      we can't explicitly declare
					      that this have exactly 256
					      entries, but it *must*. */
: baseTransferFunction(ndata_in) 
{
  if (ndata_in != 1) {
    fprintf(stderr,
	    "TableTransferFunction: only one input dataset permitted!\n");
    ndatavol= 1;
  }

  table= new gBColor[256];

  if (table_in)
    for (int i=0; i<256; i++) table[i]= table_in[i];
}

 TableTransferFunction::TableTransferFunction( const TableTransferFunction& o )
: baseTransferFunction(o.ndatavol)
{
  table= new gBColor[256];
  for (int i=0; i<256; i++) table[i]= o.table[i];
}

void TableTransferFunction::apply_row (Sample *samples, int i, int j, int kmin,
		int kmax, baseSampleVolume *svol,
		int ndata_in, DataVolume** data_table )
{
	for (int k = kmin; k <= kmax; k++) {
		apply (samples[k], i, j, k, svol, ndata_in, data_table);
	}
}

baseTransferFunction* TableTransferFunction::copy()
{
  return new TableTransferFunction( *this );
}

/*                                                     */
/*  GradTableTransferFunction class member definitions */
/*                                                     */
     
GradTableTransferFunction::GradTableTransferFunction( int ndata_in,
			gBColor *table_in) /* Note: due to some bogosities,
					      we can't explicitly declare
					      that this have exactly 256
					      entries, but it *must*. */
: baseTransferFunction(ndata_in) 
{
  if (ndata_in != 1) {
    fprintf(stderr,
	    "TableTransferFunction: only one input dataset permitted!\n");
    ndatavol= 1;
  }

  table= new gBColor[256];

  if (table_in)
    for (int i=0; i<256; i++) table[i]= table_in[i];
}

GradTableTransferFunction::GradTableTransferFunction( 
		              const GradTableTransferFunction& o )
: baseTransferFunction(o.ndatavol)
{
  table= new gBColor[256];
  for (int i=0; i<256; i++) table[i]= o.table[i];
}

baseTransferFunction* GradTableTransferFunction::copy()
{
  return new GradTableTransferFunction( *this );
}

void GradTableTransferFunction::apply_row (Sample *samples, int i, int j,
					   int kmin, int kmax,
					   baseSampleVolume *svol,
					   int ndata_in,
					   DataVolume** data_table )
{
	for (int k = kmin; k <= kmax; k++) {
		apply (samples[k], i, j, k, svol, ndata_in, data_table);
	}
}

BoundBoxTransferFunction::BoundBoxTransferFunction( const gBoundBox& bbox_in,
						    int xsize_in, int ysize_in,
						    int zsize_in )
  : baseTransferFunction( 0 ), grid( xsize_in, ysize_in, zsize_in, bbox_in )
{
  xmin= xmax= ymin= ymax= zmin= zmax= 0;
  bounds_ok= 0;
}

BoundBoxTransferFunction::BoundBoxTransferFunction( 
				    const BoundBoxTransferFunction& other )
  : baseTransferFunction( 0 ), grid( other.grid )
{
  xmin= xmax= ymin= ymax= zmin= zmax= 0;
  bounds_ok= 0;
}

void BoundBoxTransferFunction::recalc_bounds( baseSampleVolume* svol )
{
  // Boost bounds indices to values appropriate for the original volume
  // of definition, in case this method is being called for a subvolume.

  float subvol_rel_xmin= ((svol->boundbox().xmin() - grid.bbox().xmin())
			  / (grid.bbox().xmax() - grid.bbox().xmin()));
  float subvol_rel_ymin= ((svol->boundbox().ymin() - grid.bbox().ymin())
                          / (grid.bbox().ymax() - grid.bbox().ymin()));
  float subvol_rel_zmin= ((svol->boundbox().zmin() - grid.bbox().zmin())
			  / (grid.bbox().zmax() - grid.bbox().zmin()));

  float subvol_x_ratio= (grid.bbox().xmax() - grid.bbox().xmin())
    / (svol->boundbox().xmax() - svol->boundbox().xmin());
  float subvol_y_ratio= (grid.bbox().ymax() - grid.bbox().ymin())
    / (svol->boundbox().ymax() - svol->boundbox().ymin());
  float subvol_z_ratio= (grid.bbox().zmax() - grid.bbox().zmin())
    / (svol->boundbox().zmax() - svol->boundbox().zmin());

  xmin= (int)(((-subvol_rel_xmin)*subvol_x_ratio)*svol->xsize() + 0.5);
  xmax= (int)(((1.0-subvol_rel_xmin)*subvol_x_ratio)*svol->xsize() - 0.5);
  ymin= (int)(((-subvol_rel_ymin)*subvol_y_ratio)*svol->ysize() + 0.5);
  ymax= (int)(((1.0-subvol_rel_ymin)*subvol_y_ratio)*svol->ysize() - 0.5);
  zmin= (int)(((-subvol_rel_zmin)*subvol_z_ratio)*svol->zsize() + 0.5);
  zmax= (int)(((1.0-subvol_rel_zmin)*subvol_z_ratio)*svol->zsize() - 0.5);

  bounds_ok= 1;
}

void BoundBoxTransferFunction::apply_row(Sample *samples, 
					 int i, int j, int kmin, int kmax, 
					 baseSampleVolume *svol, int ndata_in, 
					 DataVolume** data_table )
{
  check_bounds(svol);
	
  int k;
  if ((i!=xmin) && (i!=xmin+1) && (i!=xmax) 
      && (j!=ymin) && (j!=ymin+1) && (j!=ymax)) {
    for (k=kmin; k<=kmax; k++) samples[k].clr.clear();
  }
  else {
    for (k = kmin; k <= kmax; k++) {
      if (((j==ymin)||(j==ymin+1)) 
	  && ((k==zmin)||(k==zmin+1))) { // x axis
	samples[k].clr= gBColor( 255, 0, 0, 255 );
      }
      else if (((k==zmin)||(k==zmin+1)) 
	       && ((i==xmin)||(i==xmin+1))) { // y axis
	samples[k].clr= gBColor( 0, 255, 0, 255 );
      }
      else if (((i==xmin)||(i==xmin+1)) 
	       && ((j==ymin)||(j==ymin+1))) { // z axis
	samples[k].clr= gBColor( 0, 0, 255, 255 );
      }
      else if ( (j==ymax && k==zmax)
		|| (k==zmax && i==xmax)
		|| (i==xmax && j==ymax)
		|| (i==xmin && j==ymax)
		|| (j==ymin && i==xmax)
		|| (j==ymin && k==zmax)
		|| (k==zmin && j==ymax)
		|| (i==xmin && k==zmax)
		|| (k==zmin && i==xmax) ) {
	samples[k].clr= gBColor( 255, 255, 255, 255 );
      }
      else samples[k].clr.clear();
    }
  }
}

baseTransferFunction* BoundBoxTransferFunction::copy()
{
  return new BoundBoxTransferFunction( *this );
}

/*                                                  */
/*  MethodTransferFunction class member definitions */
/*                                                  */


baseTransferFunction* MethodTransferFunction::copy()
{
  return new MethodTransferFunction( *this );
}

void MethodTransferFunction::apply_row (Sample *samples, int i, int j, int kmin, 
		int kmax, baseSampleVolume *svol, 
		int ndata_in, DataVolume** data_table )
{
     for (int k = kmin; k <= kmax; k++) {
               apply (samples[k], i, j, k, svol, ndata_in, data_table);
     }
}

/*                                                    */
/*  baseTransferFunction class member definitions */
/*                                                    */

baseTransferFunction* baseTransferFunction::copy()
{
  fprintf(stderr,"baseTransferFunction::copy() called!\n");
  exit(-1);
  return NULL;
}


/*                                               */
/*  SSumTransferFunction class member definitions */
/*                                               */
inline 
SSumTransferFunction::SSumTransferFunction( int ndata_in, int ntfuns_in,
					    baseTransferFunction **tfuns_in )
     : SumTransferFunction (ndata_in, ntfuns_in, tfuns_in)
{     
     a_arr = r_arr = g_arr = b_arr = NULL;
}

SSumTransferFunction::SSumTransferFunction( int ndata_in, int ntfuns_in,
					  float *factors_in,
					  baseTransferFunction **tfuns_in )
: SumTransferFunction (ndata_in, ntfuns_in, factors_in, tfuns_in)
{
     a_arr = r_arr = g_arr = b_arr = NULL;
}

inline SSumTransferFunction::SSumTransferFunction
( const SSumTransferFunction& other ) 
     : SumTransferFunction (other)
{
     a_arr = r_arr = g_arr = b_arr = NULL;
}

void SSumTransferFunction::apply_row (Sample *samples, int i, int j, int kmin, 
				     int kmax, baseSampleVolume *svol, 
				     int ndata_in, DataVolume** data_table )
{
     
     DataVolume **send = data_table;

     if (results_length < kmax+1) {
	  results_length= kmax+1;
	  delete [] results;
	  delete [] a_arr;
	  delete [] r_arr;
	  delete [] g_arr;
	  delete [] b_arr;
	  results= new Sample[kmax+1];
	  a_arr = new float[kmax + 1];
	  r_arr = new float[kmax + 1];
	  g_arr = new float[kmax + 1];
	  b_arr = new float[kmax + 1];
     }
     
     for (int k = 0; k <= kmax; k++) {
	  a_arr[k] = r_arr[k] = g_arr[k] = b_arr[k] = (float) 1.0;
	  samples[k].clr.clear();
     }
     
     
     for (int lupe = 0; lupe < ntfuns; lupe++) {
	  tfuns[lupe]->apply_row (results, i, j, kmin, kmax, svol, 
				  tfuns[lupe]->ndata(), send);
	  
	  for (int k = kmin; k <= kmax; k++) {
	       a_arr[k] *= (float)(1.0 - results[k].clr.a() * factors[lupe]);
	       r_arr[k] *= (float)(1.0 - results[k].clr.r() *
				   results[k].clr.a() * factors[lupe]);
	       g_arr[k] *= (float)(1.0 - results[k].clr.g() *
				   results[k].clr.a() * factors[lupe]);
	       b_arr[k] *= (float)(1.0 - results[k].clr.b() *
				   results[k].clr.a() * factors[lupe]);
	       //samples[k].clr += results[k].clr * factors[lupe];
	  }
	  
	  send += tfuns[lupe]->ndata();
     }
     
     for (int k = kmin; k <= kmax; k++) {
       float a_inv= (1-a_arr[k] > 0.0) ? 1.0/(1 - a_arr[k]) : 1.0;
	  samples[k].clr = gBColor
	       (a_inv*(1 - r_arr[k]), a_inv*(1 - g_arr[k]), 
		a_inv*(1 - b_arr[k]), 1 - a_arr[k]);
     }
}

baseTransferFunction* SSumTransferFunction::copy()
{
     SSumTransferFunction *result= new SSumTransferFunction( *this );
     
     for (int i=0; i<ntfuns; i++) 
	  result->tfuns[i]= tfuns[i]->copy();
     
     return result;
}

//
// Mask transfer function member defs
//

MaskTransferFunction::MaskTransferFunction( int ndata_in,
					    baseTransferFunction* input_in,
					    const float input_weight_in,
					    baseTransferFunction* mask_in,
					    const float mask_weight_in ) 
  : baseTransferFunction( ndata_in )
{
  input= input_in;
  input_weight= input_weight_in;
  mask= mask_in;
  mask_weight= mask_weight_in;
  mask_row= NULL;
  mask_row_length= 0;

  if (input && mask 
      && (ndata() != input->ndata() + mask->ndata())) {
    fprintf(stderr,
	    "MaskTransferFunction: data count mismatch (%d != %d+%d)!\n",
	    ndata(), mask->ndata(), input->ndata());
    ndatavol= mask->ndata() + input->ndata();
  }
}

inline MaskTransferFunction::MaskTransferFunction( const 
						   MaskTransferFunction& o )
  : baseTransferFunction( o.ndata() )
{
  input= o.input;
  input_weight= o.input_weight;
  mask= o.mask;
  mask_weight= o.mask_weight;
  mask_row= NULL;
  mask_row_length= 0;
}

MaskTransferFunction::~MaskTransferFunction()
{
  // Handler will take care of deletion of children.
}

baseTransferFunction* MaskTransferFunction::copy()
{
  MaskTransferFunction* result= new MaskTransferFunction( *this );
  if (input) result->input= input->copy();
  else result->input= NULL;
  if (mask) result->mask= mask->copy();
  else result->mask= NULL;

  return result;
}

void MaskTransferFunction::apply_row( Sample* samples, int i, int j, 
				      int kmin, int kmax,
				      baseSampleVolume* svol, int ndata_in,
				      DataVolume** data_table )
{
  if (!mask || !input) return;

  // Check for space
  if (mask_row_length < kmax+1) {
    mask_row_length= kmax+1;
    delete [] mask_row;
    mask_row= new Sample[kmax+1];
  }
  
  int k;
  int kmin_new;
  int kmax_new;

  mask->apply_row( mask_row, i, j, kmin, kmax, svol, 
		   mask->ndata(), data_table );

  for (k=kmin; k<kmax; k++) if (mask_row[k].clr.ia()) break;
  kmin_new= k;
  for (k=kmax; k>kmin_new; k--) if (mask_row[k].clr.ia()) break;
  kmax_new= k;
  if (kmin_new==kmax_new && !(mask_row[kmin_new].clr.ia())) {
    // All blank
    for (k=kmin; k<=kmax; k++) samples[k].clr.clear();
  }
  else {
    input->apply_row( samples, i, j, kmin_new, kmax_new, svol, 
		      input->ndata(), data_table+mask->ndata() );
    for (k=kmin; k<kmin_new; k++) samples[k].clr.clear();
    for (k=kmin_new; k<=kmax_new; k++) {
      float maskval= mask_weight*mask_row[k].clr.a();
      maskval *= input_weight;
      maskval= (maskval<0.0)? 0.0 : ( (maskval>1.0) ? 1.0 : maskval );
      samples[k].clr.scale_alpha( maskval );
    }
    for (k=kmax_new+1; k<=kmax; k++) samples[k].clr.clear();
  }
}

void MaskTransferFunction::edit( const int new_ndata, 
				 baseTransferFunction* new_input,
				 const float new_input_weight,
				 baseTransferFunction* new_mask,
				 const float new_mask_weight )
{
  ndatavol= new_ndata;
  input= new_input;
  input_weight= new_input_weight;
  mask= new_mask;
  mask_weight= new_mask_weight;
  if (input && mask 
      && (ndata() != input->ndata() + mask->ndata())) {
    fprintf(stderr,
	    "MaskTransferFunction::edit: data count mismatch (%d != %d+%d)!\n",
	    ndata(), mask->ndata(), input->ndata());
    ndatavol= mask->ndata() + input->ndata();
  }
}

//
// BlockTransferFunction member defs
//

BlockTransferFunction::BlockTransferFunction( const float fxmin_in, 
					      const float fymin_in, 
					      const float fzmin_in, 
					      const float fxmax_in,
					      const float fymax_in, 
					      const float fzmax_in,
					      const gBColor clr_in, 
					      const int inside_in )
  : baseTransferFunction(0)
{
  fxmin= fxmin_in;
  fymin= fymin_in;
  fzmin= fzmin_in;
  fxmax= fxmax_in;
  fymax= fymax_in;
  fzmax= fzmax_in;
  clr= clr_in;
  inside= inside_in;
  xmin= ymin= zmin= xmax= ymax= zmax= 0;
  bounds_ok= 0;
}

BlockTransferFunction::BlockTransferFunction( const float fxmin_in, 
					      const float fymin_in, 
					      const float fzmin_in, 
					      const float fxmax_in,
					      const float fymax_in, 
					      const float fzmax_in,
					      const gBColor clr_in, 
					      const int inside_in,
					      const GridInfo& grid_in )
  : baseTransferFunction(0), grid(grid_in)
{
  fxmin= fxmin_in;
  fymin= fymin_in;
  fzmin= fzmin_in;
  fxmax= fxmax_in;
  fymax= fymax_in;
  fzmax= fzmax_in;
  clr= clr_in;
  inside= inside_in;
  xmin= ymin= zmin= xmax= ymax= zmax= 0;
  bounds_ok= 0;
}

BlockTransferFunction::BlockTransferFunction( const BlockTransferFunction& o )
  : baseTransferFunction(0), grid(o.grid)
{
  fxmin= o.fxmin;
  fymin= o.fymin;
  fzmin= o.fzmin;
  fxmax= o.fxmax;
  fymax= o.fymax;
  fzmax= o.fzmax;
  clr= o.clr;
  inside= o.inside;
  xmin= ymin= zmin= xmax= ymax= zmax= 0;
  bounds_ok= 0;
}

BlockTransferFunction::~BlockTransferFunction()
{
  // Nothing to delete
}

baseTransferFunction* BlockTransferFunction::copy()
{
  return new BlockTransferFunction( *this );
}

void BlockTransferFunction::apply_row (Sample *samples, int i, int j, 
				       int kmin, int kmax, 
				       baseSampleVolume *svol, int ndata_in, 
				       DataVolume **data_table)
{
  check_bounds(svol);
  int k;
  if (inside) {
    if (i<xmin || i>xmax || j<ymin || j>ymax) 
      for (k=kmin; k<=kmax; k++) samples[k].clr.clear();
    else {
      if (kmin < zmin) {
	if (kmax > zmax) {  // ------******--------
	  for (k=kmin; k<zmin; k++) samples[k].clr.clear();
	  for (k=zmin; k<=zmax; k++) samples[k].clr= clr;
	  for (k=zmax+1; k<=kmax; k++) samples[k].clr.clear();
	}
	else if (kmax > zmin) { // --------**********
	  for (k=kmin; k<zmin; k++) samples[k].clr.clear();
	  for (k=zmin; k<=kmax; k++) samples[k].clr= clr;
	}
	else { // -----------------
	  for (k=kmin; k<=kmax; k++) samples[k].clr.clear();
	}
      }
      else {
	if (kmax > zmax) { // ******---------
	  for (k=kmin; k<=zmax; k++) samples[k].clr= clr;
	  for (k=zmax+1; k<=kmax; k++) samples[k].clr.clear();
	}
	else { // ************
	  for (k=kmin; k<=kmax; k++) samples[k].clr= clr;
	}
      }
    }
  }
  else {
    if (i<xmin || i>xmax || j<ymin || j>ymax) 
      for (k=kmin; k<=kmax; k++) samples[k].clr= clr;
    else {
      if (kmin < zmin) {
	if (kmax > zmax) {  // ******------********
	  for (k=kmin; k<zmin; k++) samples[k].clr= clr;
	  for (k=zmin; k<=zmax; k++) samples[k].clr.clear();
	  for (k=zmax+1; k<=kmax; k++) samples[k].clr= clr;
	}
	else if (kmax > zmin) { // ********----------
	  for (k=kmin; k<zmin; k++) samples[k].clr= clr;
	  for (k=zmin; k<=kmax; k++) samples[k].clr.clear();
	}
	else { // *****************
	  for (k=kmin; k<=kmax; k++) samples[k].clr= clr;
	}
      }
      else {
	if (kmax > zmax) { // ------*********
	  for (k=kmin; k<=zmax; k++) samples[k].clr.clear();
	  for (k=zmax+1; k<=kmax; k++) samples[k].clr= clr;
	}
	else { // ------------
	  for (k=kmin; k<=kmax; k++) samples[k].clr.clear();
	}
      }
    }
  }
}

void BlockTransferFunction::edit( const float new_fxmin_in, 
				  const float new_fymin_in,
				  const float new_fzmin_in, 
				  const float new_fxmax_in,
				  const float new_fymax_in, 
				  const float new_fzmax_in,
				  const gBColor new_clr_in, 
				  const int new_inside_in )
{
  fxmin= new_fxmin_in;
  fymin= new_fymin_in;
  fzmin= new_fzmin_in;
  fxmax= new_fxmax_in;
  fymax= new_fymax_in;
  fzmax= new_fzmax_in;
  clr= new_clr_in;
  inside= new_inside_in;
  bounds_ok= 0;
}

void BlockTransferFunction::recalc_bounds( baseSampleVolume* svol )
{
  // Boost bounds indices to values appropriate for the original volume
  // of definition, in case this method is being called for a subvolume.

  float subvol_rel_xmin= ((svol->boundbox().xmin() - grid.bbox().xmin())
			  / (grid.bbox().xmax() - grid.bbox().xmin()));
  float subvol_rel_ymin= ((svol->boundbox().ymin() - grid.bbox().ymin())
                          / (grid.bbox().ymax() - grid.bbox().ymin()));
  float subvol_rel_zmin= ((svol->boundbox().zmin() - grid.bbox().zmin())
			  / (grid.bbox().zmax() - grid.bbox().zmin()));

  float subvol_x_ratio= (grid.bbox().xmax() - grid.bbox().xmin())
    / (svol->boundbox().xmax() - svol->boundbox().xmin());
  float subvol_y_ratio= (grid.bbox().ymax() - grid.bbox().ymin())
    / (svol->boundbox().ymax() - svol->boundbox().ymin());
  float subvol_z_ratio= (grid.bbox().zmax() - grid.bbox().zmin())
    / (svol->boundbox().zmax() - svol->boundbox().zmin());

  xmin= (int)(((fxmin-subvol_rel_xmin)*subvol_x_ratio)*svol->xsize() + 0.5);
  xmax= (int)(((fxmax-subvol_rel_xmin)*subvol_x_ratio)*svol->xsize() + 0.5);
  ymin= (int)(((fymin-subvol_rel_ymin)*subvol_y_ratio)*svol->ysize() + 0.5);
  ymax= (int)(((fymax-subvol_rel_ymin)*subvol_y_ratio)*svol->ysize() + 0.5);
  zmin= (int)(((fzmin-subvol_rel_zmin)*subvol_z_ratio)*svol->zsize() + 0.5);
  zmax= (int)(((fzmax-subvol_rel_zmin)*subvol_z_ratio)*svol->zsize() + 0.5);

  bounds_ok= 1;
}
