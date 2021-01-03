/****************************************************************************
 * xmautoimagehandler.cc for pvm 2.4
 * Author Robert Earhart
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

/*xmautoimagehandler.cc

  implements the XmautoImageHandler class,
  which allows one to send rgbImages to a window.

  Interfaces to Motif properly.

  Written February 1993, Rob Earhart <earhart@psc.edu>
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

static Cursor watch=0;
	
XmautoImageHandler::~XmautoImageHandler() 
{
  if (does_top)
    XtDestroyWidget(draw_parent);
  else
    XtDestroyWidget(draw_me);
}

Boolean XmautoImageHandler::check() 
{
  if (the_image != NULL) /*Is it created?*/
    return(True);
  else {
    if (current_image && XtIsRealized(draw_me)) /*Can we create it?*/ { 
      the_image = new XImageHandler(XtDisplay(draw_me),
				    0,
				    XtWindow(draw_me));
      if (the_image) {
	if (! watch)
	  watch=XCreateFontCursor(XtDisplay(draw_parent), XC_watch);
	XDefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent), watch);
	the_image->display(current_image, False);
	XUndefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent));
	return(True);
      } else
	return(False);
    } else
      return(False);
  }
}

static void redraw_meCB(Widget w, XmautoImageHandler *client_data,
			caddr_t call_data)

/*Sends a redraw message to it\'s image.*/
{
  client_data->redraw();
}

static void resize_meCB(Widget w, XmautoImageHandler *client_data,
			caddr_t call_data)

/*Sends a resize message to it\'s image.*/
{
  client_data->resize();
}

void XmautoImageHandler::redraw() 
{
  if (does_top && !redrawn_before) {
    Window canvas_win = XtWindow(draw_me);
    XChangeProperty(XtDisplay(draw_me), canvas_win, 
		    atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		    (unsigned char *)&canvas_win, 1 );
    redrawn_before = 0;
  }
  if (check()) {
    if (! watch)
      watch=XCreateFontCursor(XtDisplay(draw_parent), XC_watch);
    XDefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent), watch);
    the_image->redraw();
    XUndefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent));
  }
}

void XmautoImageHandler::resize() 
{
  if (check()) {
    if (! watch)
      watch=XCreateFontCursor(XtDisplay(draw_parent), XC_watch);
    XDefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent),
		  watch);
    the_image->resize();
    XUndefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent));
  }
}

XmautoImageHandler::XmautoImageHandler(Widget parent,
				       int height,
				       int width,
				       Widget base_parent,
				       Arg *top_args,
				       int top_n,
				       rgbImage *initial_image,
				       FILE *the_file) 
{
  Arg args[6];
  register int n;
  if (does_top = !base_parent) {
    Widget temp;

    draw_parent=XmCreateFormDialog(parent, "xmautoimagehandlerW",
				   top_args, top_n);
    n=0;
    XtSetArg(args[n], XtNwidth, width); n++;
    XtSetArg(args[n], XtNheight, height); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    draw_me= XmCreateDrawingArea(draw_parent, "xmautoimagehandler",
				 args, n);
    while (temp = XtParent(parent)) parent = temp;
    redrawn_before = 0;
    atomcolormapwindows= 
      XInternAtom(XtDisplay(parent), "WM_COLORMAP_WINDOWS", False);
  } else {
    draw_parent=base_parent;
    draw_me= XmCreateDrawingArea(draw_parent, "xmautoimagehandler",
				 top_args, top_n);
  }
  XtManageChild(draw_me);
  XtAddCallback(draw_me, XmNresizeCallback,
		(XtCallbackProc) resize_meCB,
		(XtPointer) this);
  XtAddCallback(draw_me, XmNexposeCallback,
		(XtCallbackProc) redraw_meCB,
		(XtPointer) this);
  if (does_top)
    XtManageChild(draw_parent);
  current_image=initial_image;
  the_image=NULL;
  fp = the_file;
}

void XmautoImageHandler::display( rgbImage *image, short refresh) 
{
  current_image= image;
  if (check()) {
    if (! watch)
      watch=XCreateFontCursor(XtDisplay(draw_parent), XC_watch);
    XDefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent), watch);
    the_image->display(image, refresh);
    XUndefineCursor(XtDisplay(draw_parent), XtWindow(draw_parent));
  }
}
