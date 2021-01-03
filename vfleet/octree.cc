/****************************************************************************
 * octree.cc
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
#include <string.h> // For memcmp needed by geometry.h in octree.h
#include "octree.h"

//
// The ray-octcell intersection algorithm is Spackman and Willis' SMART
// algorithm.  See Computers and Graphics Vol 15, No. 2, pp 185-194, 1991
//

baseOctree::baseOctree(const gBoundBox& bounds_in, 
		       int xdim_in, int ydim_in, int zdim_in, 
		       int cellsize_in, 
		       void*(*allocate_class_array)( int length ),
		       void (*delete_class_array)( void * ))
: bounds( bounds_in ), extended_bounds( bounds_in )
{
  int n;

  xdim= xdim_in;
  ydim= ydim_in;
  zdim= zdim_in;
  cellsize= cellsize_in;

#ifdef never
  fprintf(stderr,"cellsize= %d\n",cellsize);
#endif

  // Save the delete method for use by the destructor.
  delete_method= delete_class_array;

  // Figure out the tree depth
  int count, maxdim;
  maxdim= xdim;
  if (ydim>maxdim) maxdim= ydim;
  if (zdim>maxdim) maxdim= zdim;
  count= 1;
  nlevels= 0;
  while (count<maxdim) {
    nlevels++;
    count *= 2;
  }

  if ((ydim == zdim) && (count == ydim) 
      && (cellsize == OCTREE_PERFECT_CELL_SIZE)) perfect= 1;
  else perfect= 0;

  // Rescale the extended bounding box to encompass the extended octree
  float ext_xmax= bounds.xmin() 
       + (count*(bounds.xmax()-bounds.xmin()))/xdim;
  float ext_ymax= bounds.ymin() 
       + (count*(bounds.ymax()-bounds.ymin()))/ydim;
  float ext_zmax= bounds.zmin() 
       + (count*(bounds.zmax()-bounds.zmin()))/zdim;
  extended_bounds= gBoundBox( bounds.xmin(), bounds.ymin(), bounds.zmin(), 
			      ext_xmax, ext_ymax, ext_zmax );


  // Build some scaling information which saves calculating time
  // in the intersection iterator
  scale= gVector( extended_bounds.xmax() - extended_bounds.xmin(),
		 extended_bounds.ymax() - extended_bounds.ymin(),
		 extended_bounds.zmax() - extended_bounds.zmin() );
  inv_scale= gVector( 1.0/scale.x(), 1.0/scale.y(), 1.0/scale.z() );

  // Build array reference information
  width_x= new int[nlevels+1];
  width_y= new int[nlevels+1];
  width_z= new int[nlevels+1];
  width_x[nlevels]= xdim;
  width_y[nlevels]= ydim;
  width_z[nlevels]= zdim;
  for (n=nlevels-1; n>0; n--) {
    width_x[n]= (width_x[n+1] + 1)/2;
    width_y[n]= (width_y[n+1] + 1)/2;
    width_z[n]= (width_z[n+1] + 1)/2;
#ifdef never
    fprintf(stderr,"widths %d: %d %d %d\n",
	    n,width_x[n],width_y[n],width_z[n]);
#endif
  }
  width_x[0]= width_y[0]= width_z[0]= 1;

  // Allocate space
  start_points= new void**[nlevels+1];
  for (n=0; n<=nlevels; n++) {
    int plane_size= width_y[n] * width_z[n];
    start_points[n]= new void*[width_x[n]];
    for (int i=0; i<width_x[n]; i++) {
      start_points[n][i]= (*allocate_class_array)(plane_size);
#ifdef never
      fprintf(stderr,"start point %d, plane %d: %d=%x; size %d\n",
	      n,i,(int)start_points[n][i], (int)start_points[n][i],
	      plane_size);
#endif
    }
  }

  // top_cell is just the top allocated cell.
  top_cell= start_points[0][0];

  // Nothing to clean up
}

baseOctree::~baseOctree() 
{ 
  for (int level=0; level<nlevels+1; level++) {
    for (int i=0; i<width_x[level]; i++) 
      (*delete_method)( start_points[level][i] );
    delete [] start_points[level];
  }
  delete [] start_points;

  delete [] width_x;
  delete [] width_y;
  delete [] width_z;
}

void baseOctree::initialize_leaves( void (*init_method)(void *cells,
							int i, int j, int kmin, int kmax,
							void* init),
				    void *init )
{
	int kmax = width_z[nlevels] - 1;
	void *cells;

	for (int i = 0; i < width_x[nlevels]; i++) {
		for (int j = 0; j < width_y[nlevels]; j++) {
				cells = get_cell (nlevels, i, j, 0);

			(*init_method) (cells, i, j, 0, kmax, init);
		}
	}
}

#define UPDATE(a,b,c) \
linfo->V_smart.a -= linfo->V_compare; linfo->H_smart.b -= linfo->wid_dir.c; \
linfo->H_smart.c += linfo->wid_dir.b;

baseOctcell_intersect_iter::baseOctcell_intersect_iter( 
			      const baseOctree *thistree, 
			      const gVector& dir, const gPoint& from, 
			      const float startlength, const float endlength )
{
  tree= thistree;
  level_info_stack= new level_info_struct[ tree->get_nlevels() ];
  gVector lcldir= dir;
  gPoint lclfrom= from;
  reset( lcldir, lclfrom, startlength, endlength );
}

void baseOctcell_intersect_iter::reset( const gVector& dir, const gPoint& from,
				       const float minlength, 
				       const float maxlength )
{
  first_pass= 1;
  level= 1;

  level_info_struct* linfo= level_info_stack;

  linfo->base_i= linfo->base_j= linfo->base_k= 0;

  if ((dir.x()==0.0) && (dir.y()==0.0) && (dir.z()==0.0)) {
    fprintf(stderr,"baseOctcell_intersect_iter: direction vector is zero!\n");
    exit(-1);
  }

  // entry_octant is the octant the ray comes from.
  entry_octant= 
    ( (dir.x() < 0) ? 1 : 0 )
    | ( (dir.y() < 0) ? 2 : 0 )
    | ( (dir.z() < 0) ? 4 : 0 );
  int exit_octant= entry_octant ^ 7;

  // Find the point of intersection of the ray with the octree top level
  // cell.  The first check determines if the ray starts in the cell;
  // the second check extends the ray to the cell boundary.
  float maxdist= maxlength;
  gPoint isect= from + (dir * minlength);
  if ( !(tree->extended_bounds.inside( isect )) ) {
    if (!(tree->extended_bounds.intersect( dir, from, minlength, 
					       &maxdist))) {
      fprintf(stderr,
	      "baseOctcell_intersect_iter: ray does not intersect octree!\n");
      exit(-1);
    }
    isect= from + (dir * maxdist);
  }

  // Traversal is done in integer arithmetic.  Everything is scaled
  // to this variable.
  int unity_scale= (1<<15) - 1;

  // Generate the discrete distance and direction vectors
  discrete_vector dist_disc;
  dist_disc.x= (int)(unity_scale * tree->inv_scale.x()
		     * (isect.x() - tree->extended_bounds.xmin()));
  dist_disc.y= (int)(unity_scale * tree->inv_scale.y()
		     * (isect.y() - tree->extended_bounds.ymin()));
  dist_disc.z= (int)(unity_scale * tree->inv_scale.z()
		     * (isect.z() - tree->extended_bounds.zmin()));

  if (exit_octant & 1) dist_disc.x= unity_scale - dist_disc.x;
  if (exit_octant & 2) dist_disc.y= unity_scale - dist_disc.y;
  if (exit_octant & 4) dist_disc.z= unity_scale - dist_disc.z;

  gVector dir_scale( dir.x()*tree->inv_scale.x(), 
		    dir.y()*tree->inv_scale.y(), 
		    dir.z()*tree->inv_scale.z() );
  gVector dir_scale_pos( dir_scale.x() >= 0.0 ? dir_scale.x() : -dir_scale.x(),
		   dir_scale.y() >= 0.0 ? dir_scale.y() : -dir_scale.y(),
		   dir_scale.z() >= 0.0 ? dir_scale.z() : -dir_scale.z() );

  discrete_vector dir_disc;
  if ( dir_scale_pos.x() > dir_scale_pos.y() ) {
    if ( dir_scale_pos.x() > dir_scale_pos.z() ) {
      float rescale= 1.0/dir_scale_pos.x();
      dir_disc.x= unity_scale;
      dir_disc.y= (int)(unity_scale*( dir_scale_pos.y()*rescale ));
      dir_disc.z= (int)(unity_scale*( dir_scale_pos.z()*rescale ));
    }
    else {
      float rescale= 1.0/dir_scale_pos.z();
      dir_disc.x= (int)(unity_scale*( dir_scale_pos.x()*rescale ));
      dir_disc.y= (int)(unity_scale*( dir_scale_pos.y()*rescale ));
      dir_disc.z= unity_scale;
    }
  }
  else {
    if ( dir_scale_pos.y() > dir_scale_pos.z() ) {
      float rescale= 1.0/dir_scale_pos.y();
      dir_disc.x= (int)(unity_scale*( dir_scale_pos.x()*rescale ));
      dir_disc.y= unity_scale;
      dir_disc.z= (int)(unity_scale*( dir_scale_pos.z()*rescale ));
    }
    else {
      float rescale= 1.0/dir_scale_pos.z();
      dir_disc.x= (int)(unity_scale*( dir_scale_pos.x()*rescale ));
      dir_disc.y= (int)(unity_scale*( dir_scale_pos.y()*rescale ));
      dir_disc.z= unity_scale;
    }
  }

  // V_smart is the scaled distance vector
  linfo->V_smart.x= unity_scale*dist_disc.x;
  linfo->V_smart.y= unity_scale*dist_disc.y;
  linfo->V_smart.z= unity_scale*dist_disc.z;

  // H_smart is direction vector cross distance vector, both scaled.
  linfo->H_smart.x= dir_disc.y * dist_disc.z - dir_disc.z * dist_disc.y;
  linfo->H_smart.y= dir_disc.z * dist_disc.x - dir_disc.x * dist_disc.z;
  linfo->H_smart.z= dir_disc.x * dist_disc.y - dir_disc.y * dist_disc.x;

  // wid_dir is ray direction scaled by child width
  linfo->wid_dir.x= unity_scale * dir_disc.x >> 1;
  linfo->wid_dir.y= unity_scale * dir_disc.y >> 1;
  linfo->wid_dir.z= unity_scale * dir_disc.z >> 1;

  // V_compare is scaled child width
  linfo->V_compare= (unity_scale * unity_scale) >> 1;

#ifdef never
fprintf(stderr,"dir= %f %f %f, scale= %f %f %f\n",
	dir.x(),dir.y(),dir.z(),scale.x(),scale.y(),scale.z());
fprintf(stderr,"dir_disc= %d %d %d  dist_disc= %d %d %d\n",
	dir_disc.x,dir_disc.y,dir_disc.z,dist_disc.x,dist_disc.y,dist_disc.z);
fprintf(stderr,"H_smart= %d %d %d, V_smart= %d %d %d\n",
	linfo->H_smart.x,linfo->H_smart.y,linfo->H_smart.z,linfo->V_smart.x,linfo->V_smart.y,linfo->V_smart.z);
fprintf(stderr,"wid_dir= %d %d %d, relative_child= %d\n",
	linfo->wid_dir.x,linfo->wid_dir.y,linfo->wid_dir.z);
#endif

  // Set up relative_child, which gives relative octant of entry
  linfo->relative_child= 0;
  if (linfo->V_smart.x > linfo->V_compare) { UPDATE(x,y,z); }
  else linfo->relative_child |= 1;
  if (linfo->V_smart.y > linfo->V_compare) { UPDATE(y,z,x); } 
  else linfo->relative_child |= 2;
  if (linfo->V_smart.z > linfo->V_compare) { UPDATE(z,x,y); }
  else linfo->relative_child |= 4;

#ifdef never
fprintf(stderr,"H_smart= %d %d %d, V_smart= %d %d %d rel_child= %d\n\n",
	linfo->H_smart.x,linfo->H_smart.y,linfo->H_smart.z,linfo->V_smart.x,linfo->V_smart.y,linfo->V_smart.z,
	linfo->relative_child);
#endif

  // length_rescale is used to go from V_smart component values to
  // distances traversed in the cell.
  float dirlng= dir_scale_pos.length();
  length_rescale= gVector( (dir_scale_pos.x() == 0.0) ? 0.0 
			   : tree->scale.x()*dirlng/(dir_scale_pos.x()
						     *unity_scale),
			   (dir_scale_pos.y() == 0.0) ? 0.0 
			   : tree->scale.y()*dirlng/(dir_scale_pos.y()
						     *unity_scale),
			   (dir_scale_pos.z() == 0.0) ? 0.0 
			   : tree->scale.z()*dirlng/(dir_scale_pos.z()
						     *unity_scale));
}
#undef UPDATE

baseOctcell_intersect_iter::baseOctcell_intersect_iter( 
                                  const baseOctcell_intersect_iter &iter )
// Copy constructor
{
  tree= iter.tree;
  level= iter.level;
  entry_octant= iter.entry_octant;

  level_info_stack= new level_info_struct[ tree->get_nlevels() ];
  for (int i=0; i<level; i++)
    level_info_stack[i]= iter.level_info_stack[i];

  first_pass= iter.first_pass;
  length_rescale= iter.length_rescale;
}

gVector& baseOctcell_intersect_iter::get_cell_entry_scaled()
{
  static gVector result;

  level_info_struct* linfo= level_info_stack + level - 1;
  int exit_octant= entry_octant ^ 7;

  discrete_vector dist_disc;
  dist_disc.x= linfo->V_smart.x;
  dist_disc.y= linfo->V_smart.y;
  dist_disc.z= linfo->V_smart.z;

  if (exit_octant & 1) dist_disc.x= linfo->V_compare - dist_disc.x;
  if (exit_octant & 2) dist_disc.y= linfo->V_compare - dist_disc.y;
  if (exit_octant & 4) dist_disc.z= linfo->V_compare - dist_disc.z;

  dist_disc.x= dist_disc.x >> 15;
  dist_disc.y= dist_disc.y >> 15;
  dist_disc.z= dist_disc.z >> 15;

  unsigned int dist_tmp= linfo->V_compare >> 15;
  float inv_scale;
  if (!dist_tmp) inv_scale= 1.0;
  else inv_scale= 1.0/(dist_tmp);
  result= gVector(-0.5 + (inv_scale * dist_disc.x),
		  -0.5 + (inv_scale * dist_disc.y),
		  -0.5 + (inv_scale * dist_disc.z) );
  return result;
}

gVector& baseOctcell_intersect_iter::get_cell_exit_scaled()
{
  static gVector result;

  level_info_struct* linfo= level_info_stack + level - 1;
  int exit_octant= entry_octant ^ 7;

  discrete_vector dist_disc;
  int next_v_compare;

  if ( linfo->H_smart.x < 0 ) {
    if ( linfo->H_smart.y<0 ) { 
      // X_PLANE_STRUCK
      dist_disc.x= 0;
      dist_disc.y= linfo->H_smart.z;
      dist_disc.z= -linfo->H_smart.y;
      next_v_compare= linfo->wid_dir.x;
      }
    else if ( linfo->H_smart.y>0 ) { 
      // Z_PLANE_STRUCK
      dist_disc.z= 0;
      dist_disc.x= linfo->H_smart.y + 1;
      dist_disc.y= -linfo->H_smart.x + 1;
      next_v_compare= linfo->wid_dir.z;
    } 
    else { 
      // Z_X_PLANE_STRUCK
      dist_disc.z= 0;
      dist_disc.x= 0;
      dist_disc.y= linfo->H_smart.z + 1;
      next_v_compare= linfo->wid_dir.x;
    }
  }
  else if ( linfo->H_smart.x>0 ) {
    if ( linfo->H_smart.z < 0 ) { 
      // Y_PLANE_STRUCK 
      dist_disc.y= 0;
      dist_disc.z= linfo->H_smart.x;
      dist_disc.x= -linfo->H_smart.z;
      next_v_compare= linfo->wid_dir.y;
    }
    else if ( linfo->H_smart.z>0 ) { 
      // X_PLANE_STRUCK 
      dist_disc.x= 0;
      dist_disc.y= linfo->H_smart.z;
      dist_disc.z= -linfo->H_smart.y;
      next_v_compare= linfo->wid_dir.x;
    }
    else { 
      // X_Y_PLANE_STRUCK 
      dist_disc.x= 0;
      dist_disc.y= 0;
      dist_disc.z= linfo->H_smart.x;
      next_v_compare= linfo->wid_dir.y;
    }
  }
  else {
    if ( linfo->H_smart.y<0 ) { 
      // X_PLANE_STRUCK 
      dist_disc.x= 0;
      dist_disc.y= linfo->H_smart.z;
      dist_disc.z= -linfo->H_smart.y;
      next_v_compare= linfo->wid_dir.x;
    } 
    else if ( linfo->H_smart.y>0 ) { 
      // Y_Z_PLANE_STRUCK 
      dist_disc.y= 0;
      dist_disc.z= 0;
      dist_disc.x= linfo->H_smart.y;
      next_v_compare= linfo->wid_dir.z;
    }
    else { // X_Y_Z_PLANE_STRUCK 
      dist_disc.x= 0;
      dist_disc.y= 0;
      dist_disc.z= 0;
      next_v_compare= linfo->wid_dir.x;
    }
  }

  if (exit_octant & 1) dist_disc.x= next_v_compare - dist_disc.x;
  if (exit_octant & 2) dist_disc.y= next_v_compare - dist_disc.y;
  if (exit_octant & 4) dist_disc.z= next_v_compare - dist_disc.z;

  dist_disc.x= dist_disc.x >> 15;
  dist_disc.y= dist_disc.y >> 15;
  dist_disc.z= dist_disc.z >> 15;

  unsigned int dist_tmp= next_v_compare >> 15;
  float inv_scale;
  if (!dist_tmp) inv_scale= 1.0;
  else inv_scale= 1.0/(dist_tmp);
  result= gVector(-0.5 + (inv_scale * dist_disc.x),
		  -0.5 + (inv_scale * dist_disc.y),
		  -0.5 + (inv_scale * dist_disc.z) );

  return result;
}
