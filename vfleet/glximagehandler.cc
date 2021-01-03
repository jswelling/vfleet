/**********************************************************************
 * glximagehandler.cc for pvm 2.4
 * Author Joel Welling, Rob Earhart
 *
 * Copyright 1993, Pittsburgh Supercomputing Center,
 * Carnegie Mellon University
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
 **********************************************************************/

#ifdef SGI_MIPS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gl/glws.h>

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "glximagehandler.h"


GLXImageHandler::GLXImageHandler(Display *dpy_in, int screen_in,
				 Window win_in)
  : XImageHandler (dpy_in, screen_in, win_in)
{
  current_lrect = NULL;

  init_fun = &GLXImageHandler::glx_ih_init;
  display_fun = &GLXImageHandler::glx_ih_display;
}


GLXImageHandler::~GLXImageHandler()
{
  if (current_lrect)
    free (current_lrect);
}


void GLXImageHandler::display(rgbImage *image, short refresh)
{
  if (image->compressed()) image->uncompress();
  current_image = image;
  
  if (current_lrect) {
    if ((image_x_size != current_image->xsize()) 
	|| (image_y_size != current_image->ysize())) {
      // current_lrect is wrong size;  replace it
      free( current_lrect );
      image_x_size = current_image->xsize();
      image_y_size = current_image->ysize();
      current_lrect = (unsigned long *) malloc(image_x_size * image_y_size *
					       sizeof (long));
    }
  }
  else {
    image_x_size = current_image->xsize();
    image_y_size = current_image->ysize();
  
    current_lrect = (unsigned long *) malloc(image_x_size * image_y_size *
					     sizeof (long));
  }

  if (!XGetWindowAttributes(dpy,win,&attrib)) {
    fprintf(stderr,
	    "XImageHandler::display: Can't get attributes!\n");
    exit(-1);
  }

  if (! inited)
    (this->*init_fun)();

  (this->*display_fun)(current_lrect);

  XFlush(dpy);

  if (refresh && (attrib.map_state == IsViewable))
    redraw();
}


void GLXImageHandler::redraw()
{
  int bottom, right;
  
  get_window_size();
  
  long retval;

  retval = GLXwinset (dpy, win);

  if (retval < 0) {
    fprintf(stderr, "GLXWinset returned %ld\n", retval);
    exit(-1);
  }

  if (inited) {

    // Draw the background below the image, if necessary.
    
    bottom = win_y_size - image_y_size;

    if (bottom > 0) {
      viewport(0, (Screencoord) win_x_size - 1,
	       0, (Screencoord) bottom - 1);
      clear ();
    }

    // Draw the background to the right of the image, if necessary.
    
    if (win_x_size > image_x_size) {
      viewport((Screencoord) image_x_size, (Screencoord) win_x_size - 1,
	       (Screencoord) bottom, (Screencoord) win_y_size - 1);
      clear();
    }
    
    viewport(0, (Screencoord) win_x_size - 1,
	     0, (Screencoord) win_y_size - 1);

    lrectwrite (0, bottom, image_x_size - 1, image_y_size - 1 + bottom,
		current_lrect);
  } else {
    viewport(0, (Screencoord) win_x_size - 1,
	     0, (Screencoord) win_y_size - 1);
    clear ();
  }
  gflush();
}


void GLXImageHandler::resize()
{
  /* This routine handles resize events on the window.
   * Of course, all this really boils down to is generating
   * a redraw request if the window grew.*/

  unsigned int old_win_x_size=win_x_size;
  unsigned int old_win_y_size=win_y_size;

  get_window_size();

  // The XImageHandler only redraws if one of the window's dimensions
  // has increased AND the XImageHandler has been initialized. The
  // GlxImageHandler is slightly different, because GlxMDraw widgets
  // don't automatically update their backgrounds: we have to redraw
  // WHENEVER a window dimension increases.

  if ((win_x_size > old_win_x_size) || (win_y_size > old_win_y_size))
    redraw();
}


void GLXImageHandler::glx_ih_display(unsigned long *image_in)
{
  // This is quite a bit simpler than the corresponding X code, but
  // then SGI uses a single, standard layout (ABGR) for video memory.
  // Such, perhaps, are the perqs of using a machine that costs more
  // than most family sedans.

  int xlim, ylim;
  register int i,j;
  unsigned long *index;

  xlim = image_x_size;
  ylim = image_y_size;

  index = image_in;
  for (j=ylim-1; j >= 0; j--)
    for (i=0; i < xlim; i++) {
      unsigned long pixel = current_image->pix_r(i,j) |
	current_image->pix_g(i,j) << 8 |
	current_image->pix_b(i,j) << 16 |
	current_image->pix_a(i,j) << 24;
      *(index++) = pixel;
    }
}


void GLXImageHandler::glx_ih_init()	
{
  inited = True;
}

#endif
