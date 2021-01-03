/****************************************************************************
 * octree.h
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

/* Notes-
 */

#define OCTREE_PERFECT_CELL_SIZE 8
#define OCTREE_PERFECT_CELL_SHIFT 3

class baseOctree;
class baseOctcell_kids_iter;
class baseOctcell_intersect_iter;

class baseOctree {
friend class baseOctcell_kids_iter;
friend class baseOctcell_intersect_iter;
public:
  baseOctree(const gBoundBox& bounds_in, 
	     int xdim_in, int ydim_in, int zdim_in, 
	     int cellsize_in, void *(*allocate_class_array)( int length ),
	     void (*delete_class_array)( void* ));
  ~baseOctree();
  void* top() const { return top_cell; }
  int get_nlevels() const { return nlevels; }
  void *get_cell( const int level, 
		  const int i, const int j, const int k ) const
  {
#ifdef never
    if ((level > nlevels)
	|| (i>=width_x[level])) return NULL; 
#endif
    if (perfect) {
      if (i<width_x[level])
	return (void*)((char*)(start_points[level][i]) 
		       + (((j << level)+k) << OCTREE_PERFECT_CELL_SHIFT));
      else return NULL;
    }
    else if ((i < width_x[level])
	     && (j < width_y[level])
	     && (k < width_z[level])) {
      int offset= (j*width_z[level]) + k;
      return (void*)((char*)(start_points[level][i]) 
		     + offset*cellsize);
    }
    else return NULL;
  }
  void *get_cell( const int level,
		  const int i, const int j, const int k, 
		  const int morton ) const
  {
    return get_cell( level,
		     i + (morton & 1),
		     j + ((morton & 2) >> 1),
		     k + ((morton & 4) >> 2) );			  
  }
  void *get_child( const int pt_level, 
		   const int pt_i, const int pt_j, const int pt_k,
		   const int morton_index ) const
  {
    return get_cell( pt_level+1, 
		     pt_i << 1 + (morton_index & 1),
		     pt_j << 1 + ((morton_index & 2) >> 1),
		     pt_k << 1 + ((morton_index & 4) >> 2) );
  }
  void get_level_dims( const int level_in,
		      int* xdim_in, int* ydim_in, int* zdim_in ) const
  {
    *xdim_in= width_x[level_in];
    *ydim_in= width_y[level_in];
    *zdim_in= width_z[level_in];
  }
  int operator==( const baseOctree& other ) const
  { 
    return (top_cell==other.top_cell && nlevels==other.nlevels 
	    && bounds==other.bounds && extended_bounds==other.extended_bounds
	    && scale==other.scale && inv_scale==other.inv_scale
	    && xdim==other.xdim && ydim==other.ydim && zdim==other.zdim
	    && cellsize==other.cellsize);
  }
  int operator!=( const baseOctree& other ) const
  { 
    return (top_cell!=other.top_cell || nlevels!=other.nlevels 
	    || bounds!=other.bounds || extended_bounds!=other.extended_bounds
	    || scale!=other.scale || inv_scale!=other.inv_scale
	    || xdim!=other.xdim || ydim!=other.ydim || zdim!=other.zdim
	    || cellsize!=other.cellsize);
  }
protected:
  void initialize_leaves( void (*init_method)(void* cell, 
					      int i, int j, int kmin, int kmax, void *init),
			  void *init );
  void *top_cell;
  int nlevels;
  void ***start_points;
  void (*delete_method)( void* );
  gBoundBox bounds;
  gBoundBox extended_bounds;
  gVector scale;      // used by baseOctcell_intersect_iter
  gVector inv_scale;  // likewise
  int xdim;
  int ydim;
  int zdim;
  int *width_x;
  int *width_y;
  int *width_z;
  int cellsize;
  int perfect;
};

template< class T >
class Octree : public baseOctree {
private:
  static void* allocate_class_array( int length )
    { return( new T[length] ); }
  static void delete_class_array( void* class_array )
  { delete [] (T*)class_array; }
  void (*leaf_init_method)( void* cell, int i, int j, int kmin, int kmax, void *init );
public:
  T* top() const { return (T*)baseOctree::top(); }     
  Octree(const gBoundBox& bounds_in, int xdim_in, int ydim_in, int zdim_in, 
	 void (*leaf_initialize)(void* cell, int i, int j, int kmin, int kmax,
				 void *init_data),
	 void *init)
  : baseOctree( bounds_in, xdim_in, ydim_in, zdim_in, sizeof(T),
		Octree<T>::allocate_class_array, 
	       Octree<T>::delete_class_array )
  { 
    leaf_init_method= leaf_initialize;
    initialize_leaves( leaf_init_method, init );
  }
  void reinitialize( void *init ) 
  { initialize_leaves( leaf_init_method, init ); }
  T* get_cell( const int level,
	       const int i, const int j, const int k ) const 
  { return( (T*)baseOctree::get_cell(level,i,j,k) ); }
};

// Iterates over all children
class baseOctcell_kids_iter {
public:
  baseOctcell_kids_iter( const baseOctree* tree_in )
  {
    index= -1;
    level= 1;
    i= j= k= 0;
    tree= tree_in;
#ifdef never
    fprintf(stderr,"top-level constructor.\n");
#endif
  }
  baseOctcell_kids_iter( const baseOctcell_kids_iter& iter ) // copy
  {
    index= iter.index;
    level= iter.level;
    i= iter.i;
    j= iter.j;
    k= iter.k;
    tree= iter.tree;
  }
protected:
  baseOctcell_kids_iter(const baseOctcell_kids_iter& iter, 
			const int junk) // child iter
  {
    if (iter.index<0) {
      fprintf(stderr,
   "baseOctcell_kids_iter:: child constructor error: called before next!\n");
      abort();
    }
    index= -1;
    level= iter.level + 1;
    i= (iter.i + (iter.index & 1)) << 1;
    j= (iter.j + ((iter.index & 2) >> 1)) << 1;
    k= (iter.k + ((iter.index & 4) >> 2)) << 1;
    tree= iter.tree;
#ifdef never
    fprintf(stderr,"new iter: level= %d, index= %d, ijk= %d %d %d\n",
	    level,index,i,j,k);
#endif
  }
  void* next()
  {
    while (index<7) {
      index++;
#ifdef never
      fprintf(stderr,
	      "next cell: index= %d, ijk= %d %d %d, level= %d\n",
	      index,i,j,k,level);
#endif
      void* result= tree->get_cell(level, i, j, k, index);
      if (result) return(result);
    }
    return( NULL );
  }
  int index;
  int level;
  int i;
  int j;
  int k;
  const baseOctree *tree;
};

template< class T >
class Octcell_kids_iter: private baseOctcell_kids_iter {
public:
  Octcell_kids_iter( const Octree<T>* base )
  : baseOctcell_kids_iter( (const baseOctree*)base ) {}
  Octcell_kids_iter( const Octcell_kids_iter<T>& iter ) // copy
  : baseOctcell_kids_iter( iter ) {}
  Octcell_kids_iter(const Octcell_kids_iter<T>& iter, const int junk) // child
  : baseOctcell_kids_iter( iter, 1 ) {}
  Octcell_kids_iter<T> child_iter() const // Must call next() first!
  { return( Octcell_kids_iter<T>(*this,1) ); }
  T* next( int debug=0 ) 
  { 
    if (debug) 
      fprintf(stderr,"kids_iter::next: level= %d, index= %d, ijk= %d %d %d\n",
	      level, index+1, i, j, k);
    return (T*)baseOctcell_kids_iter::next(); 
  }
  int leaf() const { return( level == tree->get_nlevels() ); }
  int get_level() const { return level; }
};

//
// The ray-octcell intersection algorithm is Spackman and Willis' SMART
// algorithm.  See Computers and Graphics Vol 15, No. 2, pp 185-194, 1991
//
// Definitions needed for baseOctcell ray intersection iterator follow.
// They are undone later on.
//

#define UPDATE(a,b,c) \
linfo->V_smart.a -= linfo->V_compare; linfo->H_smart.b -= linfo->wid_dir.c; \
linfo->H_smart.c += linfo->wid_dir.b;

#define ONE_PLANE_STRUCK(bit,a,b,c) \
if (linfo->relative_child & bit) done= 1; \
else { \
linfo->relative_child |= bit; \
linfo->V_compare= linfo->wid_dir.a; \
linfo->V_smart.a= linfo->wid_dir.a; linfo->V_smart.b= linfo->H_smart.c; \
linfo->V_smart.c= -linfo->H_smart.b; \
linfo->H_smart.b+=linfo->wid_dir.c; linfo->H_smart.c-=linfo->wid_dir.b; \
}

#define X_PLANE_STRUCK ONE_PLANE_STRUCK(1,x,y,z)
#define Y_PLANE_STRUCK ONE_PLANE_STRUCK(2,y,z,x)
#define Z_PLANE_STRUCK ONE_PLANE_STRUCK(4,z,x,y)

#define TWO_PLANE_STRUCK(bits,a,b,c) \
if (linfo->relative_child & bits) done= 1; \
else { \
linfo->relative_child |= bits; \
linfo->V_compare= linfo->wid_dir.c; \
linfo->V_smart.a= linfo->H_smart.b; linfo->V_smart.b=linfo->wid_dir.c; \
linfo->V_smart.c= linfo->wid_dir.c; \
linfo->H_smart.a= linfo->wid_dir.b - linfo->wid_dir.c; \
linfo->H_smart.b-=linfo->wid_dir.a; \
linfo->H_smart.c+=linfo->wid_dir.a; \
}

#define Y_Z_PLANE_STRUCK TWO_PLANE_STRUCK(6,x,y,z)
#define Z_X_PLANE_STRUCK TWO_PLANE_STRUCK(5,y,z,x)
#define X_Y_PLANE_STRUCK TWO_PLANE_STRUCK(3,z,x,y)

#define THREE_PLANE_STRUCK \
if (linfo->relative_child & 7) done= 1;\
else { \
linfo->relative_child |= 7; \
linfo->V_compare= linfo->wid_dir.x; \
linfo->V_smart.x= linfo->wid_dir.x; linfo->V_smart.y= linfo->wid_dir.x; \
linfo->V_smart.z= linfo->wid_dir.x; \
linfo->H_smart.x=linfo->wid_dir.y - linfo->wid_dir.z; \
linfo->H_smart.y= linfo->wid_dir.z - linfo->wid_dir.x; \
linfo->H_smart.z= linfo->wid_dir.x - linfo->wid_dir.y; \
}

#define X_Y_Z_PLANE_STRUCK THREE_PLANE_STRUCK

// This is outside the baseOctcell_intersect_iter class to satisfy
// the DEC Alpha cxx compiler, which can't compile it otherwise.
struct discrete_vector { int x, y, z; };

// Iterates over cells intersected by a ray
class baseOctcell_intersect_iter {
  struct level_info_struct {
    discrete_vector H_smart;
    int base_i;
    int base_j;
    int base_k;
    discrete_vector wid_dir;
    discrete_vector V_smart;
    unsigned int V_compare;
    unsigned char relative_child;
    int filler;  // speeds things up;  don't ask me why
    int filler2; // 64 bytes to here (pad out to 8 longwords)
#ifdef SGI_MIPS
    int filler3; // 68 bytes to here
    int filler4; // 72 bytes to here; SGI seems to like it this long
#endif
  };
public:
  baseOctcell_intersect_iter( const baseOctree *thistree,
			      const gVector& dir, const gPoint& from, 
			      const float startlength, const float endlength );
  baseOctcell_intersect_iter( const baseOctcell_intersect_iter &iter ); //copy
  ~baseOctcell_intersect_iter() { delete [] level_info_stack; }
  void reset( const gVector& dir, const gPoint& from,
	     const float startlength, const float endlength );
  float dist_in_cell() const // call only after next!
  {
    // length_rescale contains one factor of unity_scale in its
    // denominator, but we can\'t include the other factor of
    // unity_scale because that could overflow the resolution of
    // a 32 bit float.  Hence we right-shift the appropriate
    // component of V_smart by 15 bits to more-or-less cancel the
    // other factor.
    float result;
    register level_info_struct* linfo= level_info_stack + level - 1;
    if ( linfo->H_smart.x >= 0 ) {
      if ( linfo->H_smart.z >= 0 ) 
	result= (linfo->V_smart.x * length_rescale.x())/(linfo->V_compare 
							 >> (15-level));
      else result= (linfo->V_smart.y * length_rescale.y())/(linfo->V_compare 
							  >>(15-level));
    }
    else {
      if ( linfo->H_smart.y >= 0 ) 
	result= (linfo->V_smart.z * length_rescale.z())/(linfo->V_compare 
							 >> (15-level));
      else result= (linfo->V_smart.x * length_rescale.x())/(linfo->V_compare 
							  >>(15-level));
    }
    return result;
  }
  float rough_dist_in_cell() const // call only after next!
  {
    float result;
    register level_info_struct* linfo= level_info_stack + level - 1;
    if ( linfo->H_smart.x >= 0 ) {
      if ( linfo->H_smart.z >= 0 ) 
	result= ((linfo->V_smart.x >> 15) * length_rescale.x());
      else result= ((linfo->V_smart.y >> 15) * length_rescale.y());
    }
    else {
      if ( linfo->H_smart.y >= 0 ) 
	result= ((linfo->V_smart.z >> 15) * length_rescale.z());
      else result= ((linfo->V_smart.x >> 15) * length_rescale.x());
    }
    return result;
  }
  void get_indices( int* i_in, int* j_in, int* k_in ) const
  {
    register level_info_struct* linfo= level_info_stack + level - 1;

    register int morton= linfo->relative_child ^ entry_octant;
    *i_in= linfo->base_i + (morton & 1);
    *j_in= linfo->base_j + ((morton & 2) >> 1);
    *k_in= linfo->base_k + ((morton & 4) >> 2);
  }
  gVector& get_cell_entry_scaled();
  gVector& get_cell_exit_scaled();
protected:
  void push() // Go deeper into octree; call only after next!
  {
#ifdef never
    if (level<tree->get_nlevels()) {
#endif
      register level_info_struct* linfo= level_info_stack + level;
      level++;

      linfo->base_i= ((linfo-1)->base_i
	       + (((linfo-1)->relative_child ^ entry_octant) & 1)) << 1;
      linfo->base_j= ((linfo-1)->base_j
	       + ((((linfo-1)->relative_child ^ entry_octant) & 2) >> 1)) << 1;
      linfo->base_k= ((linfo-1)->base_k
	       + ((((linfo-1)->relative_child ^ entry_octant) & 4) >> 2)) << 1;
      linfo->relative_child= 0;

      linfo->V_compare= (linfo-1)->V_compare >> 1;
      linfo->wid_dir.x= (linfo-1)->wid_dir.x >> 1;
      linfo->wid_dir.y= (linfo-1)->wid_dir.y >> 1;
      linfo->wid_dir.z= (linfo-1)->wid_dir.z >> 1;
      linfo->V_smart= (linfo-1)->V_smart;
      linfo->H_smart= (linfo-1)->H_smart;

      if (linfo->V_smart.x > linfo->V_compare) { UPDATE(x,y,z); }
      else linfo->relative_child |= 1;
      if (linfo->V_smart.y > linfo->V_compare) { UPDATE(y,z,x); }
      else linfo->relative_child |= 2;
      if (linfo->V_smart.z > linfo->V_compare) { UPDATE(z,x,y); }
      else linfo->relative_child |= 4;

      first_pass= 1;
#ifdef never
    }
    else {
      fprintf(stderr,"Octree push beyond stack depth!\n");
      exit(-1);
    }
#endif
  }
  void pop() // Go to shallower level of octree
  {
    level--;
  }
  void* next()
  {
    register level_info_struct* linfo= level_info_stack + level - 1;
    int done;
    do {
      done= 0;
      if (first_pass) first_pass= 0;
      else { // move into next cell
	if ( linfo->H_smart.x < 0 ) {
	  if ( linfo->H_smart.y<0 ) { X_PLANE_STRUCK }
	  else if ( linfo->H_smart.y>0 ) { Z_PLANE_STRUCK } 
	  else { Z_X_PLANE_STRUCK }
	}
	else if ( linfo->H_smart.x>0 ) {
	  if ( linfo->H_smart.z < 0 ) { Y_PLANE_STRUCK }
	  else if ( linfo->H_smart.z>0 ) { X_PLANE_STRUCK }
	  else { X_Y_PLANE_STRUCK }
	}
	else {
	  if ( linfo->H_smart.y<0 ) { X_PLANE_STRUCK } 
	  else if ( linfo->H_smart.y>0 ) { Y_Z_PLANE_STRUCK }
	  else { X_Y_Z_PLANE_STRUCK }
	}
      }
      
      if (!done) {
        void* result= tree->get_cell(level,
				     linfo->base_i, linfo->base_j, 
				     linfo->base_k,
				     (linfo->relative_child ^ entry_octant));
	if (result) return result;
      }
      else {
	linfo--;
	pop();
      }
    }
    while (level>0);
    return NULL;
  }
  void* next( const int debug )
  {
    register level_info_struct* linfo= level_info_stack + level - 1;
    int done;
    do {
      done= 0;
      if (first_pass) first_pass= 0;
      else { // move into next cell
	if ( linfo->H_smart.x < 0 ) {
	  if ( linfo->H_smart.y<0 ) { X_PLANE_STRUCK }
	  else if ( linfo->H_smart.y>0 ) { Z_PLANE_STRUCK } 
	  else { Z_X_PLANE_STRUCK }
	}
	else if ( linfo->H_smart.x>0 ) {
	  if ( linfo->H_smart.z < 0 ) { Y_PLANE_STRUCK }
	  else if ( linfo->H_smart.z>0 ) { X_PLANE_STRUCK }
	  else { X_Y_PLANE_STRUCK }
	}
	else {
	  if ( linfo->H_smart.y<0 ) { X_PLANE_STRUCK } 
	  else if ( linfo->H_smart.y>0 ) { Y_Z_PLANE_STRUCK }
	  else { X_Y_Z_PLANE_STRUCK }
	}
      }
      
      void* result= tree->get_cell(level,
				   linfo->base_i, linfo->base_j, 
				   linfo->base_k,
				   (linfo->relative_child 
				    ^ entry_octant));
      if (debug) {
	fprintf(stderr,
		"isect_iter step: level= %d, rel_child= %d, next= %d\n",
		level, linfo->relative_child, (long)result);
	fprintf(stderr,"                 base= %d %d %d\n",
		linfo->base_i, linfo->base_j, linfo->base_k);
      }
      if (!done) {
        void* result= tree->get_cell(level,
				     linfo->base_i, linfo->base_j, 
				     linfo->base_k,
				     (linfo->relative_child ^ entry_octant));
	if (result) return result;
      }
      else {
	if (debug) fprintf(stderr,"pop!\n");
	linfo--;
	pop();
      }
    }
    while (level>0);
    if (debug) fprintf(stderr,"Done with ray!\n");
    return NULL;
  }
  int trees_match(const baseOctree* tree_in) const
    { return (*tree_in == *tree); }
  const baseOctree *tree;
  level_info_struct* level_info_stack;
  int level;
  unsigned char entry_octant;
  int first_pass;      // true until first call to next
  gVector length_rescale;        // used to calculate distance in cell
};

template< class T >
class Octcell_intersect_iter: private baseOctcell_intersect_iter {
public:
  Octcell_intersect_iter( const Octree<T> *base,
			  const gVector& dir, const gPoint& from,
			  const float minlength, const float maxlength )
  : baseOctcell_intersect_iter( (baseOctree*)base, dir, from, 
				minlength, maxlength ) {}
  Octcell_intersect_iter( const Octcell_intersect_iter<T> &iter ) // copy
  : baseOctcell_intersect_iter( iter ) {}
  void reset( const gVector& dir, const gPoint& from, 
	     const float minlength, const float maxlength )
  { baseOctcell_intersect_iter::reset( dir, from, minlength, maxlength ); }
  void push() { baseOctcell_intersect_iter::push(); } // deeper into octree
  T* next() 
  { return (T*)baseOctcell_intersect_iter::next(); }
  T* next( const int debug ) 
  { return (T*)baseOctcell_intersect_iter::next(debug); }
  int leaf() const { return( level == tree->get_nlevels() ); }
  float dist_in_cell() const 
  { return(baseOctcell_intersect_iter::dist_in_cell()); }
  float rough_dist_in_cell() const 
  { return(baseOctcell_intersect_iter::rough_dist_in_cell()); }
  int get_level() const { return level; }
  void get_indices( int* x_in, int* y_in, int* z_in ) const
  { baseOctcell_intersect_iter::get_indices( x_in, y_in, z_in ); }
  gVector& get_cell_entry_scaled()
  { return baseOctcell_intersect_iter::get_cell_entry_scaled(); }
  gVector& get_cell_exit_scaled()
  { return baseOctcell_intersect_iter::get_cell_exit_scaled(); }
  int trees_match( const Octree<T>* tree_in ) 
  { return( baseOctcell_intersect_iter::trees_match(tree_in) ); }
};

#undef UPDATE
#undef ONE_PLANE_STRUCK
#undef X_PLANE_STRUCK
#undef Y_PLANE_STRUCK
#undef Z_PLANE_STRUCK
#undef TWO_PLANE_STRUCK
#undef X_Y_PLANE_STRUCK
#undef Y_Z_PLANE_STRUCK
#undef Z_X_PLANE_STRUCK
#undef THREE_PLANE_STRUCK



