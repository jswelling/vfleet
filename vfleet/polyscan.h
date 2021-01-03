/* This functionality is derived from Paul Heckbert's polygon scan conversion
 * routines, "Generic Convex Polygon Scan Conversion and Clipping", in
 * Graphics Gems (the first book of the series).
 */

/* poly.h: definitions for polygon package */

#ifndef POLY_H_INCLUDED
#define POLY_H_INCLUDED

#include "geometry.h"

#define POLY_NMAX 10		/* max #sides to a polygon; change if needed */
/* note that Poly::clip, given an n-gon as input, might output an (n+6)gon */
/* POLY_NMAX=10 is thus appropriate if input polygons are triangles or quads */

class Poly_box {	/* A BOX (TYPICALLY IN SCREEN SPACE) */
public:
  double x0, x1;		/* left and right */
  double y0, y1;		/* top and bottom */
  double z0, z1;		/* near and far */
};

class Poly_window {	/* WINDOW: A DISCRETE 2-D RECTANGLE */
public:
  int x0, y0;			/* xmin and ymin */
  int x1, y1;			/* xmax and ymax (inclusive) */
};

class Poly {	/* A POLYGON */
public:
  enum CLIP {
    CLIP_OUT=0,   // polygon entirely outside box
    CLIP_PARTIAL, // polygon partially inside
    CLIP_IN       // polygon entirely inside box
  };
  enum SCAN {
    SCAN_START= 0,  // first pixel of scan line
    SCAN_MIDDLE,    // somewhere in the middle
    SCAN_END,       // last pixel of scan line
    SCAN_START_END  // one pixel long scans
  };
  struct Vert { // don't put > 32 doubles in Vert struct; mask will overflow
    double sx, sy, sz, sw;	/* screen space position (sometimes homo.) */
    double x, y, z;		/* world space position */
    double u, v, q;		/* texture position (sometimes homogeneous) */
    double r, g, b;		/* (red,green,blue) color */
    double nx, ny, nz;		/* world space normal vector */
    void print( FILE* ofile, const char* str, const unsigned long mask );
    Vert& operator=( const gPoint& other )
      { 
	x= other.x(); y= other.y(); z= other.z(); 
	nx= ny= nz= 0.0;
	return *this;
      }
    void trans_world( const gTransfm& world_trans, 
		      const gTransfm& world_trans_norm )
      {
	gPoint trans_pt= world_trans * gPoint( x, y, z );
	x= trans_pt.x(); y= trans_pt.y(); z= trans_pt.z();
	gVector trans_norm= world_trans_norm * gVector( nx, ny, nz );
	nx= trans_norm.x(); ny= trans_norm.y(); nz= trans_norm.z();
      }
    void trans_to_screen( gTransfm& screen_trans )
      {
	gPoint trans_pt= screen_trans * gPoint( x, y, z );
	sx= trans_pt.x(); sy= trans_pt.y(); sz= trans_pt.z(); sw= trans_pt.w();
      }
  };
  int n;			/* number of sides */
  unsigned long mask;		/* interpolation mask for vertex elems */
  Vert vert[POLY_NMAX];         /* vertices */
  Poly() { n=0; }
  void print( FILE* ofile, const char* str );
  static void vert_label( FILE* ofile, const char* str, 
			const unsigned long mask );
  CLIP clip_to_box(Poly_box* box);
  CLIP clip_to_box_sides(Poly_box* box);
  void scan( Poly_window* win, 
	    int (*pixelproc)(const int x, const int y, Vert* p,
			      Poly::SCAN step, void* cb_data_dummy),
	    void* cb_data);
  void trans_world( const gTransfm& world_trans, 
		    const gTransfm& world_trans_norm )
    {
      for (int i=0; i<n; i++) vert[i].trans_world( world_trans, 
						   world_trans_norm );
    }
  void trans_to_screen( gTransfm& screen_trans )
    {
      for (int i=0; i<n; i++) vert[i].trans_to_screen( screen_trans );
    }
  void homogenize(); // divide out homogeneous coords
  int front_facing(); // must be in screen coords
  static Vert* dummy_vert; // used in computing masks
private:
  static void clip_to_halfspace( Poly* p, Poly* q, const int index,
				const double sign, const double k );
  inline static void incrementalize_y( double* p1, double* p2, double* p, 
				      double* dp, int y, unsigned long mask );
  inline static void incrementalize_x( double* p1, double* p2, double* p, 
				      double* dp, int x, unsigned long mask );
  inline static void increment( double* p, double* dp, unsigned long mask )
    {
      for (; mask!=0; mask>>=1, p++, dp++)
	if (mask&1)
	  *p += *dp;
    }
  inline static int scanline( int y, Vert *l, Vert *r,
			      Poly_window *win, 
			      int (*pixelproc)( const int x, const int y, 
						Poly::Vert* p, Poly::SCAN step,
						void* cb_data_dummy ), 
			      void* cb_data,
			      unsigned long mask );
};

#define POLY_MASK(elem) (1 << (&(Poly::dummy_vert->elem) \
			       - (double *)(Poly::dummy_vert)))

/*
 * mask is an interpolation mask whose kth bit indicates whether the kth
 * double in a Poly::Vert is relevant.
 * For example, if the valid attributes are sx, sy, and sz, then set
 *	mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz);
 */

#endif
