/****************************************************************************
 * canvases.cc
 * Author Daniel Martinez
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"

//
// To use this from a UIL file, do something like:
//
//
// procedure CreateCanvas();  (to make the proc known to UIL)
// ...
// object canvas : user_defined procedure CreateCanvas {
//   arguments { ...etc... };
//   callbacks { ...etc... };
// };
//
// And from the cc side, include:
// MrmRegisterClass(0, NULL, "CreateCanvas", CreateCanvas, NULL);
//
// Widgets thus created will respond properly to
// XImageHandler* ihandler= AttachImageHandler(widget);
//


#ifdef IRIS_GL_IMAGES

#include <Sgm/GlxMDraw.h>

#include "glximagehandler.h"

// The desired configuration for GL.

static GLXconfig glx_params [] = {
    { GLX_NORMAL, GLX_DOUBLE, FALSE },
    { GLX_NORMAL, GLX_RGB, TRUE },
    { NULL, NULL, NULL }
};


// At creation time, we give the glxMDrawingArea a list of callbacks
// which are to be called when it is actually instantiated. Before we
// can create this list, we have to declare any functions which it
// might refer to.

static void glx_init_cb (Widget w, XtPointer client_data,
			 GlxDrawCallbackStruct *call_data);

static XtCallbackRec glxInitCallbacks[] = {
  { (XtCallbackProc) glx_init_cb, (XtPointer) NULL },
  { (XtCallbackProc) NULL, (XtPointer) NULL },
};

#endif // IRIS_GL_IMAGES


// CreateCanvas is called whenever we need a DrawingArea widget. If
// IRIS_GL_IMAGES was defined at compile time, CreateCanvas examines the
// system it's running on, and creates a GlxMDrawingArea if it can.
// Otherwise, it creates an ordinary Motif DrawingArea.

// If IRIS_GL_IMAGES was not defined at compile time, CreateCanvas creates
// the Motif DrawingArea, even if it's running on a GL-capable system.
// The caller is returned a widget; the caller must then determine the
// Widget's class (using XtClass or a related function) to determine
// how to draw into that widget.

Widget CreateCanvas (Widget parent, String name,
		     ArgList arglist, Cardinal argcount, ...)
{
  Widget retVal;
#ifdef IRIS_GL_IMAGES
  GLXconfig *retConfig = NULL;
  Arg glxArgs[2];
  int n;
  ArgList jointArgs;
  
  retConfig = GLXgetconfig(XtDisplay(parent),
			   XScreenNumberOfScreen(XtScreen(parent)),
			   glx_params);
  if (retConfig) {
    free(retConfig);

    n = 0;
    XtSetArg(glxArgs[n], GlxNglxConfig, glx_params); n++;
    XtSetArg(glxArgs[n], GlxNginitCallback, glxInitCallbacks); n++;

    jointArgs = XtMergeArgLists(arglist, argcount, glxArgs, n);
    retVal = GlxCreateMDraw(parent, name, jointArgs, argcount + n);

    XtFree((char *)jointArgs);
    
  } else
#endif // IRIS_GL_IMAGES
    retVal = XmCreateDrawingArea(parent,name,arglist,argcount);

  return retVal;
}


// AttachImageHandler determines the class of the widget which it's
// given, and attaches the appropriate ImageHandler. Currently, this
// means attaching a GLXImageHandler if the widget is a GlxMDrawArea,
// and an XImageHandler otherwise.

// As with CreateCanvas above, if IRIS_GL_IMAGES is not defined at compile
// time, the function defaults to plain-vanilla Motif and Xlib
// behavior.

XImageHandler *AttachImageHandler(Widget w)
{
  XImageHandler *retVal;
  
#ifdef IRIS_GL_IMAGES
  if (XtClass(w) == glxMDrawWidgetClass) {
    retVal = new GLXImageHandler(XtDisplay(w),
				 DefaultScreen(XtDisplay(w)),
				 XtWindow(w));
  } else
#endif // IRIS_GL_IMAGES
    if (XtClass(w) == xmDrawingAreaWidgetClass) {
      retVal = new XImageHandler(XtDisplay(w),
				 DefaultScreen(XtDisplay(w)),
				 XtWindow(w));
    } else {
      fprintf(stderr, "Unable to create a suitable ImageHandler!\n");
    }
  return retVal;
}



#ifdef IRIS_GL_IMAGES

// glx_init_cb is a callback, used only for GlxMDraw widgets and
// called at initialization time. Its attachment is taken care of in
// CreateCanvas.

// GlxMDraw areas have a fairly spartan set of default behaviors
// relative to XmDrawingAreas: for one thing, they don't automatically
// repaint their backgrounds. It's therefore our responsibility to get
// the widget's background color, in preparation for later redrawing
// the background when necessary.


static void glx_init_cb (Widget w, XtPointer client_data,
			 GlxDrawCallbackStruct *call_data)
{
  Arg childArg, parentArg;
  Display *dpy;
  Colormap cmap;
  Pixel bgIndex;
  XColor bgRGB;

  // A GlxMDrawingArea widget's background colors are specified
  // relative to its parent's colormap. Ergo, in order to set the
  // background color properly, we must query the widget for its
  // background color index, query the parent for its colormap, and
  // then use the two together to obtain an actual RGB triple.
  // Tedious, but straightforward.

  XtSetArg(childArg, XmNbackground, &bgIndex);
  XtSetArg(parentArg, XmNcolormap, &cmap);
  
  XtGetValues(w, &childArg, 1);
  XtGetValues(XtParent(w), &parentArg, 1);

  bgRGB.pixel = bgIndex;
  bgRGB.flags = (DoRed | DoGreen | DoBlue);

  dpy = XtDisplay(w);
  XQueryColor(dpy, cmap, &bgRGB);

  GLXwinset(dpy, call_data->window);

  RGBcolor (bgRGB.red >> 8, bgRGB.green >> 8, bgRGB.blue >> 8);
  
  screenspace();
  viewport(0, (Screencoord) call_data->width-1,
	   0, (Screencoord) call_data->height-1);
  clear();
}

#endif // IRIS_GL_IMAGES
