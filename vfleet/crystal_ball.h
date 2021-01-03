/****************************************************************************
 * crystal_ball.h
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
This module provides entry points and structs for the 'mouse user interface'.
*/

#define PI 3.14159265
#define DegtoRad PI/180.0 
#define RadtoDeg 180.0/PI

/* Struct to hold mouse position information */
typedef struct mouse_pos_struct { int x, y, maxx, maxy; } MousePosition;

/* Routine to return a crystal ball transformation matrix */
float *crystal_ball();
float *rots_to_matrix();
void matrix_to_rots();
void matrices_to_rots();
