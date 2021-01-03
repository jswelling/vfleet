/****************************************************************************
 * tfun.h
 * Author Joel Welling, Dmitry Dakhnovsky
 * Copyright 1993,1995, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

class Sample;
class baseSampleVolume;
class DataVolume;

class BoundBoxTransferFunction : public baseTransferFunction {
 public:
  BoundBoxTransferFunction( const gBoundBox& bbox_in, 
			    int xsize_in, int ysize_in, int zsize_in );
  BoundBoxTransferFunction( const BoundBoxTransferFunction& other );
  baseTransferFunction *copy();
  virtual TransferFunctionType type() const { return BBOX_TFUN; }
  inline void apply( Sample &sample, int i, int j, int k, 
		     baseSampleVolume *svol, 
		     int ndata_in, DataVolume** data_table );
  void apply_row (Sample *samples, int i, int j, int kmin, int kmax, 
		     baseSampleVolume *svol, int ndata_in, 
		     DataVolume **data_table);
  void get_sizes( int *x_out, int *y_out, int *z_out ) const
  { *x_out= grid.xsize(); *y_out= grid.ysize(); *z_out= grid.zsize(); }
  const gBoundBox boundbox() const { return grid.bbox(); }
  void set_grid( const GridInfo& grid_in )
  { 
    grid= grid_in;
    bounds_ok= 0;
  }
 private:
  void recalc_bounds(baseSampleVolume* svol);
  void check_bounds(baseSampleVolume* svol)
  { if (!bounds_ok) recalc_bounds( svol ); }
  GridInfo grid;
  int bounds_ok;
  int xmin;
  int ymin;
  int zmin;
  int xmax;
  int ymax;
  int zmax;
};

class SumTransferFunction : public baseTransferFunction {     
public:
  inline SumTransferFunction( int ndata_in,
		      int ntfuns_in,
		      baseTransferFunction **tfuns_in);
  SumTransferFunction( int ndata_in,
		      int ntfuns_in,
		      float *factors_in,
		      baseTransferFunction **tfuns_in);
  inline SumTransferFunction( const SumTransferFunction& other );
  ~SumTransferFunction() { delete [] factors; delete [] tfuns; }
  baseTransferFunction* copy();
  TransferFunctionType type() const { return SUM_TFUN; }
  virtual inline void apply( Sample &sample, int i, int j, int k, 
	     baseSampleVolume *svol, 
	     int ndata_in, DataVolume** data_table );
  virtual void apply_row (Sample *samples, int i, int j, int kmin, int kmax, 
		     baseSampleVolume *svol, int ndata_in, 
		     DataVolume **data_table);
  int tfun_count() const { return ntfuns; }
  baseTransferFunction **tfun_table() const { return tfuns; }
  float *factor_table() const { return factors; }
  virtual void edit( int ndata_new, int ntfuns_new, float *factors_new,
	     baseTransferFunction **tfuns_new );
 protected:
  int ntfuns;
  float *factors;
  baseTransferFunction **tfuns;
  Sample* results;
  int results_length;
};

class TableTransferFunction : public baseTransferFunction {
public:
  TableTransferFunction( int ndata_in,
			 gBColor *table_in); /* Note: due to some bogosities,
					       we can't explicitly declare
					       that this have exactly 256
					       entries, but it *must*. */
  TableTransferFunction( const TableTransferFunction& other );
  ~TableTransferFunction() { delete table; }
  baseTransferFunction *copy();
  TransferFunctionType type() const { return TABLE_TFUN; }
  inline void apply( Sample &sample, int i, int j, int k, 
	      baseSampleVolume *svol,
	      int ndata_in, DataVolume** data_table );
  
  void apply_row( Sample *samples, int i, int j, int kmin, int kmax,
		    baseSampleVolume *svol,
		    int ndata_in, DataVolume** data_table );
  gBColor *get_table() const { return table; }
private:
  gBColor *table;
};

class GradTableTransferFunction : public baseTransferFunction {
public:

     GradTableTransferFunction( int ndata_in,
			    gBColor *table_in); /* Note: due to some 
						   bogosities, we can't
						   explicitly declare that
						   this have exactly 256
						   entries, but it *must*. */
  GradTableTransferFunction( const GradTableTransferFunction& other );
  ~GradTableTransferFunction() { delete table; }
  baseTransferFunction *copy();
  TransferFunctionType type() const { return GRADTABLE_TFUN; }
  inline void apply( Sample &sample, int i, int j, int k, 
              baseSampleVolume *svol,
              int ndata_in, DataVolume** data_table );
  void apply_row( Sample *samples, int i, int j, int kmin, int kmax,
              baseSampleVolume *svol,
		  int ndata_in, DataVolume** data_table );

  gBColor *get_table() const { return table; }
private:
  gBColor *table;
};

class MethodTransferFunction : public baseTransferFunction {
public:
  MethodTransferFunction( int ndata_in, 
			  void (*method_in)( Sample& smpl, 
					     int i, int j, int k, 
					     int ndata_in, 
					     DataVolume** data_table) )
  : baseTransferFunction( ndata_in )
  {
    method= method_in;
  }
  MethodTransferFunction( const MethodTransferFunction& o )
       : baseTransferFunction( o.ndatavol ) { method= o.method; }
  ~MethodTransferFunction() {}
  baseTransferFunction* copy();
  TransferFunctionType type() const { return METHOD_TFUN; }
  inline void apply( Sample& sample, int i, int j, int k, 
	      baseSampleVolume *svol, 
		     int ndata_in, DataVolume** data_table );
  
  void apply_row( Sample *samples, int i, int j, int kmin, int kmax,
		  baseSampleVolume *svol,
		  int ndata_in, DataVolume** data_table );
private:
  void (*method)( Sample&, int, int, int, int, DataVolume** );
};

class SSumTransferFunction : public SumTransferFunction {
public:
     inline SSumTransferFunction( int ndata_in,
				  int ntfuns_in,
				  baseTransferFunction **tfuns_in);
     SSumTransferFunction( int ndata_in,
			   int ntfuns_in,
			   float *factors_in,
			   baseTransferFunction **tfuns_in);

     inline SSumTransferFunction( const SSumTransferFunction& other );
     baseTransferFunction* copy();
     TransferFunctionType type() const { return SSUM_TFUN; }
     inline void apply( Sample &sample, int i, int j, int k, 
			baseSampleVolume *svol, 
			int ndata_in, DataVolume** data_table );
     void apply_row (Sample *samples, int i, int j, int kmin, int kmax, 
		     baseSampleVolume *svol, int ndata_in, 
		     DataVolume **data_table);
private:
     float *a_arr, *r_arr, *g_arr, *b_arr;
     float a, r, g, b;
};

class MaskTransferFunction : public baseTransferFunction {
public:
  MaskTransferFunction( int ndata_in, 
			baseTransferFunction *input_in,
			const float input_weight_in,
			baseTransferFunction *mask_in,
			const float mask_weight_in );  
  inline MaskTransferFunction( const MaskTransferFunction& other );
  ~MaskTransferFunction();
  baseTransferFunction* copy();
  TransferFunctionType type() const { return MASK_TFUN; }
  inline void apply( Sample &sample, int i, int j, int k, 
		     baseSampleVolume *svol, 
		     int ndata_in, DataVolume** data_table );
  void apply_row (Sample *samples, int i, int j, int kmin, int kmax, 
		  baseSampleVolume *svol, int ndata_in, 
		  DataVolume **data_table);
  baseTransferFunction* get_mask() const { return mask; }
  float get_mask_weight() const { return mask_weight; }
  baseTransferFunction* get_input() const { return input; }
  float get_input_weight() const { return input_weight; }
  virtual void edit( const int new_ndata, 
		     baseTransferFunction* new_input, 
		     const float new_input_weight,
		     baseTransferFunction* new_mask,
		     const float new_mask_weight );
private:
  baseTransferFunction* input;
  float input_weight;
  baseTransferFunction* mask;
  float mask_weight;
  Sample* mask_row;
  int mask_row_length;
};

class BlockTransferFunction : public baseTransferFunction {
public:
  BlockTransferFunction( const float fxmin_in, const float fymin_in, 
			 const float fzmin_in, const float fxmax_in,
			 const float fymax_in, const float fzmax_in,
			 const gBColor clr_in, const int inside_in );
  BlockTransferFunction( const float fxmin_in, const float fymin_in, 
			 const float fzmin_in, const float fxmax_in,
			 const float fymax_in, const float fzmax_in,
			 const gBColor clr_in, const int inside_in,
			 const GridInfo& grid_in );
  BlockTransferFunction( const BlockTransferFunction& other );
  ~BlockTransferFunction();
  baseTransferFunction *copy();
  virtual TransferFunctionType type() const { return BLOCK_TFUN; }
  inline void apply( Sample &sample, int i, int j, int k, 
		     baseSampleVolume *svol, 
		     int ndata_in, DataVolume** data_table );
  void apply_row (Sample *samples, int i, int j, int kmin, int kmax, 
		  baseSampleVolume *svol, int ndata_in, 
		  DataVolume **data_table);
  void get_info( float* fxmin_out, float* fymin_out, float* fzmin_out,
		 float* fxmax_out, float* fymax_out, float* fzmax_out,
		 gBColor* clr_out, int* inside_out )
  { 
    *fxmin_out= fxmin; *fymin_out= fymin; *fzmin_out= fzmin;
    *fxmax_out= fxmax; *fymax_out= fymax; *fzmax_out= fzmax;
    *clr_out= clr;
    *inside_out= inside;
  }
  virtual void edit( const float new_fxmin_in, const float new_fymin_in,
		     const float new_fzmin_in, const float new_fxmax_in,
		     const float new_fymax_in, const float new_fzmax_in,
		     const gBColor new_clr_in, const int new_inside_in );
  const GridInfo& get_grid() const { return grid; }
  void set_grid( const GridInfo& grid_in ) 
  { 
    grid= grid_in; 
    bounds_ok= 0;
  }
 private:
  void recalc_bounds(baseSampleVolume* svol);
  void check_bounds(baseSampleVolume* svol)
  { if (!bounds_ok) recalc_bounds( svol ); }
  GridInfo grid;
  gBColor clr;
  int bounds_ok;
  int inside;
  float fxmin;
  float fymin;
  float fzmin;
  float fxmax;
  float fymax;
  float fzmax;
  int xmin;
  int ymin;
  int zmin;
  int xmax;
  int ymax;
  int zmax;
};

inline void MaskTransferFunction::apply( Sample &sample, int i, int j, int k,
					 baseSampleVolume* svol,
					 int ndata_in, 
					 DataVolume** data_table )
{
  Sample mask_sample;
  if (!mask || !input) return;
  mask->apply( mask_sample, i, j, k, svol, mask->ndata(), data_table );
  input->apply( sample, i, j, k, svol, 
		input->ndata(), data_table+mask->ndata() );
  float maskval= mask_weight*mask_sample.clr.a();
  maskval *= input_weight;
  maskval= (maskval<0.0)? 0.0 : ( (maskval>1.0) ? 1.0 : maskval );
  sample.clr.scale_alpha( maskval );
}

inline void BlockTransferFunction::apply( Sample &sample, int i, int j, int k,
					  baseSampleVolume* svol,
					  int ndata_in, 
					  DataVolume** data_table )
{
  check_bounds(svol);
  if (inside) {
    if ((i>=xmin) && (i<=xmax) && (j>=ymin) && (j<=ymax)
	&& (k>=zmin) && (k<=zmax)) sample.clr= clr;
    else sample.clr.clear();
  }
  else {
    if ((i<xmin) || (i>xmax) || (j<ymin) || (j>ymax) 
	|| (k<zmin) || (k>zmax)) sample.clr= clr;
    else sample.clr.clear();
  }
}

inline void SumTransferFunction::apply( Sample &sample, int i, int j, int k, 
				 baseSampleVolume *svol, 
				 int ndata_in, DataVolume** data_table )
{
  // Okay... now we go through our tfun list and pass each the
  // appropriate number of datavolumes. :\)

  DataVolume **send = data_table;
  Sample result;
  sample.clr.clear();

  for (int lupe = 0; lupe < ntfuns; lupe++) {
    result.clr.clear();
    tfuns[lupe]->apply(result, i, j, k, svol, tfuns[lupe]->ndata(), send);
    sample.clr += result.clr * factors[lupe];
    send += tfuns[lupe]->ndata();
  }
}

inline void TableTransferFunction::apply( Sample &sample, int i, int j, int k, 
				 baseSampleVolume *svol, 
				 int ndata_in, DataVolume** data_table )
{
     sample.clr = table[data_table[0]->val(i, j, k)];
}

inline void GradTableTransferFunction::apply( Sample &sample, 
				      int i, int j, int k,
				      baseSampleVolume *svol,
				      int ndata_in, DataVolume **data_table )
{
#ifdef DEBUG
    if (ndata_in != ndatavol)
      fprintf(stderr, "Warning: GradTableTransferFunction::apply was passed the wrong number of datavolumes.\n");
    if (ndata_in < 1) {
      fprintf(stderr, "Warning: GradTableTransferFunction::apply wasn't passed any datavolumes.\n");
      return;
    }
#endif

    float grad_scale= (*data_table)->gradient(i,j,k).length()
       / (*data_table)->max_gradient_magnitude();
	

    sample.clr= table[(*data_table)->val(i, j, k)];
    sample.clr= gBColor( sample.clr.r(), sample.clr.g(), sample.clr.b(),
                         grad_scale*sample.clr.a() );
}

inline void BoundBoxTransferFunction::apply( Sample &sample, 
					     int i, int j, int k,
					     baseSampleVolume *svol,
					     int ndata_in, 
					     DataVolume **data_table )
{
  check_bounds(svol);
	
  if (((j==ymin) || (j==ymin+1)) && ((k==zmin)||(k==zmin+1))) { // x axis
    sample.clr= gBColor( 255, 0, 0, 255 );
  }
  else if (((k==zmin) || (k==zmin+1)) && ((i==xmin)||(i==xmin+1))) { // y axis
    sample.clr= gBColor( 0, 255, 0, 255 );
  }
  else if (((i==xmin) || (i==xmin+1)) && ((j==ymin)||(j==ymin+1))) { // z axis
    sample.clr= gBColor( 0, 0, 255, 255 );
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
    sample.clr= gBColor( 255, 255, 255, 255 );
  }
  else sample.clr.clear();
}

inline void MethodTransferFunction::apply ( Sample& sample, int i, int j, int k, 
	      baseSampleVolume *svol, 
	      int ndata_in, DataVolume** data_table )
{
    if (ndata_in != ndatavol)
      fprintf(stderr, "Warning: MethodTransferFunction::apply was passed the wrong number of datavolumes.\n");
    (*method)( sample, i, j, k, ndata_in, data_table ); 
}

inline void SSumTransferFunction::apply( Sample &sample, int i, int j, int k, 
					baseSampleVolume *svol, 
					int ndata_in, DataVolume** data_table )
{
     // Okay... now we go through our tfun list and pass each the
     // appropriate number of datavolumes. :\)
     
     DataVolume **send = data_table;
     Sample result;
     sample.clr.clear();
     float a, r, g, b;
     
     a = r = g = b = (float) 1.0;
     
     for (int lupe = 0; lupe < ntfuns; lupe++) {
	  result.clr.clear();
	  tfuns[lupe]->apply(result, i, j, k, svol,
			     tfuns[lupe]->ndata(), send);
	  
	  a *= (1.0 - result.clr.a() * factors[lupe]);
	  r *= (1.0 - result.clr.r() * result.clr.a() * factors[lupe]);
	  g *= (1.0 - result.clr.g() * result.clr.a() * factors[lupe]);
	  b *= (1.0 - result.clr.b() * result.clr.a() * factors[lupe]);
	  
	  //sample.clr += result.clr * factors[lupe];
	  send += tfuns[lupe]->ndata();
     }
     float a_inv= ((1-a)>0.0) ? 1.0/(1.0-a) : 1.0;
     sample.clr = gBColor (a_inv*(1 - r), a_inv*(1 - g), a_inv*(1 - b), 1 - a);
}

