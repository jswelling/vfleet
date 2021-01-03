/****************************************************************************
 * xmautodrawih.cc
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

/* implements the XmautodrawImageHandler class
   provides a motif interface to the XDrawImageHandler class
   or adds drawing functions to the XmautoImageHandler class,
   depending upon how you look at it
   Now with a shiny new C interface
   */

#include <stdio.h>
#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/MwmUtil.h>
#include <X11/cursorfont.h>

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xext.h>
#endif /* SHM */
#endif /* LOCAL */

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "xdrawih.h"

static void redraw_meCB(Widget w, XmautodrawImageHandler *client_data,
			caddr_t call_data)

/*Sends a redraw message to it\'s image.*/
{
  client_data->redraw();
}

static void resize_meCB(Widget w, XmautodrawImageHandler *client_data,
			caddr_t call_data)

/*Sends a resize message to it\'s image.*/
{
  client_data->resize();
}

XmautodrawImageHandler::XmautodrawImageHandler(Widget parent,
					       int height,
					       int width,
					       Widget base_parent,
					       Arg *top_args,
					       int top_n,
					       rgbImage *initial_image) 
: XmautoImageHandler (parent, height, width, base_parent, 
		      top_args, top_n, initial_image, NULL)
{
  pixheight = height;
  pixwidth = width;
}

XmautodrawImageHandler::~XmautodrawImageHandler() 
{
}

void XmautodrawImageHandler::winsize(int *height, int *width)
{
  if (check())
    ((XDrawImageHandler*) the_image)->winsize(height, width);
}

void XmautodrawImageHandler::clear(int r, int g, int b)
{
  if (check()) 
    ((XDrawImageHandler*) the_image)->clear(r, g, b);
}

void XmautodrawImageHandler::ppoint(int r, int g, int b, 
				    int pnum, XPoint *points)
{
  if (check())
    ((XDrawImageHandler*) the_image)->ppoint(r, g, b, pnum, points);
}

void XmautodrawImageHandler::pline(int r, int g, int b, 
				   int pnum, XPoint *points)
{
  if (check())
    ((XDrawImageHandler*) the_image)->pline(r, g, b, pnum, points);
}

void XmautodrawImageHandler::pgon(int r, int g, int b, 
				  int pnum, XPoint *points)
{
  if (check())
    ((XDrawImageHandler*) the_image)->pgon(r, g, b, pnum, points);
}

Boolean XmautodrawImageHandler::check() 
{
  if (the_image != NULL) /*Is it created?*/
    return(True);

  if (XtIsRealized(draw_me)) {
    XSync(XtDisplay(draw_me), 0);
    the_image = 
      new XDrawImageHandler(XtDisplay(draw_me), 0, XtWindow(draw_me),
			    pixheight, pixwidth);
    if (the_image) return(True);
    else return(False);
  }

  return(False); /* draw_me wasn't realized - couldn't create the_image */
}

/* C interface routines */

void *xmadih_create(Widget parent, int height, int width)
{
  return new XmautodrawImageHandler(parent, height, width,
				    NULL, NULL, 0, NULL);
}

void xmadih_delete(void *xmadih)
{
  delete (XmautodrawImageHandler*)xmadih;
}

void xmadih_ppoint(void *xmadih, int r, int g, int b, int pnum, XPoint *points)
{
  ((XmautodrawImageHandler*)xmadih)->ppoint(r, g, b, pnum, points);
}

void xmadih_pline(void *xmadih, int r, int g, int b, int pnum, XPoint *points)
{
  ((XmautodrawImageHandler*)xmadih)->pline(r, g, b, pnum, points);
}

void xmadih_pgon(void *xmadih, int r, int g, int b, int pnum, XPoint *points)
{
  ((XmautodrawImageHandler*)xmadih)->pgon(r, g, b, pnum, points);
}

void xmadih_winsize(void *xmadih, int *height, int *width)
{
  ((XmautodrawImageHandler*)xmadih)->winsize(height, width);
}

void xmadih_redraw(void *xmadih)
{
  ((XmautodrawImageHandler*)xmadih)->redraw();
}

void xmadih_clear(void *xmadih, int r, int g, int b)
{
  ((XmautodrawImageHandler*)xmadih)->clear(r, g, b);
}

Window xmadih_window( void* xmadih )
{
  return ((XmautodrawImageHandler*)xmadih)->get_win();
}

Widget xmadih_widget( void* xmadih )
{
  return ((XmautodrawImageHandler*)xmadih)->widget();
}
