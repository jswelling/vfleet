/****************************************************************************
 * xdrawih.cc
 * Author Joe Demers
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

/* Adds drawing functions to the basic ximagehandler class
   (polygons, polylines, and polymarkers)
   also function for clearing screen to desired color
   and creating a new pixmap if the window size has changed
   Lastly, now has C interface routines
   */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdCmap.h>

#ifndef AVOID_XVIEW
#include <xview/xview.h>
#endif

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
/*#include <sys/shm.h>*/
#include <X11/extensions/xshm.h>
#endif /* SHM */
#endif /* LOCAL */

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "xdrawih.h"

XDrawImageHandler::XDrawImageHandler (Display *d_in, int s_in, Window w_in,
				      int height, int width)
: XImageHandler (d_in, s_in, w_in)
{
  pixheight = height;
  pixwidth = width;

  current_pixmap = XCreatePixmap(dpy, (Drawable)attrib.root, 
				 pixwidth, pixheight, attrib.depth);
  if (!inited) 
    (this->*init_fun)();

  clear(0, 0, 0);
}

XDrawImageHandler::~XDrawImageHandler()
{
  if (current_pixmap) XFreePixmap(dpy, current_pixmap);
  current_pixmap = 0;
}

void XDrawImageHandler::winsize(int *height, int *width)
{
  get_window_size();

  *height = win_y_size;
  *width = win_x_size;
}

/* if the window size has changed since the last clear or construction,
   frees the old pixmap and creates a new one of the proper size
   then clears the pixmap by drawing a rectangle into it of the specified color
   */
void XDrawImageHandler::clear(int r, int g, int b)
{
  get_window_size();

  if ((win_x_size != pixwidth) || (win_y_size != pixwidth)) {
    if (current_pixmap) XFreePixmap(dpy, current_pixmap);
    current_pixmap = XCreatePixmap(dpy, (Drawable)attrib.root, 
				   win_x_size, win_y_size, attrib.depth);
    pixwidth = win_x_size;
    pixheight = win_y_size;
  }
  if (attrib.depth != 1) // if not monochrome
    XSetForeground(dpy, gc, calc_pixel(r,g,b));
  else XSetForeground(dpy, gc, BlackPixel(dpy, screen));
  XFillRectangle(dpy, current_pixmap, gc, 0, 0, win_x_size, win_y_size);
  XSetForeground(dpy, gc, WhitePixel(dpy, screen));
} 

void XDrawImageHandler::ppoint(int r, int g, int b, int pnum, XPoint *points) 
{
  int i;

  if (attrib.depth != 1) // if not monochrome
    XSetForeground(dpy, gc, calc_pixel(r,g,b));
  for (i=0; i<pnum; i++) 
    XDrawPoint(dpy, current_pixmap, gc, points[i].x, points[i].y);
}

void XDrawImageHandler::pline(int r, int g, int b, int pnum, XPoint *points) 
{
  if (attrib.depth != 1) // if not monochrome
    XSetForeground(dpy, gc, calc_pixel(r,g,b));
  XDrawLines(dpy, current_pixmap, gc, points, pnum, CoordModeOrigin);
}

void XDrawImageHandler::pgon(int r, int g, int b, int vnum, XPoint *points) 
{
  if (attrib.depth != 1) // if not monochrome
    XSetForeground(dpy, gc, calc_pixel(r,g,b));
  XFillPolygon(dpy, current_pixmap, gc, 
	       points, vnum, Complex, CoordModeOrigin);
}

/* C interface routines */

void *xdrawih_create(Display *display, int screen, Window win, 
		     int width, int height)
{
  return new XDrawImageHandler(display, screen, win, width, height);
}

void xdrawih_delete(void *xdrawih)
{
  delete (XDrawImageHandler*)xdrawih;
}

void xdrawih_ppoint(void *xdrawih, int r, int g, int b, 
		    int pnum, XPoint *points)
{
  ((XDrawImageHandler*)xdrawih)->ppoint(r, g, b, pnum, points);
}

void xdrawih_pline(void *xdrawih, int r, int g, int b, 
		   int pnum, XPoint *points)
{
  ((XDrawImageHandler*)xdrawih)->pline(r, g, b, pnum, points);
}

void xdrawih_pgon(void *xdrawih, int r, int g, int b, 
		  int pnum, XPoint *points)
{
  ((XDrawImageHandler*)xdrawih)->pgon(r, g, b, pnum, points);
}

void xdrawih_redraw(void *xdrawih)
{
  ((XDrawImageHandler*)xdrawih)->redraw();
}

void xdrawih_winsize(void *xdrawih, int *height, int *width)
{
  ((XDrawImageHandler*)xdrawih)->winsize(height, width);
}  

void xdrawih_clear(void *xdrawih, int r, int g, int b)
{
  ((XDrawImageHandler*)xdrawih)->clear(r, g, b);
}

Window xdrawih_window( void *xdrawih )
{
  return ((XDrawImageHandler*)xdrawih)->get_win();
}
