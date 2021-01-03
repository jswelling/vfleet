/* This example uses the pg_isosurface function to produce an iso-valued
 * surface from 3D gridded data.  A bounding box shows the bounds of the
 * computational region.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "octree.h"

#ifndef MAKING_DEPEND
/* We want to protect these from makedepend.  Not a problem, since
 * this executable isn't actually built in the distribution.
 */
#include <p3dgen.h>
#include <drawp3d.h>
#endif

/* Used to test initialization value passing */
static char *dummy_init= "hello world";

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Camera information */
static P_Point lookfrom= { 0.5, 0.5, 3.0 };
static P_Point lookat= { 0.5, 0.5, 0.5 };
static P_Vector up= { 0.0, 1.0, 0.0 };
static float fovea= 45.0;
static float hither= -1.0;
static float yon= -10.0;

/* Light source information */
static P_Point light_loc= {0.0, 1.0, 10.0};
static P_Color light_color= { P3D_RGB, 0.8, 0.8, 0.8, 0.3 };
static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 0.8 };
static P_Color blue= { P3D_RGB, 0.0, 0.0, 1.0 };

/* Dimensions of the grid from which the isosurface is to be extracted */
#define ISO_NX 48
#define ISO_NY 48
#define ISO_NZ 64

float data[ISO_NX][ISO_NY][ISO_NZ];

/* Ray path coordinate vector length */
#define RAY_PATH_PTS 200

struct octreeSampleCell {
  int i, j, k;
  octreeSampleCell() {};
  ~octreeSampleCell();
};

octreeSampleCell::~octreeSampleCell()
{
#ifdef never
  fprintf(stderr,"Deleting cell at %d; ijk= %d %d %d\n",
	  (int)this,i,j,k);
#endif
}

void leaf_initialize( void* cell_in,
		      int i_in, int j_in, int k_in_min, int k_in_max, 
		      void *init )
{
  octreeSampleCell* cell= (octreeSampleCell*)cell_in;
  cell->i= i_in;
  cell->j= j_in;
  for (int k=k_in_min; k<=k_in_max; k++) {
    cell->k= k;
    if ((char *)init != dummy_init) 
      fprintf(stderr,"Initialization value mismatch at %d %d %d!\n",
	      cell->i,cell->j,cell->k);
    if (cell->i>=ISO_NX || cell->j>=ISO_NY || cell->k>=ISO_NZ) 
      fprintf(stderr,"Initialization failure; %d %d %d vs %d %d %d\n",
	      cell->i,cell->j,cell->k,ISO_NX,ISO_NY,ISO_NZ);
#ifdef never
    fprintf(stderr,"Initializing cell at %d; %d %d %d\n",(int)cell,
	    cell->i,cell->j,cell->k);
#endif
  }
}

static void fill_with_zeros()
/* This function provides the values of which an iso-valued surface is
 * found.
 */
{
  int i, j, k;

  for (i=0; i<ISO_NX; i++) {
    for (j=0; j<ISO_NY; j++) {
      for (k=0; k<ISO_NZ; k++) {
	data[i][j][k]= 0.0;
      }
    }
  }
}

void kids_walk( Octcell_kids_iter<octreeSampleCell>& iter, int level )
{
#ifdef never
  fprintf(stderr,"Kids walk level %d\n",level);
#endif
  octreeSampleCell *next_cell;
  while (next_cell= iter.next()) {
    if (iter.leaf()) {
#ifdef never
      fprintf(stderr,"walked to leaf %d %d %d; address %d\n", next_cell->i, 
	      next_cell->j, next_cell->k,(int)next_cell);
#endif
      data[next_cell->i][next_cell->j][next_cell->k]= 1.0;
    }
    else {
      fprintf(stderr,"not leaf;  dividing.\n");
      Octcell_kids_iter<octreeSampleCell> new_iter= iter.child_iter();
      kids_walk( new_iter, level+1 );
    }
  }
}

void intersect_walk( Octcell_intersect_iter<octreeSampleCell>& iter, 
		    int level, float *dist, 
		    const gVector& dir, const gPoint& fm)
{
#ifdef never
  fprintf(stderr,"Intersect walk level %d\n",level);
#endif
  octreeSampleCell *next_cell;
  while (next_cell= iter.next()) {
    fprintf(stderr,"Hit level %d\n",iter.get_level());
    if (iter.leaf()) {
      fprintf(stderr,"leaf %d %d %d at %d\n", next_cell->i, 
	      next_cell->j, next_cell->k,(long)next_cell);
      data[next_cell->i][next_cell->j][next_cell->k]= 1.0;

      float dx= 1.0/ISO_NX;
      float dy= 1.0/ISO_NY;
      float dz= 1.0/ISO_NZ;
      gBoundBox bbox(next_cell->i * dx, 
		     next_cell->j * dy,
		     next_cell->k * dz,
		     (next_cell->i + 1) * dx,
		     (next_cell->j + 1) * dy,
		     (next_cell->k + 1) * dz);
      if (bbox.inside(fm)) {
	fprintf(stderr,"Origin in cell: scaled: %f %f %f\n",
		(ISO_NX * (fm.x() - bbox.xmin())) - 0.5,
		(ISO_NY * (fm.y() - bbox.ymin())) - 0.5,
		(ISO_NZ * (fm.z() - bbox.zmin())) - 0.5);
      }
      else {
	float isect_dist= 10.0;
	if (bbox.intersect( dir, fm, 0.0, &isect_dist )) {
	  gPoint isect_pt= fm + (dir * isect_dist);
	  fprintf(stderr,"Correct scaled entry pt: %f %f %f\n",
		  (ISO_NX * (isect_pt.x() - bbox.xmin())) - 0.5,
		  (ISO_NY * (isect_pt.y() - bbox.ymin())) - 0.5,
		  (ISO_NZ * (isect_pt.z() - bbox.zmin())) - 0.5);
	}
	else fprintf(stderr,"Ray should not hit this cell!\n");
      }
      gVector sc_entry= iter.get_cell_entry_scaled();
      gVector sc_exit= iter.get_cell_exit_scaled();
      fprintf(stderr,"Scaled entry: %f %f %f\n",
	      sc_entry.x(),sc_entry.y(),sc_entry.z());
      fprintf(stderr,"Scaled exit: %f %f %f\n",
	      sc_exit.x(), sc_exit.y(), sc_exit.z());

      *dist += iter.dist_in_cell();
    }
    else {
      iter.push();
    }
  }
}

static void shoot_ray( const gPoint& lookfm, const gPoint& lookat )
/* This function provides the values of which an iso-valued surface is
 * found.
 */
{
  gBoundBox bbox(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  Octree<octreeSampleCell> *tree= 
    new Octree<octreeSampleCell>(bbox,ISO_NX,ISO_NY,ISO_NZ, 
				 leaf_initialize, dummy_init);

  Octcell_kids_iter<octreeSampleCell> kids_iter(tree);

#ifdef never
  (void)kids_iter.next();
  (void)kids_iter.next();
  (void)kids_iter.next(); // This is the one
  Octcell_kids_iter<octreeSampleCell> kid1= kids_iter.child_iter();
//  (void)kid1.next();
//  Octcell_kids_iter<octreeSampleCell> kid2= kid1.child_iter();
//  (void)kids_iter.next();
  kids_walk( kid1, 0 );
#endif

  gVector lookdir= lookat - lookfm;
  lookdir.normalize();

  float distance_walked= 10.0; // reset by intersect
  (void)bbox.intersect( lookdir, lookfm, 0.0, &distance_walked );
  float other_side= 10.0; // reset by intersect
  (void)bbox.intersect( lookdir, lookfm, distance_walked+0.001, &other_side );
  fprintf(stderr,"Initial range: %f to %f\n",distance_walked,other_side);

  Octcell_intersect_iter<octreeSampleCell>
    isect_iter(tree, lookdir, lookfm, 0.0, 10.0);
  intersect_walk( isect_iter, 0, &distance_walked, lookdir, lookfm );

  fprintf(stderr,"Traversal distance: %f vs. accurate %f\n\n",
	  distance_walked, other_side);

  fprintf(stderr,"deleting.\n");
  delete tree;
}

static void add_ray( P_Point& rayfrom, P_Point& rayto )
{
  float linecoords[RAY_PATH_PTS][3];

  gPoint from( rayfrom.x, rayfrom.y, rayfrom.z );
  gPoint to( rayto.x, rayto.y, rayto.z );

  gVector dir= to - from;
  gVector step= dir * (1.0/((float)(RAY_PATH_PTS-1)));
  gPoint loc;

  for (int i=0; i<RAY_PATH_PTS; i++) {
    loc= from + (step*((float)i));
    linecoords[i][0]= loc.x();
    linecoords[i][1]= loc.y();
    linecoords[i][2]= loc.z();
  }
  ERRCHK( pg_open("") );
  ERRCHK( pg_gobcolor( &blue ) );
  ERRCHK( pg_polyline( po_create_cvlist( P3D_CVTX, RAY_PATH_PTS, 
					 (float *)linecoords )));
  ERRCHK( pg_close() );
}

static void create_isosurf(P_Point& rayfrom, P_Point& rayto)
{
  P_Point corner1, corner2;
  float val= 0.1;

  /* Set the corners of the region in which to draw the isosurface 
   * and bounding box.
   */
  corner1.x= 0.0;
  corner1.y= 0.0;
  corner1.z= 0.0;
  corner2.x= 1.0;
  corner2.y= 1.0;
  corner2.z= 1.0;

  /* Calculate the data to contour, and the values with which to color it */
  fill_with_zeros();
  gPoint from( rayfrom.x, rayfrom.y, rayfrom.z );
  gPoint at( rayto.x, rayto.y, rayto.z );
//    fprintf(stderr,"Traversal pass: %d: %f %f\n",i, from.z(), at.z());
  shoot_ray(from, at);

  /* Open a GOB into which to put the isosurface and bounding box */
  ERRCHK( pg_open("mygob") );

  /* This actually generates the isosurface GOB.  See the documentation
   * for an explaination of the parameters.
   */
  ERRCHK( pg_isosurface( P3D_CVTX, (float *)data, (float *)0, 
			ISO_NX, ISO_NY, ISO_NZ, val, 
			&corner1, &corner2, 0,0 ) );

  /* This adds a bounding box, to show the boundaries of the grid of
   * data from which the isosurface is extracted.
   */
  ERRCHK( pg_boundbox( &corner1, &corner2 ) );

  /* Add a ray along the ray path */
  add_ray( rayfrom, rayto );

  /* Close the GOB which holds the new objects. */
  ERRCHK( pg_close() );
}

main( int argc, char *argv[] )
{
  P_Point rayfrom; 
  P_Point rayto;

  /* Get parameters */
  if (argc != 7) {
    fprintf(stderr,"Needs six float args!\n");
    exit(-1);
  }
  rayfrom.x= atof( argv[1] );
  rayfrom.y= atof( argv[2] );
  rayfrom.z= atof( argv[3] );
  rayto.x= atof( argv[4] );
  rayto.y= atof( argv[5] );
  rayto.z= atof( argv[6] );

  /* Initialize the renderer */
  ERRCHK( pg_init_ren("myrenderer","vrml","octree_tester.vrml",
		      "I don't care about this string") );

  /* Generate a light source and camera */
  ERRCHK( pg_open("mylights") );
  ERRCHK( pg_light(&light_loc, &light_color) );
  ERRCHK( pg_ambient(&ambient_color) );
  ERRCHK( pg_close() );
  ERRCHK( pg_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* This function actually generates the isosurface GOB, and a bounding
   * box to show the limits of the grid from which the surface is extracted.
   */
  create_isosurf(rayfrom, rayto);

  /* Cause the isosurface and bounding box to be rendered */
  ERRCHK( pg_snap("mygob","mylights","mycamera") );

  /* Close DrawP3D */
  ERRCHK( pg_shutdown() );
}

