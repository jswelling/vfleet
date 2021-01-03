/****************************************************************************
 * xcball.c
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
/*
This class provides a 'crystal ball' type 3D motion interface with an X
input window.
*/

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "geometry.h"
#include "cball.h"
#include "xcball.h"

XmCBall::XmCBall( Widget widget_in, 
		  void (*motion_callback_in)(gTransfm *, void *, CBall *),
		  void *cb_info_in ) 
: CBall( motion_callback_in, cb_info_in )
{
  widget= widget_in;
  XtAddEventHandler(widget, ButtonPressMask | ButtonReleaseMask, FALSE,
		    (XtEventHandler)window_input_callback, this);
}

XmCBall::~XmCBall()
{
  XtRemoveEventHandler(widget, ButtonPressMask | ButtonReleaseMask, FALSE,
		    (XtEventHandler)window_input_callback, this);
}

void XmCBall::window_input_callback( Widget w, XmCBall *cball, XEvent *event )
{
  static MousePosition drag_start, drag_end;
  static int drag_start_valid= 0;

  XWindowAttributes window_attributes;

  if ( XGetWindowAttributes( XtDisplay(w), XtWindow(w),
			     &window_attributes ) ) {
    switch (event->type) {
    case ButtonPress: 
      {
	if (event->xbutton.button == Button1) {
	  drag_start.maxx = window_attributes.width;
	  drag_start.maxy = window_attributes.height;
	  drag_start.x = event->xbutton.x;
	  drag_start.y = window_attributes.height - event->xbutton.y;
	  drag_start_valid= 1;
	}
      }
      break;
    case ButtonRelease:
      {
	if (event->xbutton.button == Button1) {
	  drag_end.maxx = window_attributes.width;
	  drag_end.maxy = window_attributes.height;
	  drag_end.x = event->xbutton.x;
	  drag_end.y = window_attributes.height - event->xbutton.y;
	  if (drag_start_valid) cball->move( drag_start, drag_end );
	  drag_start_valid= 0;
	}
      }
      break;
    }
  }
  else fprintf(stderr,
	       "XmCBall::window_input_callback: unable to get window size");

}
