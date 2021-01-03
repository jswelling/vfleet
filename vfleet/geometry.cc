/****************************************************************************
 * geometry.cc
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
 *
 * The bounding box intersection algorithm included herein is derived from
 * that of Rayshade 4.0 by Craig Kolb.  Copyright information for that
 * algorithm follows:
 ***
 *  Copyright (C) 1989, 1991, Craig E. Kolb
 *  All rights reserved.
 *
 *  This software may be freely copied, modified, and redistributed
 *  provided that this copyright notice is preserved on all copies.
 *
 *  You may not distribute this software, in whole or in part, as part of
 *  any commercial product without the express consent of the authors.
 *
 *  There is no warranty or other guarantee of fitness of this software
 *  for any purpose.  It is provided solely "as is".
 ***
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "geometry.h"

/* Notes-
 */

#if ( CRAY_ARCH_T3D )
extern "C" double SQRTI( double* input );
#endif

/* Vectors are considered to align if the ratio of their cross product 
 * squared to their dot product squared is less than the following value.
 */
static const float alignment_epsilon= 0.00001;

void gVector::normalize() 
{
#if ( CRAY_ARCH_T3D )
  double lsqr= lengthsqr();
  if (lsqr != 0) {
    float l_inv= SQRTI(&lsqr);
    for (int i=0; i<4; i++) data[i] *= l_inv;
  }
  else {
    fprintf(stderr,
	    "gVector::normalize: Tried to normalize a zero length vector!\n");
    abort();
  }
#elif (CRAY_ARCH_T3E )
  float lsqr= lengthsqr();
  if (lsqr != 0) {
    // compiler is supposed to recognize and optimize 1/sqrt
    float l_inv= 1.0/sqrtf(lsqr); 
    data[0] *= l_inv;
    data[1] *= l_inv;
    data[2] *= l_inv;
    data[3] *= l_inv;
  }
  else {
    fprintf(stderr,
	    "gVector::normalize: Tried to normalize a zero length vector!\n");
    abort();
  }
#else
  float l= length();
  float one_over_l;
  if (l != 0.0) {
	one_over_l = 1.0 / l;
    register int i;

    for (i=0; i<4; i++) data[i] *= one_over_l;
  }
  else {
    fprintf(stderr,
	    "gVector::normalize: Tried to normalize a zero length vector!\n");
    abort();
  }
#endif
}

/* This routine returns a rotation which will reverse this
 * vector (as opposed to a simple inversion, which is not a rotation\).
 */
gTransfm *gVector::flipping_transform()
{
  float dot;

  /* Find a vector not parallel to the given vector */
  gVector p( 0.0, 0.0, 1.0 );
  dot= p*(*this);
  gVector cross= p ^ (*this);
  if ( (cross*cross) < alignment_epsilon*dot*dot ) { /* this p won't work */
    p= gVector( 1.0, 0.0, 0.0 );
    dot= p*(*this);
    };

  /* Extract the normal component of that vector */
  p= p.normal_component_wrt( (*this) );

  /* Return a 180 degree rotation about that vector */
  return( gTransfm::rotation( &p, 180.0 ) );
}

/* This routine returns a rotation which will rotate this vector
 * to align with its second parameter vector.
 */
gTransfm *gVector::aligning_rotation( const gVector& other )
{
  gVector a= (*this) ^ other;
  float dot= (*this) * other;

  if ( a*a < alignment_epsilon*dot*dot ) { /* They already align */
    if (dot >= 0.0 ) /* parallel */
      return( new gTransfm( gTransfm::identity ) );
    else /* anti-parallel */
      return( flipping_transform() );
  }

    float theta= acos( dot / ((*this).length() * other.length()) );
    return( gTransfm::rotation( &a, RadtoDeg*theta ) );
}

/*
* This routine returns the component of this vector perpendicular to
* the given vector.
*/
gVector gVector::normal_component_wrt( const gVector& other )
{
  gVector perpvec;
  if (other.lengthsqr() > 0.0) 
    return( (*this) - ( other * (((*this)*other)/other.lengthsqr()) ) );

  return( *this ); // error exit- other is null
}

gTransfm gTransfm::identity;

gTransfm::gTransfm()
{
  int i;
  for (i=0; i<16; i++) data[i]= 0.0;
  data[0]= data[5]= data[10]= data[15]= 1.0;
}

gTransfm::gTransfm( const float data_in[16] )
{
  for (int i=0; i<16; i++) data[i]= data_in[i];
}

gTransfm::gTransfm( const gTransfm& other )
{
  int i;
  for (i=0; i<16; i++) data[i]= other.data[i];
}

void gTransfm::dump() const
{
  int i;
  for (i=0; i<16; i+=4) 
    fprintf(stderr,"( %f %f %f %f )\n",
	    data[i], data[i+1], data[i+2], data[i+3]);
}

gTransfm *gTransfm::scale( float val )
{
  gTransfm *result= new gTransfm; // automatically zeroed
  result->data[0]= result->data[5]= result->data[10]= val;
  result->data[15]= 1.0;

  return(result);  
}

gTransfm *gTransfm::scale( float xval, float yval, float zval )
{
  gTransfm *result= new gTransfm; // automatically identity
  result->data[0]= xval;
  result->data[5]= yval;
  result->data[10]= zval;
  result->data[15]= 1.0;

  return(result);
}

gTransfm *gTransfm::rotation( gVector *axis, float angle )
{
  gTransfm *result= new gTransfm; // automatically zeroed
  float x, y, z, norm;
  float s, c;

  x= axis->x();
  y= axis->y();
  z= axis->z();
  norm= sqrt( x*x + y*y + z*z );
  if (norm==0.0) {
    fprintf(stderr,"gTransfm::rotation: axis degenerate; using z axis!\n");
    x= 0.0;
    y= 0.0;
    z= 1.0;
  }
  else {
    x= x/norm;
    y= y/norm;
    z= z/norm;
  }
  
  s= sin( DegtoRad * angle );
  c= cos( DegtoRad * angle );

  *(result->data+0)=   x*x + (1.0-x*x)*c;
  *(result->data+1)=   x*y*(1.0-c) - z*s;
  *(result->data+2)=   x*z*(1.0-c) + y*s;
  *(result->data+3)=   0.0;
  *(result->data+4)=   x*y*(1.0-c) + z*s;
  *(result->data+5)=   y*y + (1.0-y*y)*c;
  *(result->data+6)=   y*z*(1.0-c) - x*s;
  *(result->data+7)=   0.0;
  *(result->data+8)=   x*z*(1.0-c) - y*s;
  *(result->data+9)=   y*z*(1.0-c) + x*s;
  *(result->data+10)=  z*z + (1.0-z*z)*c;
  *(result->data+11)=  0.0;
  *(result->data+12)=  0.0;
  *(result->data+13)=  0.0;
  *(result->data+14)=  0.0;
  *(result->data+15)=  1.0;
  
  return( result );
}

gTransfm *gTransfm::translation( float x, float y, float z )
{
  gTransfm *result= new gTransfm; // automatically identity
  result->data[3]= x;
  result->data[7]= y;
  result->data[11]= z;
  return( result );
}

void gTransfm::transpose_self()
{
  register float temp;
  register int i, j;

  for (i=0; i<4; i++)
    for (j=0; j<i; j++)
      if ( i != j) {
	temp= *(data + 4*i +j);
	*(data + 4*i + j)= *(data + 4*j +i);
	*(data + 4*j + i)= temp;
      };
}

gTransfm& gTransfm::operator=( const gTransfm& a )
{
  if (this != &a) { // beware of s=s
    int i;
    for (i=0; i<16; i++) data[i]= a.data[i];
  }
  return( *this );
}

gTransfm& gTransfm::operator*( const gTransfm& a )
{
  int i, row_shift;
  static gTransfm result;
  gTransfm scratch;
  register float x, y, z, w;

  float* scratch_data= scratch.data;

  for (row_shift = 0; row_shift<16; row_shift+=4) {
    x = data[row_shift] * a.data[0];
    y = data[row_shift] * a.data[1];
    z = data[row_shift] * a.data[2];
    w = data[row_shift] * a.data[3];
    
    for (i=1;i<4;i++) {
      x += data[row_shift+i] * a.data[ 4*i ];
      y += data[row_shift+i] * a.data[(4*i) + 1];
      z += data[row_shift+i] * a.data[(4*i) + 2];
      w += data[row_shift+i] * a.data[(4*i) + 3];
    }
    *scratch_data++= x;
    *scratch_data++= y;
    *scratch_data++= z;
    *scratch_data++= w;
  }

  result= scratch;

  return( result );
}

gVector gTransfm::operator*( const gVector& vec ) const
{
  register float x;
  register float y;
  register float z;
  register float w;
  gVector result;

  x= data[0] * vec.data[0];
  y= data[4] * vec.data[0];
  z= data[8] * vec.data[0];
  w= data[12] * vec.data[0];

  for (int column= 1; column<4; column++ ) {
    x += data[column]*vec.data[column];
    y += data[column+4]*vec.data[column];
    z += data[column+8]*vec.data[column];
    w += data[column+12]*vec.data[column];
  }

  result.data[0]= x;
  result.data[1]= y;
  result.data[2]= z;
  result.data[3]= w;
  return result;
}

gPoint gTransfm::operator*( const gPoint& pt ) const
{
  register float x;
  register float y;
  register float z;
  register float w;
  gPoint result;

  x= data[0] * pt.data[0];
  y= data[4] * pt.data[0];
  z= data[8] * pt.data[0];
  w= data[12] * pt.data[0];

  for (int column= 1; column<4; column++ ) {
    x += data[column]*pt.data[column];
    y += data[column+4]*pt.data[column];
    z += data[column+8]*pt.data[column];
    w += data[column+12]*pt.data[column];
  }

  result.data[0]= x;
  result.data[1]= y;
  result.data[2]= z;
  result.data[3]= w;
  return result;
}

char *gTransfm::tostring() const
{
  char *result= new char[16*17+1];
  (void)sprintf(result,
		"%16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f ",
		data[0],data[1],data[2],data[3],
		data[4],data[5],data[6],data[7],
		data[8],data[9],data[10],data[11],
		data[12],data[13],data[14],data[15]);
  return( result );
}

gTransfm gTransfm::fromstring(char **buf)
{
  gTransfm t;
  (void)sscanf(*buf,
	       "%16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f ",
	       &(t.data[0]),&(t.data[1]),&(t.data[2]),&(t.data[3]),
	       &(t.data[4]),&(t.data[5]),&(t.data[6]),&(t.data[7]),
	       &(t.data[8]),&(t.data[9]),&(t.data[10]),&(t.data[11]),
	       &(t.data[12]),&(t.data[13]),&(t.data[14]),&(t.data[15]));
  *buf += 16*17;
  return(t);
}

gBoundBox::gBoundBox( float xin_llf, float yin_llf, float zin_llf,
		      float xin_trb, float yin_trb, float zin_trb )
{
  llf= gPoint( xin_llf, yin_llf, zin_llf );
  trb= gPoint( xin_trb, yin_trb, zin_trb );
}

gBoundBox::gBoundBox()
{
  llf= gPoint( 0.0, 0.0, 0.0 );
  trb= gPoint( 1.0, 1.0, 1.0 );
}

gBoundBox::gBoundBox( const gBoundBox& a )
{
  llf= a.llf;
  trb= a.trb;
}

gBoundBox::~gBoundBox()
{
  // Nothing to do
}

gBoundBox& gBoundBox::operator=( const gBoundBox& other )
{
  if (this != &other) {
    llf= other.llf;
    trb= other.trb;
  }
  return( *this );
}

////
// Check for intersection between bounding box and the given ray.
// If there is an intersection between mindist and *maxdist along
// the ray, *maxdist is replaced with the distance to the point of
// intersection, and a non-zero is returned.  Otherwise, zero is returned.
//
// If this routine is used to check for intersection with a volume
// rather than a "hollow" box, one should first determine if
// (ray->pos + mindist * ray->dir) is inside the bounding volume, and
// call BoundsIntersect() only if it is not.
////
int gBoundBox::intersect( const gVector& raydir, const gPoint& raypos,
			  const float mindist, float *maxdist ) const
{
  double t, tmin, tmax;
  double dir, pos;
  
  tmax = *maxdist;
  tmin = mindist;
  
  dir = raydir.x();
  pos = raypos.x();
  
  if (dir < 0) {
    t = (llf.x() - pos) / dir;
    if (t < tmin)
      return(0);
    if (t <= tmax)
      tmax = t;
    t = (trb.x() - pos) / dir;
    if (t >= tmin) {
      if (t > tmax)
	return(0);
      tmin = t;
    }
  } else if (dir > 0.) {
    t = (trb.x() - pos) / dir;
    if (t < tmin)
      return(0);
    if (t <= tmax)
      tmax = t;
    t = (llf.x() - pos) / dir;
    if (t >= tmin) {
      if (t > tmax)
	return(0);
      tmin = t;
    }
  } else if (pos < llf.x() || pos > trb.x())
    return(0);
  
  dir = raydir.y();
  pos = raypos.y();
  
  if (dir < 0) {
    t = (llf.y() - pos) / dir;
    if (t < tmin)
      return(0);
    if (t <= tmax)
      tmax = t;
    t = (trb.y() - pos) / dir;
    if (t >= tmin) {
      if (t > tmax)
	return(0);
      tmin = t;
    }
  } else if (dir > 0.) {
    t = (trb.y() - pos) / dir;
    if (t < tmin)
      return(0);
    if (t <= tmax)
      tmax = t;
    t = (llf.y() - pos) / dir;
    if (t >= tmin) {
      if (t > tmax)
	return(0);
      tmin = t;
    }
  } else if (pos < llf.y() || pos > trb.y())
    return(0);
  
  dir = raydir.z();
  pos = raypos.z();
  
  if (dir < 0) {
    t = (llf.z() - pos) / dir;
    if (t < tmin)
      return(0);
    if (t <= tmax)
      tmax = t;
    t = (trb.z() - pos) / dir;
    if (t >= tmin) {
      if (t > tmax)
	return(0);
      tmin = t;
    }
  } else if (dir > 0.) {
    t = (trb.z() - pos) / dir;
    if (t < tmin)
      return(0);
    if (t <= tmax)
      tmax = t;
    t = (llf.z() - pos) / dir;
    if (t >= tmin) {
      if (t > tmax)
	return(0);
      tmin = t;
    }
  } else if (pos < llf.z() || pos > trb.z())
    return(0);
  
  /*
   * If tmin == mindist, then there was no "near"
   * intersection farther than EPSILON away.
   */
  if (tmin == mindist) {
    if (tmax < *maxdist) {
      *maxdist = tmax;
      return(1);
    }
  } else {
    if (tmin < *maxdist) {
      *maxdist = tmin;
      return(1);
    }
  }
  return(0);	/* hit, but not closer than maxdist */
}
