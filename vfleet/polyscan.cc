/* This functionality is derived from Paul Heckbert's polygon scan conversion
 * routines, "Generic Convex Polygon Scan Conversion and Clipping", in
 * Graphics Gems (the first book of the series).
 */

/*
 * poly.c: simple utilities for polygon data structures
 */

#include <stdio.h>
#include <stdlib.h>

#if ( HPUX || IBM_RS6000 || SGI_MIPS || SUN_SPARC )
#include <strings.h>
#else
#include <string.h>
#endif

#include <math.h>
#include "polyscan.h"

#define SWAP(a, b, temp)	{temp = a; a = b; b = temp;}
#define COORD(vert, i) ((double *)(vert))[i]

#define CLIP_AND_SWAP(elem, sign, k, p, q, r) { \
    Poly::clip_to_halfspace(p, q, &v->elem-(double *)v, sign, sign*k); \
    if (q->n==0) {this->n = 0; return Poly::CLIP_OUT;} \
    SWAP(p, q, r); \
}

Poly::Vert* Poly::dummy_vert= new Poly::Vert; // used by POLY_MASK macro

void Poly::print( FILE* ofile, const char* str )
{
  int i;
  
  fprintf(ofile,"%s: %d sides\n", str, n);
  Poly::vert_label(ofile,"        ", mask);
  for (i=0; i<n; i++) {
    fprintf(ofile,"   v[%d] ", i);
    vert[i].print(ofile,"",mask);
  }
}

void Poly::vert_label(FILE* ofile, const char* str, const unsigned long mask)
{
    fprintf(ofile,"%s", str);

    if (mask&POLY_MASK(sx))   fprintf(ofile,"   sx  ");
    if (mask&POLY_MASK(sy))   fprintf(ofile,"   sy  ");
    if (mask&POLY_MASK(sz))   fprintf(ofile,"   sz  ");
    if (mask&POLY_MASK(sw))   fprintf(ofile,"   sw  ");
    if (mask&POLY_MASK(x))    fprintf(ofile,"   x   ");
    if (mask&POLY_MASK(y))    fprintf(ofile,"   y   ");
    if (mask&POLY_MASK(z))    fprintf(ofile,"   z   ");
    if (mask&POLY_MASK(u))    fprintf(ofile,"   u   ");
    if (mask&POLY_MASK(v))    fprintf(ofile,"   v   ");
    if (mask&POLY_MASK(q))    fprintf(ofile,"   q   ");
    if (mask&POLY_MASK(r))    fprintf(ofile,"   r   ");
    if (mask&POLY_MASK(g))    fprintf(ofile,"   g   ");
    if (mask&POLY_MASK(b))    fprintf(ofile,"   b   ");
    if (mask&POLY_MASK(nx))   fprintf(ofile,"   nx  ");
    if (mask&POLY_MASK(ny))   fprintf(ofile,"   ny  ");
    if (mask&POLY_MASK(nz))   fprintf(ofile,"   nz  ");
    fprintf(ofile,"\n");
}

void Poly::Vert::print(FILE* ofile, const char* str, const unsigned long mask)
{
    fprintf(ofile,"%s", str);
    if (mask&POLY_MASK(sx)) fprintf(ofile," %6.1f", sx);
    if (mask&POLY_MASK(sy)) fprintf(ofile," %6.1f", sy);
    if (mask&POLY_MASK(sz)) fprintf(ofile," %6.2f", sz);
    if (mask&POLY_MASK(sw)) fprintf(ofile," %6.2f", sw);
    if (mask&POLY_MASK(x))  fprintf(ofile," %6.2f", x);
    if (mask&POLY_MASK(y))  fprintf(ofile," %6.2f", y);
    if (mask&POLY_MASK(z))  fprintf(ofile," %6.2f", z);
    if (mask&POLY_MASK(u))  fprintf(ofile," %6.2f", u);
    if (mask&POLY_MASK(v))  fprintf(ofile," %6.2f", v);
    if (mask&POLY_MASK(q))  fprintf(ofile," %6.2f", q);
    if (mask&POLY_MASK(r))  fprintf(ofile," %6.4f", r);
    if (mask&POLY_MASK(g))  fprintf(ofile," %6.4f", g);
    if (mask&POLY_MASK(b))  fprintf(ofile," %6.4f", b);
    if (mask&POLY_MASK(nx)) fprintf(ofile," %6.3f", nx);
    if (mask&POLY_MASK(ny)) fprintf(ofile," %6.3f", ny);
    if (mask&POLY_MASK(nz)) fprintf(ofile," %6.3f", nz);
    fprintf(ofile,"\n");
}


void Poly::homogenize()
{
  int i;
  if (mask&POLY_MASK(sx)) for (i=0; i<n; i++) vert[i].sx /= vert[i].sw;
  if (mask&POLY_MASK(sy)) for (i=0; i<n; i++) vert[i].sy /= vert[i].sw;
  if (mask&POLY_MASK(sz)) for (i=0; i<n; i++) vert[i].sz /= vert[i].sw;
  if (mask&POLY_MASK(sw)) for (i=0; i<n; i++) vert[i].sw= 1.0;
  if (mask&POLY_MASK(u))  for (i=0; i<n; i++) vert[i].u /= vert[i].sw;
  if (mask&POLY_MASK(v))  for (i=0; i<n; i++) vert[i].v /= vert[i].sw;
  if (mask&POLY_MASK(q))  for (i=0; i<n; i++) vert[i].q = 1.0/vert[i].sw;
}

/*
 * Poly::clip_to_halfspace: clip convex polygon p against a plane,
 * copying the portion satisfying sign*s[index] < k*sw into q,
 * where s is a Poly::Vert* cast as a double*.
 * index is an index into the array of doubles at each vertex, such that
 * s[index] is sx, sy, or sz (screen space x, y, or z).
 * Thus, to clip against xmin, use
 *	poly_clip_to_halfspace(p, q, XINDEX, -1., -xmin);
 * and to clip against xmax, use
 *	poly_clip_to_halfspace(p, q, XINDEX,  1.,  xmax);
 */

void Poly::clip_to_halfspace(Poly* p, Poly* q, const int index, 
			    const double sign, const double k)
{
    register unsigned long m;
    register double *up, *vp, *wp;
    register Vert *v;
    int i;
    Vert *u;
    double t, tu, tv;

    q->n = 0;
    q->mask = p->mask;

    /* start with u=vert[n-1], v=vert[0] */
    u = &p->vert[p->n-1];
    tu = sign*COORD(u, index) - u->sw*k;
    for (v= &p->vert[0], i=p->n; i>0; i--, u=v, tu=tv, v++) {
	/* on old polygon (p), u is previous vertex, v is current vertex */
	/* tv is negative if vertex v is in */
	tv = sign*COORD(v, index) - v->sw*k;
	if ( (tu<=0.) ^ (tv<=0.) ) {
	    /* edge crosses plane; add intersection point to q */
	    t = tu/(tu-tv);
	    up = (double *)u;
	    vp = (double *)v;
	    wp = (double *)&q->vert[q->n];
	    for (m=p->mask; m!=0; m>>=1, up++, vp++, wp++)
		if (m&1) *wp = *up+t*(*vp-*up);
	    q->n++;
	}
	if (tv<=0.)		/* vertex v is in, copy it to q */
	    q->vert[q->n++] = *v;
    }
}

/*
 * Poly::clip_to_box: Clip the convex polygon p1 to the screen space box
 * using the homogeneous screen coordinates (sx, sy, sz, sw) of each vertex,
 * testing if v->sx/v->sw > box->x0 and v->sx/v->sw < box->x1,
 * and similar tests for y and z, for each vertex v of the polygon.
 * If polygon is entirely inside box, then POLY_CLIP_IN is returned.
 * If polygon is entirely outside box, then POLY_CLIP_OUT is returned.
 * Otherwise, if the polygon is cut by the box, p1 is modified and
 * POLY_CLIP_PARTIAL is returned.
 *
 * Given an n-gon as input, clipping against 6 planes could generate an
 * (n+6)gon, so POLY_NMAX in poly.h must be big enough to allow that.
 */
Poly::CLIP Poly::clip_to_box(Poly_box *box)
{
  int x0out = 0, x1out = 0, y0out = 0, y1out = 0, z0out = 0, z1out = 0;
  register int i;
  register Vert *v;
  Poly p2, *p, *q, *r;
  
  if (n+6>POLY_NMAX) {
    fprintf(stderr, "Poly::clip_to_box: too many vertices: %d (max=%d-6)\n",
	    n, POLY_NMAX);
    exit(1);
  }
  if (sizeof(Poly::Vert)/sizeof(double) > 32) {
    fprintf(stderr, "Poly::Vert structure too big; must be <=32 doubles\n");
    exit(1);
  }
  
  /* count vertices "outside" with respect to each of the six planes */
  for (v=vert, i=n; i>0; i--, v++) {
    if (v->sx < box->x0*v->sw) x0out++;     /* out on left */
    if (v->sx > box->x1*v->sw) x1out++;     /* out on right */
    if (v->sy < box->y0*v->sw) y0out++;     /* out on top */
    if (v->sy > box->y1*v->sw) y1out++;     /* out on bottom */
    if (v->sz < box->z0*v->sw) z0out++;     /* out on near */
    if (v->sz > box->z1*v->sw) z1out++;     /* out on far */
  }
  
  /* check if all vertices inside */
  if (x0out+x1out+y0out+y1out+z0out+z1out == 0) return Poly::CLIP_IN;
  
  /* check if all vertices are "outside" any of the six planes */
  if (x0out==n || x1out==n || y0out==n ||
      y1out==n || z0out==n || z1out==n) {
    n = 0;
    return Poly::CLIP_OUT;
  }
  
  /*
   * now clip against each of the planes that might cut the polygon,
   * at each step toggling between polygons this and p2
   */
  p = this;
  q = &p2;
  if (x0out) CLIP_AND_SWAP(sx, -1., box->x0, p, q, r);
  if (x1out) CLIP_AND_SWAP(sx,  1., box->x1, p, q, r);
  if (y0out) CLIP_AND_SWAP(sy, -1., box->y0, p, q, r);
  if (y1out) CLIP_AND_SWAP(sy,  1., box->y1, p, q, r);
  if (z0out) CLIP_AND_SWAP(sz, -1., box->z0, p, q, r);
  if (z1out) CLIP_AND_SWAP(sz,  1., box->z1, p, q, r);
  
  /* if result ended up in p2 then copy it to this */
  if (p==&p2)
#if ( DEC_ALPHA || HPUX )
    bcopy((char*)&p2, (char*)this, 
	  sizeof(Poly)-(POLY_NMAX-p2.n)*sizeof(Poly::Vert));
#else
    bcopy(&p2, this, sizeof(Poly)-(POLY_NMAX-p2.n)*sizeof(Poly::Vert));
#endif
  return Poly::CLIP_PARTIAL;
}

/*
 * Poly::clip_to_box_sides: like clip_to_box, but doesn't clip against
 * front and back (hither and yon planes).
 */
Poly::CLIP Poly::clip_to_box_sides(Poly_box *box)
{
  int x0out = 0, x1out = 0, y0out = 0, y1out = 0, z0out = 0, z1out = 0;
  register int i;
  register Vert *v;
  Poly p2, *p, *q, *r;
  
  if (n+6>POLY_NMAX) {
    fprintf(stderr, "Poly::clip_to_box: too many vertices: %d (max=%d-6)\n",
	    n, POLY_NMAX);
    exit(1);
  }
  if (sizeof(Poly::Vert)/sizeof(double) > 32) {
    fprintf(stderr, "Poly::Vert structure too big; must be <=32 doubles\n");
    exit(1);
  }
  
  /* count vertices "outside" with respect to each of the six planes */
  for (v=vert, i=n; i>0; i--, v++) {
    if (v->sx < box->x0*v->sw) x0out++;     /* out on left */
    if (v->sx > box->x1*v->sw) x1out++;     /* out on right */
    if (v->sy < box->y0*v->sw) y0out++;     /* out on top */
    if (v->sy > box->y1*v->sw) y1out++;     /* out on bottom */
  }
  
  /* check if all vertices inside */
  if (x0out+x1out+y0out+y1out+z0out+z1out == 0) return Poly::CLIP_IN;
  
  /* check if all vertices are "outside" any of the six planes */
  if (x0out==n || x1out==n || y0out==n ||
      y1out==n || z0out==n || z1out==n) {
    n = 0;
    return Poly::CLIP_OUT;
  }
  
  /*
   * now clip against each of the planes that might cut the polygon,
   * at each step toggling between polygons this and p2
   */
  p = this;
  q = &p2;
  if (x0out) CLIP_AND_SWAP(sx, -1., box->x0, p, q, r);
  if (x1out) CLIP_AND_SWAP(sx,  1., box->x1, p, q, r);
  if (y0out) CLIP_AND_SWAP(sy, -1., box->y0, p, q, r);
  if (y1out) CLIP_AND_SWAP(sy,  1., box->y1, p, q, r);
  if (z0out) CLIP_AND_SWAP(sz, -1., box->z0, p, q, r);
  if (z1out) CLIP_AND_SWAP(sz,  1., box->z1, p, q, r);
  
  /* if result ended up in p2 then copy it to this */
  if (p==&p2)
#if ( DEC_ALPHA || HPUX )
    bcopy((char*)&p2, (char*)this, 
	  sizeof(Poly)-(POLY_NMAX-p2.n)*sizeof(Poly::Vert));
#else
    bcopy(&p2, this, sizeof(Poly)-(POLY_NMAX-p2.n)*sizeof(Poly::Vert));
#endif
  return Poly::CLIP_PARTIAL;
}

void Poly::incrementalize_y( double* p1, double* p2, double* p, double* dp,
			    int y, unsigned long mask )
{
  // put intersection of line Y=y+.5 with edge between points
  // p1 and p2 in p, put change with respect to y in dp
  
  double dy, frac;
  
  dy = ((Vert *)p2)->sy - ((Vert *)p1)->sy;
  if (dy==0.) dy = 1.;
  frac = y+.5 - ((Vert *)p1)->sy;
  
  for (; mask!=0; mask>>=1, p1++, p2++, p++, dp++)
    if (mask&1) {
      *dp = (*p2-*p1)/dy;
      *p = *p1+*dp*frac;
    }
}

void Poly::incrementalize_x( double* p1, double* p2, double* p, double* dp,
			       int x, unsigned long mask )
{
  // put intersection of line X=x+.5 with edge between points
  // p1 and p2 in p, put change with respect to x in dp
  
  double dx, frac;
  
  dx = ((Vert *)p2)->sx - ((Vert *)p1)->sx;
  if (dx==0.) dx = 1.;
  frac = x+.5 - ((Vert *)p1)->sx;
  
  for (; mask!=0; mask>>=1, p1++, p2++, p++, dp++)
    if (mask&1) {
      *dp = (*p2-*p1)/dx;
      *p = *p1+*dp*frac;
    }
}

/* Poly::scanline: output scanline by sampling polygon at Y=y+.5 */
int Poly::scanline(int y, Poly::Vert* l, Poly::Vert* r, 
		   Poly_window* win, 
		   int (*pixelproc)(const int x, const int y, Poly::Vert* p,
				    Poly::SCAN step, void* cb_data_dummy), 
		   void* cb_data,
		   unsigned long mask)
{
  int x, lx, rx;
  Vert p, dp;
  
  mask &= ~POLY_MASK(sx);		/* stop interpolating screen x */
  lx = (int)ceil(l->sx-.5);
  if (lx<win->x0) lx = win->x0;
  rx = (int)floor(r->sx-.5);
  if (rx>win->x1) rx = win->x1;
  if (lx>rx) return 0;
  incrementalize_x((double*)l, (double*)r, (double*)&p, (double*)&dp, 
		   lx, mask);
  /* scan in x, generating pixels */
  if (rx>lx) {
    if ((*pixelproc)(lx, y, &p, SCAN_START, cb_data)) return 1;
    increment((double*)&p, (double*)&dp, mask);
    for (x=lx+1; x<rx; x++) {		/* scan in x, generating pixels */
      if ((*pixelproc)(x, y, &p, SCAN_MIDDLE, cb_data)) return 1;
      increment((double*)&p, (double*)&dp, mask);
    }
    if ((*pixelproc)(rx, y, &p, SCAN_END, cb_data)) return 1;
  }
  else { // one pixel long scan line
    if ((*pixelproc)(rx, y, &p, SCAN_START_END, cb_data)) return 1;
  }
  return 0;
}

/*
 * poly_scan: Scan convert a polygon, calling pixelproc at each pixel with an
 * interpolated Poly::Vert structure.  Polygon can be clockwise or ccw.
 * Polygon is clipped in 2-D to win, the screen space window.
 *
 * Scan conversion is done on the basis of Poly::Vert fields sx and sy.
 * These two must always be interpolated, and only they have special meaning
 * to this code; any other fields are blindly interpolated regardless of
 * their semantics.
 *
 * The pixelproc subroutine takes the arguments:
 *
 *	int pixelproc(cosnt int x, const int y, Poly::Vert* point)
 *
 * All the fields of point indicated by mask will be valid inside pixelproc
 * except sx and sy.  If they were computed, they would have values
 * sx=x+.5 and sy=y+.5, since sampling is done at pixel centers.  If pixelproc
 * returns non-zero, polygon scan conversion ceases and scan returns.
 */
void Poly::scan(Poly_window* win, 
		int (*pixelproc)(const int x, const int y, Poly::Vert* p,
				 Poly::SCAN step, void* cb_data_dummy),
		void* cb_data)
{
  register int i, li, ri, y, ly, ry, top, rem;
  register unsigned long rmask;
  double ymin;
  Vert l, r, dl, dr;
  
  if (n>POLY_NMAX) {
    fprintf(stderr, "poly_scan: too many vertices: %d\n", n);
    return;
  }
  if (sizeof(Poly::Vert)/sizeof(double) > 32) {
    fprintf(stderr, "Poly::Vert structure too big; must be <=32 doubles\n");
    exit(1);
  }
  
  ymin = HUGE;
  for (i=0; i<n; i++)		/* find top vertex (y points down) */
    if (vert[i].sy < ymin) {
      ymin = vert[i].sy;
      top = i;
    }
  
  li = ri = top;			/* left and right vertex indices */
  rem = n;				/* number of vertices remaining */
  y = (int)ceil(ymin-.5);		/* current scan line */
  ly = ry = y-1;			/* lower end of left & right edges */
  rmask = mask & ~POLY_MASK(sy);	/* stop interpolating screen y */
  
  while (rem>0) {	/* scan in y, activating new edges on left & right */
    /* as scan line passes over new vertices */
    
    while (ly<=y && rem>0) {	/* advance left edge? */
      rem--;
      i = li-1;			/* step ccw down left side */
      if (i<0) i = n-1;
      incrementalize_y((double*)&vert[li], (double*)&vert[i], 
		       (double*)&l, (double*)&dl, y, rmask);
      ly = (int)floor(vert[i].sy+.5);
      li = i;
    }
    while (ry<=y && rem>0) {	/* advance right edge? */
      rem--;
      i = ri+1;			/* step cw down right edge */
      if (i>=n) i = 0;
      incrementalize_y((double*)&vert[ri], (double*)&vert[i], 
		       (double*)&r, (double*)&dr, y, rmask);
      ry = (int)floor(vert[i].sy+.5);
      ri = i;
    }
    
    while (y<ly && y<ry) {	    /* do scanlines till end of l or r edge */
      if (y>=win->y0 && y<=win->y1)
	if (l.sx<=r.sx) {
	  if (scanline(y, &l, &r, win, pixelproc, cb_data, rmask)) return;
	}
	else {
	  if (scanline(y, &r, &l, win, pixelproc, cb_data, rmask)) return;
	}
      y++;
      increment((double*)&l, (double*)&dl, rmask);
      increment((double*)&r, (double*)&dr, rmask);
    }
  }
}

int Poly::front_facing()
{
  // Polygon is assumed to be in screen coords.

  // Points and lines are always front-facing
  if (n<3) return 1;
  else {
    double val= 
      vert[0].sw*( (vert[1].sx*vert[2].sy)-(vert[1].sy*vert[2].sx) )
      + vert[1].sw*( (vert[0].sy*vert[2].sx)-(vert[0].sx*vert[2].sy) )
      + vert[2].sw*( (vert[0].sx*vert[1].sy)-(vert[0].sy*vert[1].sx) );
    return (val >= 0.0);
  }
}
