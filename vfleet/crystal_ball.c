/****************************************************************************
 * crystal_ball.c
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/*
This module provides a 'crystal ball' type 3D motion interface.
*/

#include <stdio.h>
#include <math.h>
#include "crystal_ball.h"

/* crystal ball radius (1.0 is full width of viewport) */
#define CRYSTAL_BALL_RADIUS 0.8

/* Vectors are considered to align if the ratio of their cross product 
 * squared to their dot product squared is less than the following value.
 */
#define epsilon 0.00001

static float *make_translate_c(Tx,Ty,Tz)
float Tx,Ty,Tz;
{
	float *newTrans;
	int row,column;
#ifndef NOMALLOCDEF
	char *malloc();
#endif
	
	if ( !(newTrans = (float *) malloc(16*sizeof(float))) ) {
	  fprintf(stderr,
		  "make_translate_c: unable to allocate 16 floats!\n");
	  exit(2);
	}
	for (row=0;row<4;row++)
		for(column=0;column<4;column++)
			{
			if (column == row)	newTrans[4*row+column] = 1.0;
			else newTrans[4*row+column] = 0.0;
			}
	newTrans[3]=Tx;
	newTrans[7]=Ty;
	newTrans[11]=Tz;
	return(newTrans);
}

static float *make_x_rotate_c(angle)
float angle;
/* This routine returns a rotation about the x axis in c form. */
{
	float *newRotat,radAngle;
	int i;
#ifndef NOMALLOCDEF
	char *malloc();
#endif

	radAngle = angle * DegtoRad;
	if ( !(newRotat = (float *) malloc(16*sizeof(float))) ) {
	  fprintf(stderr,"make_x_rotate_c: can't allocate 16 floats!\n");
	  exit(2);
	}
	for (i=0;i<16;i++) newRotat[i]=0.0;

	newRotat[0]= 1.0;
	newRotat[5]= cos(radAngle);
	newRotat[6]= -sin(radAngle);
	newRotat[9]= sin(radAngle);
	newRotat[10]= cos(radAngle);
	newRotat[15]= 1.0;

	return( newRotat );
}

static float *make_y_rotate_c(angle)
float angle;
/* This routine returns a rotation about the y axis in c form. */
{
	float *newRotat,radAngle;
	int i;
#ifndef NOMALLOCDEF
	char *malloc();
#endif

	radAngle = angle * DegtoRad;
	if ( !(newRotat = (float *) malloc(16*sizeof(float))) ) {
	  fprintf(stderr,"make_y_rotate_c: can't allocate 16 floats!");
	  exit(2);
	}
	for (i=0;i<16;i++) newRotat[i]=0.0;

	newRotat[0]= cos(radAngle);
	newRotat[2]= sin(radAngle);
	newRotat[5]= 1.0;
	newRotat[8]= -sin(radAngle);
	newRotat[10]= cos(radAngle);
        newRotat[15]= 1.0;

	return( newRotat );
}

static float *make_z_rotate_c(angle)
float angle;
/* This routine returns a rotation about the z axis in c form. */
{
	float *newRotat,radAngle;
	int i;
#ifndef NOMALLOCDEF
	char *malloc();
#endif

	radAngle = angle * DegtoRad;
	if ( !(newRotat = (float *) malloc(16*sizeof(float))) ) {
	  fprintf(stderr,"make_z_rotate_c: can't allocate 16 floats!");
	  exit(2);
	}
	for (i=0;i<16;i++) newRotat[i]=0.0;

	newRotat[0]= cos(radAngle);
	newRotat[1]= -sin(radAngle);
	newRotat[4]= sin(radAngle);
	newRotat[5]= cos(radAngle);
	newRotat[10]= 1.0;
	newRotat[15]= 1.0;

	return( newRotat );
}

static float *make_scale_c(Sx,Sy,Sz)
float Sx,Sy,Sz;
{
	float *newScale;
	int row,column;
#ifndef NOMALLOCDEF
	char *malloc();
#endif
	
	if ( !(newScale = (float *) malloc(16*sizeof(float))) ) {
	  fprintf(stderr,"make_scale_c: unable to allocate 16 floats!");
	  exit(2);
	}
	for (row=0;row<4;row++)
		for(column=0;column<4;column++)
			{
			if (column == row)	newScale[4*row+column] = 1.0;
			else newScale[4*row+column] = 0.0;
			}
	newScale[0]= newScale[0] * Sx;
	newScale[5]= newScale[5] * Sy;
	newScale[10]= newScale[10] * Sz;
	return(newScale);
}

static float *matrix_mult_c(M1,M2)
float M1[16], M2[16];
{
	int row,column,i;
	float *newMatrix;
#ifndef NOMALLOCDEF
	char *malloc();
#endif

	if ( !(newMatrix = (float *) malloc(16*sizeof(float))) ) {
	  fprintf(stderr,"matrix_mult_c: unable to allocate 16 floats!");
	  exit(2);
	}

	for (i=0;i<16;i++) newMatrix[i]=0.0;
	for (row = 0;row<4;row++)
		for (column= 0;column<4;column++)
			for (i=0;i<4;i++)
				newMatrix[(4*row)+column] += M1[(4*row)+i]*
				   M2[(4*i)+column];
	
	return(newMatrix);
}

static float *transpose( matrix )
float *matrix;
/* This routine replaces the given matrix with its transpose. */
{
	register float temp;
	register int i,j;

	for (i=0; i<4; i++)
		for (j=0; j<i; j++)
			if ( i != j) {
				temp= *(matrix + 4*i +j);
				*(matrix + 4*i + j)= *(matrix + 4*j +i);
				*(matrix + 4*j + i)= temp;
				};

	return( matrix );
}

/* 
This routine generates a rotation about an arbitrary axis. See, for 
example, the Ardent Dore Reference Guide.  (That source apparently
has a sign error in the (result+0) element, however).
*/
static float *make_arbitrary_rotate_c( theta, x, y, z )
float theta, x, y, z;
{
	float s, c;
	float *result;

#ifdef DEBUG
	fprintf(stderr,
		"Make_Arbitrary_Rotate: theta= %f, x= %f, y= %f, z= %f\n",
		theta, x, y, z);
#endif

	s= sin( DegtoRad * theta );
	c= cos( DegtoRad * theta );

	result= (float *)malloc( 16*sizeof(float) );
	if (!result) {
	  fprintf(stderr,
		  "Make_Arbitrary_Rotate: unable to allocate 16 floats!\n");
	  exit(2);
	}

	*(result+0)=   x*x + (1.0-x*x)*c;
	*(result+1)=   x*y*(1.0-c) - z*s;
	*(result+2)=   x*z*(1.0-c) + y*s;
	*(result+3)=   0.0;
	*(result+4)=   x*y*(1.0-c) + z*s;
	*(result+5)=   y*y + (1.0-y*y)*c;
	*(result+6)=   y*z*(1.0-c) - x*s;
	*(result+7)=   0.0;
	*(result+8)=   x*z*(1.0-c) - y*s;
	*(result+9)=   y*z*(1.0-c) + x*s;
	*(result+10)=  z*z + (1.0-z*z)*c;
	*(result+11)=  0.0;
	*(result+12)=  0.0;
	*(result+13)=  0.0;
	*(result+14)=  0.0;
	*(result+15)=  1.0;

	return( result );
}

/* This routine generates a rotation matrix. */
static float *make_rotate_c(angle, xcomp, ycomp, zcomp)
float angle, xcomp, ycomp, zcomp;
{
	float *matrix, norm;

#ifdef DEBUG
	fprintf(stderr,"make_rotate_c: %f degrees about (%f, %f, %f)\n",
		angle, xcomp, ycomp, zcomp);
#endif

	/* 
	Do fall-throughs for most common cases (axis along a coordinate
	direction), or general case.
	*/
	if ( (xcomp==0.0) && (ycomp==0.0) && (zcomp==0.0) ) {
	  fprintf(stderr,
	     "make_rotate_c: can't make rotation about a zero-length axis.\n");
	  matrix = make_scale_c(1.0,1.0,1.0);
	}
	else if ( (ycomp==0.0) && (zcomp==0.0) && (xcomp>0.0) )
	        matrix = make_x_rotate_c(angle);
	else if ( (zcomp==0.0) && (xcomp==0.0) && (ycomp>0.0) )
	        matrix = make_y_rotate_c(angle);
	else if ( (xcomp==0.0) && (ycomp==0.0) && (zcomp>0.0) )
	        matrix = make_z_rotate_c(angle);
	else {
		norm= sqrt( xcomp*xcomp + ycomp*ycomp + zcomp*zcomp );
		matrix= make_arbitrary_rotate_c( angle, 
			xcomp/norm, ycomp/norm, zcomp/norm ); 
		};

	return( matrix );
}

/* This routine multiplies a vector by a matrix in C form. */
static float *matrix_vector_c(matrix, vector)
float *matrix, *vector;
{
	int i,j;
	float *result;

#ifdef DEBUG
	fprintf(stderr,"matrix_vector_c:\n");
#endif

	if ( !(result=(float *)malloc(4*sizeof(float))) ) {
		fprintf(stderr,"matrix_vector_c: can't allocate 4 floats!\n");
		exit(2);
	      }

	for (i=0; i<4; i++) {
		*(result+i)= 0.0;
		for (j=0; j<4; j++) {
			*(result+i)+= *(matrix+4*i+j) * *(vector+j);
			}
		}

	return( result );
}

/* This routine returns a rotation which will reverse the given
 * vector (as opposed to a simple inversion, which is not a rotation.
 */
static float *make_flip_c( vx, vy, vz )
float vx, vy, vz;
{
  float px= 0.0, py= 0.0, pz= 1.0, dot, cx, cy, cz, normsqr;

#ifdef DEBUG
  fprintf(stderr,"make_flip_c: flipping %f %f %f\n", vx, vy, vz);
#endif

  /* Find a vector not parallel to the given vector */
  dot= px*vx + py*vy + pz*vz;
  cx= py*vz - pz*vy;
  cy= pz*vx - px*vz;
  cz= px*vy - py*vx;
  if ( (cx*cx + cy*cy + cz*cz) < epsilon*dot*dot ) { /* this p won't work */
    px= 1.0; py= 0.0; pz= 0.0;
    dot= px*vx + py*vy + pz*vz;
    };

  /* Extract the normal component of that vector */
  normsqr= vx*vx + vy*vy + vz*vz;
  px= px - vx*dot/normsqr;
  py= py - vy*dot/normsqr;
  pz= pz - vz*dot/normsqr;

  /* Return a 180 degree rotation about that vector */
  return( make_rotate_c( 180.0, px, py, pz ) );
}

/* This routine returns a rotation which will rotate its first
 * parameter vector to align with its second parameter vector.
 */
static float *make_aligning_rotation( v1x, v1y, v1z, v2x, v2y, v2z )
float v1x, v1y, v1z, v2x, v2y, v2z;
{
  float ax, ay, az, dot, theta;

#ifdef DEBUG
  fprintf(stderr,"make_aligning_rotation: %f %f %f to %f %f %f\n",
	    v1x, v1y, v1z, v2x, v2y, v2z);
#endif

  ax= v1y*v2z - v1z*v2y;
  ay= v1z*v2x - v1x*v2z;
  az= v1x*v2y - v1y*v2x;
  dot= v1x*v2x + v1y*v2y + v1z*v2z;

  if ( (ax*ax + ay*ay + az*az) < epsilon*dot*dot ) { /* they already align */
    if (dot >= 0.0 ) /* parallel */
      return( make_translate_c( 0.0, 0.0, 0.0 ) );
    else /* anti-parallel */
      return( make_flip_c( v1x, v1y, v1z ) );
    }
  else {
    theta= acos( ( v1x*v2x+v1y*v2y+v1z*v2z ) / 
		( sqrt( v1x*v1x+v1y*v1y+v1z*v1z )
		 * sqrt( v2x*v2x+v2y*v2y+v2z*v2z ) ) );
    return( make_rotate_c( RadtoDeg*theta, ax, ay, az ) );
  }

  /* NOTREACHED */
}

static void mouse_delta( mouse, dx, dy )
MousePosition mouse;
float *dx, *dy;
/* 
This routine calculates mouse coordinates as floating point relative
to the viewport center 
*/
{
#ifdef DEBUG
  fprintf(stderr,"mouse_delta:\n");
#endif
  *dx= (float)( mouse.x - (mouse.maxx/2) )/(float)(mouse.maxx/2);
  *dy= (float)( mouse.y - (mouse.maxy/2) )/(float)(mouse.maxy/2);
}

static void mouse_to_3d( mouse, vx, vy, vz )
MousePosition mouse;
float *vx, *vy, *vz;
/* This routine translates mouse coords to 3D crystal ball coords */
{
  float x, y, radius_2d, radius_3d= CRYSTAL_BALL_RADIUS;

#ifdef DEBUG
  fprintf(stderr,"mouse_to_3d:\n");
#endif

  mouse_delta( mouse, &x, &y );
  radius_2d= sqrt( x*x + y*y );
  if (radius_2d > CRYSTAL_BALL_RADIUS) {
    x= x*CRYSTAL_BALL_RADIUS/radius_2d;
    y= y*CRYSTAL_BALL_RADIUS/radius_2d;
  }
  *vx= x;
  *vy= y;
  if ( radius_2d < CRYSTAL_BALL_RADIUS )  /* avoid round-off errors */
    *vz= sqrt( CRYSTAL_BALL_RADIUS*CRYSTAL_BALL_RADIUS-radius_2d*radius_2d );
  else *vz= 0.0;
}

static float *get_normal_component( v1x, v1y, v1z, v2x, v2y, v2z )
float v1x, v1y, v1z, v2x, v2y, v2z;
/* This routine returns a 4-vector holding the perpendicular component
 * of v1 with respect to v2, or (0, 1, 0, 1) if the two vectors are
 * parallel.
 */
{
  float *perpvec;
  float dot, normsqr;

  if ( !(perpvec= (float *)malloc( 4*sizeof(float) ) ) ) {
    fprintf(stderr,"get_normal_component: unable to allocate 4 floats!\n");
    exit(2);
  }

  dot= v1x*v2x + v1y*v2y + v1z*v2z;
  normsqr= v2x*v2x + v2y*v2y + v2z*v2z;

  perpvec[0]= v1x - v2x*dot/normsqr;
  perpvec[1]= v1y - v2y*dot/normsqr;
  perpvec[2]= v1z - v2z*dot/normsqr;
  perpvec[3]= 1.0;

  if ( (perpvec[0]==0.0) && (perpvec[1]==0.0) && (perpvec[2]==0.0) ) {
    fprintf(stderr,
	    "crystal_ball: get_normal_component: vectors align; using Y.\n");
    perpvec[1]= 1.0;
    }

  return( perpvec );
}

float *crystal_ball( viewmatrix, mousedown, mouseup )
float *viewmatrix;
MousePosition mousedown, mouseup;
/* 
This routine returns a transformation matrix appropriate for a crystal
ball rotation as specified by the given mouse motion.
*/
{
  float *rv1, *rv2, *result;
  float dx, dy, dz, upx, upy, upz;
  float v1[4], v2[4], inv_view[16];
  int i;

#ifdef DEBUG
  fprintf(stderr,"crystal_ball:\n");
#endif

  /* Create the inverse viewing transformation */
  for (i=0; i<16; i++) inv_view[i]= viewmatrix[i];
  (void)transpose( inv_view );

  /* Convert to 3D coords on crystal ball surface */
  mouse_to_3d( mousedown, &v1[0], &v1[1], &v1[2] );
  v1[3]= 1.0;
  mouse_to_3d( mouseup, &v2[0], &v2[1], &v2[2] );
  v2[3]= 1.0;

  /* Align vectors to world coordinates */
  rv1= matrix_vector_c( inv_view, v1 );
  rv2= matrix_vector_c( inv_view, v2 );

  /* Create transformation which aligns original radius vec with final one */
  result=
    make_aligning_rotation( rv1[0], rv1[1], rv1[2], rv2[0], rv2[1], rv2[2] );
  (void)transpose( result );

  /* clean up */
  free( (char *)rv1 );
  free( (char *)rv2 );

  return(result);
}

float *rots_to_matrix( rot_x, rot_y, rot_z )
float rot_x, rot_y, rot_z;
/* This routine converts three isoview-type rotation values to a 
 * rotation matrix.
 */
{
  float *mx, *my, *mz, *temp, *result;

  mx= make_rotate_c( rot_x, 1.0, 0.0, 0.0 );
  my= make_rotate_c( rot_y, 0.0, 1.0, 0.0 );
  mz= make_rotate_c( rot_z, 0.0, 0.0, 1.0 );

  temp= matrix_mult_c(mx, my);
  result= matrix_mult_c(mz, temp);

  free( mx );
  free( my );
  free( mz );
  free( temp );
  return( result );
}

static float angle_from_sin_cos( s, c )
float s, c;
/* This returns an angle given its sign and cosine, taking into account
 * the possible quadrants.  The result is in radians.
 */
{
  float result;

  result= asin( s );
  if (c<0) result= PI - result;
  return(result);
}

void matrix_to_rots( matrix, rot_x_ptr, rot_y_ptr, rot_z_ptr )
float *matrix;
float *rot_x_ptr, *rot_y_ptr, *rot_z_ptr;
/* This routine calculates isoview-type rotation values from a rotation 
 * matrix.  Note that the parameter values must be valid old values
 * on input, so the right root of the transformation equation can be
 * chosen.
 */
{
  float rot1_x, rot1_y, rot1_z;
  float rot2_x, rot2_y, rot2_z;
  float sx, cx, sy, cy, sz, cz;
  float t11, t12, t13, t21, t22, t23, t31, t32, t33;
  float old_rotx, old_roty, old_rotz;
  float dif, max_dif1, max_dif2;
  
  t11= *matrix;
  t21= *(matrix+1);
  t31= *(matrix+2);
  t12= *(matrix+4);
  t22= *(matrix+5);
  t32= *(matrix+6);
  t13= *(matrix+8);
  t23= *(matrix+9);
  t33= *(matrix+10);

  sx= -t32;
  cx= sqrt( t31*t31 + t33*t33 );

  if (cx==0.0) {
    fprintf(stderr,"cx == 0.0!; sx= %f\n",sx);
    return;
  }

  sz= t12/cx;
  cz= t22/cx;
  sy= -t31/cx;
  cy= t33/cx;

#ifdef DEBUG
  fprintf(stderr,"sx= %f, cx= +- %f\n",sx, cx);
  fprintf(stderr,"sy= +- %f, cy= +- %f\n",sy, cy);
  fprintf(stderr,"sz= +- %f, cz= +- %f\n",sz, cz);
#endif

  /* We take both roots of the set of equations */
  rot1_x= RadtoDeg * angle_from_sin_cos( sx, cx );
  rot1_y= RadtoDeg * angle_from_sin_cos( sy, cy );
  rot1_z= RadtoDeg * angle_from_sin_cos( sz, cz );
  rot2_x= RadtoDeg * angle_from_sin_cos( sx, -cx );
  rot2_y= RadtoDeg * angle_from_sin_cos( -sy, -cy );
  rot2_z= RadtoDeg * angle_from_sin_cos( -sz, -cz );

#ifdef DEBUG
  fprintf(stderr,"Solution 1: rot1_x= %f, rot1_y= %f, rot1_z= %f\n",
	  rot1_x, rot1_y, rot1_z);
  fprintf(stderr,"Solution 2: rot2_x= %f, rot2_y= %f, rot2_z= %f\n",
	  rot2_x, rot2_y, rot2_z);
#endif

  /* Pick the right root */
  old_rotx= *rot_x_ptr;
  old_roty= *rot_y_ptr;
  old_rotz= *rot_z_ptr;
  if (old_rotx > 180.0) old_rotx= old_rotx - 360.0;
  if (old_roty > 180.0) old_roty= old_roty - 360.0;
  if (old_rotz > 180.0) old_rotz= old_rotz - 360.0;
  max_dif1= fabs( rot1_x - old_rotx );
  if ( fabs(rot1_y - old_roty) > max_dif1 ) 
    max_dif1= fabs(rot1_y - old_roty);
  if ( fabs(rot1_z - old_rotz) > max_dif1 ) 
    max_dif1= fabs(rot1_z - old_rotz);

  max_dif2= fabs( rot2_x - old_rotx );
  if ( fabs(rot2_y - old_roty) > max_dif2 ) 
    max_dif2= fabs(rot2_y - old_roty);
  if ( fabs(rot2_z - old_rotz) > max_dif2 ) 
    max_dif2= fabs(rot2_z - old_rotz);

#ifdef DEBUG
  fprintf(stderr,"max_dif1= %f, max_dif2= %f\n",max_dif1, max_dif2);
#endif

  if (max_dif1 <= max_dif2) { /* pick root 1 */
#ifdef DEBUG
    fprintf(stderr,"Picked root 1\n");
#endif
    *rot_x_ptr= rot1_x;
    *rot_y_ptr= rot1_y;
    *rot_z_ptr= rot1_z;
  }
  else { /* pick root 2 */
#ifdef DEBUG
    fprintf(stderr,"Picked root 2\n");
#endif
    *rot_x_ptr= rot2_x;
    *rot_y_ptr= rot2_y;
    *rot_z_ptr= rot2_z;
  }
}

void matrices_to_rots( rot_matrix, view_matrix, 
		      rot_x_ptr, rot_y_ptr, rot_z_ptr)
float *rot_matrix, *view_matrix;
float *rot_x_ptr, *rot_y_ptr, *rot_z_ptr;
/* This routine calculates isoview rotation values from a view and
 * rotation matrix, first pre-multiplying the view matrix by the rotation
 * matrix.
 */
{
  float *temp;

  temp= matrix_mult_c( rot_matrix, view_matrix );
  matrix_to_rots( temp, rot_x_ptr, rot_y_ptr, rot_z_ptr );
  free( temp );
}
