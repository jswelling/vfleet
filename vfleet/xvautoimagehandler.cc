/****************************************************************************
 * xvautoimagehandler.cc
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
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdCmap.h>
#include "basenet.h"
#include "image.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "xvautoimagehandler_ui.h"

/* Notes-
 */

unsigned long XvautoImageHandler::WIN_IHANDLER_KEY= 0;
unsigned long XvautoImageHandler::WIN_AUTOHANDLER_KEY= 0;

static Notify_value image_window_destroy( Notify_client client,
					 Destroy_status status )
{
  if (status == DESTROY_CLEANUP) {
    XvautoImageHandler *autohandler= 
      (XvautoImageHandler *)xv_get(client, XV_KEY_DATA, 
				   XvautoImageHandler::WIN_AUTOHANDLER_KEY);
    delete autohandler;
    return( notify_next_destroy_func(client,status) );
  }
  return( NOTIFY_DONE );
}

XvautoImageHandler::XvautoImageHandler( rgbImage *image, char *name )
{
  current_image= image;

  // Make valid keys if that hasn\'t already been done
  if (!WIN_IHANDLER_KEY)
    WIN_IHANDLER_KEY= xv_unique_key();
  if (!WIN_AUTOHANDLER_KEY)
    WIN_AUTOHANDLER_KEY= xv_unique_key();

  ui_objects= new xvautoimagehandler_window1_objects;
  ui_objects->objects_initialize( NULL, name, 
				  image->xsize(), image->ysize() );

  notify_interpose_destroy_func( ui_objects->window1,
				 (Notify_value (*)())image_window_destroy );

  xv_set( ui_objects->window1, WIN_SHOW, TRUE, NULL );

  Window canvas_win= (Window)xv_get(canvas_paint_window(ui_objects->canvas1), 
				    XV_XID);
  Window top_win= (Window)xv_get(ui_objects->window1, XV_XID);
  Display *dpy= (Display *)xv_get( ui_objects->canvas1, XV_DISPLAY );
  ihandler= 
    new XImageHandler( dpy, 
		       (int)xv_get( (Xv_Screen)xv_get( ui_objects->canvas1, 
						       XV_SCREEN ),
				    SCREEN_NUMBER ),
		       canvas_win );
  xv_set( ui_objects->canvas1, XV_KEY_DATA, WIN_IHANDLER_KEY, ihandler );
  xv_set( ui_objects->window1, XV_KEY_DATA, WIN_IHANDLER_KEY, ihandler );
  xv_set( ui_objects->canvas1, XV_KEY_DATA, WIN_AUTOHANDLER_KEY, this );
  xv_set( ui_objects->window1, XV_KEY_DATA, WIN_AUTOHANDLER_KEY, this );
  ihandler->display( image );

  // Set the top window property so that the window manager pays some
  // attention to the canvas color map.
  Atom atomcolormapwindows= XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
  XChangeProperty( dpy, top_win, atomcolormapwindows, XA_WINDOW, 32, 
		   PropModeAppend, (unsigned char *)&canvas_win, 1 );

}

XvautoImageHandler::~XvautoImageHandler()
{
  delete ihandler;
  delete current_image;
  delete ui_objects;
}

void XvautoImageHandler::display( rgbImage *image, short refresh )
{
  ihandler->display(image, refresh);
}

//
// Repaint callback function for `canvas1'.
//
void
xvautoimagehandler_repaint(Canvas canvas, Xv_window paint_window, Rectlist *rects)
{
	XImageHandler *ihandler= 
	  (XImageHandler *)xv_get(canvas, XV_KEY_DATA, 
				  XvautoImageHandler::WIN_IHANDLER_KEY);

	ihandler->redraw();

	// gxv_start_connections DO NOT EDIT THIS SECTION

	// gxv_end_connections

}

//
// Resize callback function for `canvas1'.
//
void
xvautoimagehandler_resize(Canvas canvas, int width, int height)
{
	XImageHandler *ihandler= 
	  (XImageHandler *)xv_get(canvas, XV_KEY_DATA, 
				  XvautoImageHandler::WIN_IHANDLER_KEY);

	ihandler->resize();
	
	// gxv_start_connections DO NOT EDIT THIS SECTION

	// gxv_end_connections

}

//
// Initialize an instance of object `window1'.
//
void
xvautoimagehandler_window1_objects::objects_initialize(Xv_opaque owner,
						       char *title,
						       int xdim, int ydim)
{
	window1 = window1_create(owner, title, xdim, ydim);
	canvas1 = canvas1_create(window1);
}

//
// Create object `window1' in the specified instance.
//
Xv_opaque
xvautoimagehandler_window1_objects::window1_create(Xv_opaque owner,
						   char *title, 
						   int xdim, int ydim)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME,
		XV_KEY_DATA, INSTANCE, this,
		XV_WIDTH, xdim,
		XV_HEIGHT, ydim,
		XV_LABEL, title,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		NULL);
	return obj;
}

//
// Create object `canvas1' in the specified instance.
//
Xv_opaque
xvautoimagehandler_window1_objects::canvas1_create(Xv_opaque owner)
{
	extern void	xvautoimagehandler_repaint(Canvas, Xv_window, Rectlist *);
	extern void	xvautoimagehandler_resize(Canvas, int, int);
	Xv_opaque	obj;
	
	obj = xv_create(owner, CANVAS,
		XV_KEY_DATA, INSTANCE, this,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		CANVAS_REPAINT_PROC, xvautoimagehandler_repaint,
		CANVAS_RESIZE_PROC, xvautoimagehandler_resize,
		NULL);
	//
	// This line is here for backwards compatibility. It will be
	// removed for the next release.
	//
	xv_set(canvas_paint_window(obj), XV_KEY_DATA, INSTANCE, this, NULL);
	return obj;
}

