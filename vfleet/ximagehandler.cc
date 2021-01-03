/****************************************************************************
 * ximagehandler.cc for pvm 2.4
 * Author Joel Welling, Rob Earhart
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
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdCmap.h>

#ifdef never
#ifdef DEC_ALPHA
extern "C" long random();
extern "C" int srandom( unsigned seed );
#endif
#endif

#ifdef SHARE_XWS_CMAP
extern XStandardColormap* xws_best_rgb_stdmap;
#endif

#ifndef AVOID_XVIEW
#include <xview/xview.h>
#endif

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
/*#include <sys/shm.h>*/
#include <X11/extensions/XShm.h>
#endif /* SHM */
#endif /* LOCAL */

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"

/* Notes-
 */

XStandardColormap XImageHandler::map_info;

int XImageHandler::map_info_set= 0;

/*This is needed by a lot of things that use the class...*/

/*Note that the value returned by this ought to be deleted,
  if not null.
*/

XVisualInfo *get_best_visual(Display *dpy, int screen) {
    /*This should query the dpy for the best visual, and set depth
      and visual appropriately.*/

    XVisualInfo *vinfo = new XVisualInfo;
    
    /*Okay, try for 24 bit TrueColor...*/
    if (XMatchVisualInfo(dpy, screen, 24, TrueColor, vinfo))
	return(vinfo);
    
    /*First go for a 24 bit DirectColor (what I'm using as I write this :)*/
    if (XMatchVisualInfo(dpy, screen, 24, DirectColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 12 bit DirectColor...*/
    if (XMatchVisualInfo(dpy, screen, 12, DirectColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 12 bit TrueColor...*/
    if (XMatchVisualInfo(dpy, screen, 12, TrueColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 12 bit PseudoColor...*/
    if (XMatchVisualInfo(dpy, screen, 12, TrueColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 8 bit PseudoColor...*/
    if (XMatchVisualInfo(dpy, screen, 8, PseudoColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 8 bit DirectColor...*/
    if (XMatchVisualInfo(dpy, screen, 8, DirectColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 4 bit PseudoColor...*/
    if (XMatchVisualInfo(dpy, screen, 4, PseudoColor, vinfo))
	return(vinfo);
    
    /*Okay, try for 2 bit PseudoColor...*/
    if (XMatchVisualInfo(dpy, screen, 2, PseudoColor, vinfo))
	return(vinfo);

    /*If almost all fails, go for monochrome*/
    if (XMatchVisualInfo(dpy, screen, 2, StaticGray, vinfo))
	return(vinfo);

    /*If everything else fails, let the caller pick. */

    delete(vinfo);
 
    return(NULL);
}

XImageHandler::XImageHandler( Display *dpy_in, int screen_in, Window win_in)
: baseImageHandler()
{
  unsigned long valuemask;
  XGCValues xgcv;

  dpy= dpy_in;
  screen= screen_in;
  win= win_in;

  current_pixmap = 0;
  
  inited=False;

  valuemask= (GCForeground | GCBackground);
  xgcv.background= BlackPixel(dpy, screen);
  xgcv.foreground= WhitePixel(dpy, screen);
  gc= XCreateGC(dpy, win, valuemask, &xgcv);

#ifdef LOCAL
  using_local = False;
#else /* LOCAL */
#ifdef SHM
  using_shm = False;
#endif /* SHM */
#endif /* LCOAL */

  get_window_size();
  
  /* Determine the color capabilities of the device*/

  /*Since we're passed a window, use the window's visual and depth.*/
  XGetWindowAttributes(dpy, win, &attrib);

  // Check for monochrome case
  if (attrib.depth == 1) {
    init_fun = &XImageHandler::monochrome_init;
    display_fun= &XImageHandler::monochrome_display;
    return;
  }
  
  // Handle all other cases
  switch ((attrib.visual)->c_class) {
  case (int)TrueColor:
  case (int)DirectColor:
    init_fun = &XImageHandler::directcolor_init;
    display_fun= &XImageHandler::directcolor_display;
    break;
  case (int)PseudoColor:
    init_fun = &XImageHandler::pseudocolor_init;
    display_fun= &XImageHandler::pseudocolor_display;
    break;
  case (int)StaticColor:
    fprintf(stderr,
    "XImageHandler: Sorry, no color support for StaticColor visual type\n");
    exit(-1);
    break;
  case (int)GrayScale:
    fprintf(stderr,
   "XImageHandler: Sorry, no color support for GrayScale visual type\n");
    exit(-1);
    break;
  case (int)StaticGray:
    fprintf(stderr,
   "XImageHandler: Sorry, no color support for StaticGray visual type\n");
    exit(-1);
    break;
  };
}

XImageHandler::~XImageHandler()
{
#ifdef LOCAL
    if (using_local) {
	XDestroyImage(ximage);
    } else
#else /* LOCAL */
#ifdef SHM
    if (using_shm) {
#ifndef SHM_ONE
	XShmDetach(dpy, &shminfo);
#endif /* SHM_ONE */
	XDestroyImage(ximage);
#ifndef SHM_ONE	
	shmdt(shminfo.shmaddr);
#endif /* SHM_ONE */	
     shmctl(shminfo.shmid, IPC_RMID, 0);
    } else
#endif /* SHM */
#endif /* LOCAL */

    if (current_pixmap) XFreePixmap(dpy, current_pixmap);
    if (gc) XFreeGC(dpy, gc);
}

static Boolean check_display_local(Display *dpy) {
  char *name = DisplayString(dpy);
  
  if (*name == ':') /*Guaranteed to be local*/
     return True;
  else return False; /*Should check... maybe later.*/
}

void XImageHandler::display( rgbImage *image, short refresh)
{
  if (image->compressed()) image->uncompress();
  current_image= image;
  
  if (!XGetWindowAttributes(dpy,win,&attrib)) {
    fprintf(stderr,
	    "XImageHandler::check_display_local: Can't get attributes!\n");
    exit(-1);
  }

#ifdef LOCAL
  using_local = check_display_local(dpy);
#else /* LOCAL */
#ifdef SHM
  int major, minor;
  Bool pixmapsp;

  /*Note: the following bit of code gets us a shared image space to play with.
    It's a bit tedious, due to all the error checking and cleanups, but
    it works... ;-) */
  
  if (using_shm) {
#ifndef SHM_ONE
      XShmDetach(dpy, &shminfo);
#endif /* SHM_ONE */
      XDestroyImage(ximage);
#ifndef SHM_ONE	  
      shmdt(shminfo.shmaddr);
#endif /* SHM_ONE */
      shmctl(shminfo.shmid, IPC_RMID, 0);
  }
  if (check_display_local(dpy)) {
      if (using_shm = XShmQueryVersion(dpy, &major, &minor, &pixmapsp)) {
	  ximage = XShmCreateImage(dpy, attrib.visual, attrib.depth, 
				   ZPixmap, NULL, &shminfo, 
				   image->xsize(), image->ysize());
	  if (! ximage)
	      using_shm = False;
	  else {
	      if ((shminfo.shmid=
		   shmget(IPC_PRIVATE,
			  ximage->height*ximage->bytes_per_line,
			  IPC_CREAT|0777)) == -1) {
		  using_shm = False;
		  XDestroyImage(ximage);
	      } else {
		  shminfo.shmaddr = ximage->data =
		      shmat(shminfo.shmid, 0, 0);
		  if ((int)shminfo.shmaddr == -1) {
		      using_shm = False;
		      XDestroyImage(ximage);
		      shmctl(shminfo.shmid, IPC_RMID, 0);
		  }
#ifndef SHM_ONE		      
		  else if (! XShmAttach(dpy, &shminfo)) {
		      using_shm = False;
		      XDestroyImage(ximage);
		      shmdt(shminfo.shmaddr);
		      shmctl(shminfo.shmid, IPC_RMID, 0);
		  }
#endif /* SHM_ONE */
	      }
	  }
      }
  }
  else
      using_shm = False;

#endif /* SHM */
#endif /* LOCAL */
  
  image_x_size = image->xsize(); image_y_size = image->ysize();

#ifdef LOCAL
  if (! using_local)
#else /* LOCAL */
#ifdef SHM
  if (! using_shm)
#endif /* SHM */
#endif /* LOCAL */

  if (current_pixmap) XFreePixmap(dpy, current_pixmap);
  current_pixmap = XCreatePixmap(dpy, (Drawable)attrib.root, image->xsize(),
				 image->ysize(), attrib.depth);  

#ifdef LOCAL
  if (using_local)
      ximage= XGetImage(dpy, win, 0, 0, image->xsize(),
			image->ysize(), AllPlanes, ZPixmap);
  else
#else
#ifdef SHM
  if (! using_shm)
#endif /* SHM */
#endif /* LOCAL */

  ximage= XGetImage(dpy, current_pixmap, 0, 0, image->xsize(), image->ysize(),
		    AllPlanes, ZPixmap);

  if (!ximage) {
      fprintf(stderr,
	      "XImageHandler::display: XGetImage failed!\n");
      exit(-1);
  }
  
  if (! inited)
      (this->*init_fun)();
  
  (this->*display_fun)(ximage);


#ifdef LOCAL
  if (! using_local) {
#else /* LOCAL */
#ifdef SHM
#ifdef SHM_ONE	  
  shmdt(shminfo.shmaddr);
#endif /* SHM_ONE */
  if (! using_shm) {
#endif /* SHM */
#endif /* LOCAL */

  XPutImage(dpy, current_pixmap, gc, ximage, 0, 0, 0, 0,
	    image->xsize(), image->ysize());

  XDestroyImage(ximage);

#ifdef LOCAL
  }
#else
#ifdef SHM
  }
#endif /* SHM */
#endif /* LOCAL */

  XFlush(dpy);

  if (refresh && (attrib.map_state == IsViewable)) redraw();
}

void XImageHandler::redraw()
{
  /*Heh. BitBlt is so nice...*/
  get_window_size();
  if (inited) {
#ifdef LOCAL
    if (using_local)
      XPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, 
		image_x_size, image_y_size);
    else
#else
#ifdef SHM
      if (using_shm) {
#ifdef SHM_ONE
	XShmAttach(dpy, &shminfo);
#endif /* SHM_ONE */
	XShmPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, 
		     image_x_size, image_y_size, False);
#ifdef SHM_ONE
	XShmDetach(dpy, &shminfo);
#endif /* SHM_ONE */
      } else
#endif /* SHM */
#endif /* LOCAL */
	XCopyArea(dpy, current_pixmap, win, gc, 0, 0, 
		  win_x_size, win_y_size, 0, 0);
  }
  XFlush(dpy);
}

void XImageHandler::resize()
{
  /* This routine handles resize events on the window.
     Of course, all this really boils down to is generating
     a redraw request if the window grew.*/

  unsigned int old_win_x_size=win_x_size;
  unsigned int old_win_y_size=win_y_size;

  get_window_size();

  if (inited)
    if ((win_x_size > old_win_x_size) || (win_y_size > old_win_y_size))
      redraw();
}

void XImageHandler::get_window_size()
{
  // This routine adjusts this object's ideas about window size to match
  // the current state of its window.

  Drawable root;
  int x, y;
  unsigned int width, height, border_width, depth_dummy;
  if ( !XGetGeometry(dpy, (Drawable)win, &root, &x, &y, &width, &height,
		     &border_width, &depth_dummy) ) {
    fprintf(stderr,"XImageHandler::get_window_size: fatal error!\n");
    exit(-1);
  }
  win_x_size= width;
  win_y_size= height;
}

void XImageHandler::monochrome_display(XImage *ximage_in)
{
    unsigned int xlim, ylim;
    unsigned long white= WhitePixel(dpy, screen);
    unsigned long black= BlackPixel(dpy, screen);
    register int i,j;
    register unsigned long pixel;
    int run_backwards= 0;
    long *thiserr, *nexterr, *tmperr, *tmpmem;
    register long *nptr, *tptr;
    
    const int fs_scale=1024;
    const int half_fs_scale=512;

    xlim= current_image->xsize();
    ylim= current_image->ysize();
    
    tmpmem= new long(2*(xlim + 2));
    thiserr=tmpmem;
    nexterr=tmpmem+xlim+2;
    
    /* Initialize Floyd-Steinberg error vectors */
#if SYSV_RAND
    srand((int) time(0));
    for (int col = 0; col < (int) xlim; ++col) {
	thiserr[col] = (rand() % fs_scale - half_fs_scale) / 4;
    }
#else
    srandom((int) time(0));
    for (int col = 0; col < (int) xlim; ++col) {
	thiserr[col] = (random() % fs_scale - half_fs_scale) / 4;
    }
#endif
    
    /*We alternate carrying error to right and left to get a slightly
      better dither.*/
    for (j=0; j < (int) ylim; j++) {
	/*Clear up storage*/
	for (nptr = nexterr + xlim ; nptr >= nexterr; nptr--) {
	  *nptr = 0;
	}
	if (! run_backwards) {
	    nptr=nexterr;
	    tptr=thiserr;
	    for (i=0;i<=(int)(xlim-1);i++) {
		long value= rgb_to_intensity(current_image->pix_r(i,j),
					     current_image->pix_g(i,j),
					     current_image->pix_b(i,j))
		    * fs_scale / 256 + tptr[1];
		if (value>=half_fs_scale) {
 		    pixel= white;
		    value -= fs_scale;
		}
		else {
		    pixel= black;
		}
		(void)XPutPixel(ximage_in, i, j, pixel);
		tptr[2] += (value * 7) / 16;
		nptr[0] += (value * 3) / 16;
		nptr[1] += (value * 5) / 16;
		nptr[2] += value / 16;
		++tptr;
		++nptr;
	    }
	}
	else {
	    nptr=nexterr + xlim - 1;
	    tptr=thiserr + xlim - 1;
	    for (i=xlim-1;i>=0;i--) {
		long value= rgb_to_intensity(current_image->pix_r(i,j),
					     current_image->pix_g(i,j),
					     current_image->pix_b(i,j))
		    * fs_scale / 256 + tptr[1];
		if (value>=half_fs_scale) {
		    pixel= white;
		    value -= fs_scale;
		}
		else {
		    pixel= black;
		}
		(void)XPutPixel(ximage_in, i, j, pixel);
		tptr[0] += (value * 7) / 16;
		nptr[2] += (value * 3) / 16;
		nptr[1] += (value * 5) / 16;
		nptr[0] += value / 16;
		
		--tptr;
		--nptr;
	    }
	}
	run_backwards= !run_backwards;
	tmperr=thiserr;
	thiserr=nexterr;
	nexterr=tmperr;
    }
    delete(tmpmem);
}

void XImageHandler::pseudocolor_display(XImage *ximage_in)
{
    int xlim, ylim;
    register int i,j;
    int run_backwards= 0;
    register long *rnptr, *gnptr, *bnptr, *rtptr, *gtptr, *btptr;
    register long r, g, b;
    register long rpart, gpart, bpart;
    register long rfrac, gfrac, bfrac;
    register unsigned long pixel;
    
    int fs_scale = 1024;
    int half_fs_scale = 512;
    
    xlim= current_image->xsize();
    ylim= current_image->ysize();

    long *tmpmem = new long [ 6*(xlim+2) ];
    long *tmperr;
    long *r_err= tmpmem;
    long *g_err= tmpmem + 2*(xlim+2);
    long *b_err= tmpmem + 4*(xlim+2);

    long *r_err_next = r_err + xlim + 2;
    long *g_err_next = g_err + xlim + 2;
    long *b_err_next = b_err + xlim + 2;
    
    /* Initialize Floyd-Steinberg error vectors */
#if SYSV_RAND
    srand((int) time(0));
    for (int col = 0; col <= (xlim+1); ++col) {
	r_err[col] = (rand() % fs_scale - half_fs_scale) / 4;
	g_err[col] = (rand() % fs_scale - half_fs_scale) / 4;
	b_err[col] = (rand() % fs_scale - half_fs_scale) / 4;
    }
#else
    srandom((int) time(0));
    for (int col = 0; col <= (xlim+1); ++col) {
	r_err[col] = (random() % fs_scale - half_fs_scale) / 4;
	g_err[col] = (random() % fs_scale - half_fs_scale) / 4;
	b_err[col] = (random() % fs_scale - half_fs_scale) / 4;
    }
#endif
    
    for (j=0; j<ylim; j++) {
	/* clean out memory */
	rnptr = r_err_next + xlim + 1;
	gnptr = g_err_next + xlim + 1;
	bnptr = b_err_next + xlim + 1;

	for (; rnptr >= r_err_next; rnptr--,gnptr--,bnptr--)
	    *bnptr = *gnptr = *rnptr = 0;

	if (run_backwards) {
	    rnptr = r_err_next + xlim - 1;
	    gnptr = g_err_next + xlim - 1;
	    bnptr = b_err_next + xlim - 1;

	    rtptr = r_err + xlim - 1;
	    gtptr = g_err + xlim - 1;
	    btptr = b_err + xlim - 1;
	    
	    for (i=xlim-1; i>=0; i--) {

		/* what we're doing here is extracting "part" and "frac"
		   values. "Part" values are used to directly determine
		   the pixel intensity; they should always come out on
		   pixel plane values. "Frac" values should be divided up
		   in the sixteenfold way amongst adjoining dither cells.
		   */

		r= current_image->pix_r(i,j) * fs_scale * map_info.red_max
		    / 256 + rtptr[1];
		if ((rfrac = r % fs_scale) >= half_fs_scale) {
		    rpart = (r + fs_scale - rfrac) / fs_scale;
		    rfrac -= fs_scale;
		} else
		    rpart = (r - rfrac) / fs_scale;
		rtptr[0] += (rfrac * 7) / 16;
		rnptr[2] += (rfrac * 3) / 16;
		rnptr[1] += (rfrac * 5) / 16;
		rnptr[0] += rfrac / 16;
		--rtptr;
		--rnptr;
		
		g= current_image->pix_g(i,j) * fs_scale * map_info.green_max
		    / 256 + gtptr[1];
		if ((gfrac = g % fs_scale) >= half_fs_scale) {
		    gpart = (g + fs_scale - gfrac) / fs_scale;
		    gfrac -= fs_scale;
		} else
		    gpart = (g - gfrac) / fs_scale;
		gtptr[0] += (gfrac * 7) / 16;
		gnptr[2] += (gfrac * 3) / 16;
		gnptr[1] += (gfrac * 5) / 16;
		gnptr[0] += gfrac / 16;
		--gtptr;
		--gnptr;
		
		b= current_image->pix_b(i,j) * fs_scale * map_info.blue_max
		    / 256 + btptr[1];
		if ((bfrac = b % fs_scale) >= half_fs_scale) {
		    bpart = (b + fs_scale - bfrac) / fs_scale;
		    bfrac -= fs_scale;
		} else
		    bpart = (b - bfrac) / fs_scale;
		btptr[0] += (bfrac * 7) / 16;
		bnptr[2] += (bfrac * 3) / 16;
		bnptr[1] += (bfrac * 5) / 16;
		bnptr[0] += bfrac / 16;
		--btptr;
		--bnptr;
		
		pixel= map_info.base_pixel 
		    + rpart*map_info.red_mult
			+ gpart*map_info.green_mult
			    + bpart*map_info.blue_mult;
		
		(void)XPutPixel(ximage_in, i, j, pixel);
	    }
	}
	else {
	    rnptr = r_err_next;
	    gnptr = g_err_next;
	    bnptr = b_err_next;

	    rtptr = r_err;
	    gtptr = g_err;
	    btptr = b_err;
	    
	    for (i=0; i<=(xlim - 1); i++) {
		r= current_image->pix_r(i,j) * fs_scale * map_info.red_max
		    / 256 + rtptr[1];
		if ((rfrac = r % fs_scale) >= half_fs_scale) {
		    rpart = (r + fs_scale - rfrac) / fs_scale;
		    rfrac -= fs_scale;
		} else
		    rpart = (r - rfrac) / fs_scale;
		rtptr[2] += (rfrac * 7) / 16;
		rnptr[0] += (rfrac * 3) / 16;
		rnptr[1] += (rfrac * 5) / 16;
		rnptr[2] += rfrac / 16;
		++rtptr;
		++rnptr;
		
		g= current_image->pix_g(i,j) * fs_scale * map_info.green_max
		    / 256 + gtptr[1];
		if ((gfrac = g % fs_scale) >= half_fs_scale) {
		    gpart = (g + fs_scale - gfrac) / fs_scale;
		    gfrac -= fs_scale;
		} else
		    gpart = (g - gfrac) / fs_scale;
		gtptr[2] += (gfrac * 7) / 16;
		gnptr[0] += (gfrac * 3) / 16;
		gnptr[1] += (gfrac * 5) / 16;
		gnptr[2] += gfrac / 16;
		++gtptr;
		++gnptr;
		
		b= current_image->pix_b(i,j) * fs_scale * map_info.blue_max
		    / 256 + btptr[1];
		if ((bfrac = b % fs_scale) >= half_fs_scale) {
		    bpart = (b + fs_scale - bfrac) / fs_scale;
		    bfrac -= fs_scale;
		} else
		    bpart = (b - bfrac) / fs_scale;
		btptr[2] += (bfrac * 7) / 16;
		bnptr[0] += (bfrac * 3) / 16;
		bnptr[1] += (bfrac * 5) / 16;
		bnptr[2] += bfrac / 16;
		++btptr;
		++bnptr;
		
		pixel= map_info.base_pixel 
		    + rpart*map_info.red_mult
			+ gpart*map_info.green_mult
			    + bpart*map_info.blue_mult;
		
		(void)XPutPixel(ximage_in, i, j, pixel);
	    }
	}
	run_backwards= !run_backwards;
	tmperr = r_err;
	r_err = r_err_next;
	r_err_next = tmperr;
	tmperr = g_err;
	g_err = g_err_next;
	g_err_next = tmperr;
	tmperr = b_err;
	b_err = b_err_next;
	b_err_next = tmperr;
    }
    
    delete(tmpmem);
}

void XImageHandler::directcolor_display(XImage *ximage_in)
{
  int xlim, ylim;
  register int i,j;
  
  xlim= current_image->xsize();
  ylim= current_image->ysize();

  if ( (map_info.red_max==255) && (map_info.green_max==255)
      && (map_info.blue_max==255) ) {
    // short cut to handle most common case
    for (i=0; i<xlim; i++)
      for (j=0; j<ylim; j++) {
	unsigned long pixel= map_info.base_pixel
	  +current_image->pix_r(i,j)*map_info.red_mult
	  +current_image->pix_g(i,j)*map_info.green_mult
	  +current_image->pix_b(i,j)*map_info.blue_mult;
	(void)XPutPixel(ximage_in, i, j, pixel);
      }
  } else {
    for (i=0; i<xlim; i++)
      for (j=0; j<ylim; j++) {
	unsigned long pixel= map_info.base_pixel
	  +(((current_image->pix_r(i,j)*(map_info.red_max+1))/256)
	  *map_info.red_mult)
	  +(((current_image->pix_g(i,j)*(map_info.green_max+1))/256)
	  *map_info.green_mult)
	  +(((current_image->pix_b(i,j)*(map_info.blue_max+1))/256)
	  *map_info.blue_mult);
	(void)XPutPixel(ximage_in, i, j, pixel);
      }
  }
}

void XImageHandler::monochrome_init()
{
    inited=True;
}

int XImageHandler::server_is_sun_or_hp()
/* This routine returns true if the display is a Sun X11/News or HP server */
{
  char *vendor= XServerVendor(dpy);

  return( !strncmp(vendor, "X11/NeWS", 8)
         || !strncmp(vendor, "Sun Microsystems, Inc.", 22)
         || !strncmp(vendor, "Hewlett-Packard Company",23) );
}

static int pxl_compare( const void *p1, const void *p2 )
/* This routine is used by qsort to sort a list of pixel values. */
{
  if (*(long *)p1 > *(long *)p2) return(-1);
  if (*(long *)p1 == *(long *)p2) return(0);
  return(1);
}

void XImageHandler::generate_sun_rgbmap()
/* This routine generates an rgb map that won't frighten X11/News */ 
{
  int noSet= 0, noGrabbed= 0, noAlloced= 0;
  XColor *myColors= (XColor *)0;
  unsigned long pixels[512]; 
  int contig_pixels, start_pixel= 0;
  int l, m;
  unsigned int i, j, k;
  int found_map_space;
  
  /* The actual color map will be the display's default map */
  map_info.colormap= attrib.colormap;
  
  /* grab as many colours as possible */ 
  /* assume < 512 colours needed */ 
  for (l=256; l>0; l/=2) { 
    if (XAllocColorCells(dpy, map_info.colormap, False, 0, 0, 
			 pixels + noAlloced, l)) {
      noAlloced += l;
    }
  }
  
  /* Sort the available pixels */
  (void)qsort(pixels, noAlloced, sizeof(long), pxl_compare);
  
  /* Find a contiguous stretch to make into a map (top preferred) */
  m= 0;
  found_map_space= 0;
  while (!found_map_space && m<noAlloced) {
    contig_pixels= 0;
    start_pixel= m;
    m += 1;
    while (m<noAlloced && pixels[m] == pixels[m-1]-1) {
      contig_pixels++;
      m++;
    }
    if (contig_pixels>=64) { /* Found a place to put the map */
      if (contig_pixels>=125) { /* 5x5x5 case */
	map_info.red_max= 4;
	map_info.green_max= 4;
	map_info.blue_max= 4;
      }
      else if (contig_pixels>=100) { /* 5x5x4 case */
	map_info.red_max= 4;
	map_info.green_max= 4;
	map_info.blue_max= 3;
      }
      else if (contig_pixels>=80) { /* 5x4x4 case */
	map_info.red_max= 4;
	map_info.green_max= 3;
	map_info.blue_max= 3;
      }
      else { /* 4x4x4 case */
	map_info.red_max= 3;
	map_info.green_max= 3;
	map_info.blue_max= 3;
      }
      map_info.blue_mult= 1;
      map_info.green_mult= map_info.blue_max+1;
      map_info.red_mult= map_info.green_mult * (map_info.green_max + 1);
      map_info.base_pixel= start_pixel;
      noSet = (int) ((map_info.red_max + 1) * (map_info.green_max + 1)
			 * (map_info.blue_max + 1));
      found_map_space= 1;
    }
  }
  if (!found_map_space) {
    fprintf(stderr,
     "XImageHandler::generate_sun_rgbmap: couldn't find color table space!\n");
    exit(-1);
  }
  map_info.base_pixel= pixels[0] - noSet + 1;

  /* Free up unneeded color cells */
  if (start_pixel != 0) 
    XFreeColors(dpy, map_info.colormap, pixels, start_pixel, 0);
  if (start_pixel+noSet < noAlloced)
    XFreeColors(dpy, map_info.colormap, pixels+start_pixel+noSet, 
		noAlloced-(start_pixel+noSet), 0);    
  
  /* make up our default colour table */ 
  myColors= new XColor[noSet];
  
  /* Create the map */
  noGrabbed= 0;
  for (i=0; i<=map_info.red_max; ++i)
      for (j=0;j<=map_info.green_max; ++j)
	for (k=0; k<=map_info.blue_max;++k) {
	  myColors[noGrabbed].flags =
	    DoRed | DoGreen | DoBlue;
	myColors[noGrabbed].pixel = map_info.base_pixel + 
	    i * map_info.red_mult +
	      j * map_info.green_mult +
		k * map_info.blue_mult;
	/* now fill out the values */
	myColors[noGrabbed].red = (unsigned short)
	    ((i * 65535)/ map_info.red_max);
	myColors[noGrabbed].green = (unsigned short)
	    ((j * 65535) / map_info.green_max);
	myColors[noGrabbed].blue = (unsigned short)
	    ((k * 65535)/ map_info.blue_max);
	noGrabbed++;
    } 
  
  /* now store the colours, should be OK */ 
  XStoreColors(dpy, map_info.colormap, myColors, noGrabbed); 
  
  /* Clean up */
  delete myColors;
}

void XImageHandler::pseudocolor_init()	
{
  inited=True;
  
  /* Have to handle Sun X11/News and HP separately */
  if (!map_info_set) {
    if (server_is_sun_or_hp()) {
#ifdef SHARE_XWS_CMAP
      if (xws_best_rgb_stdmap) {
	map_info= *xws_best_rgb_stdmap; // bitwise copy will work
      }
      else {
	generate_sun_rgbmap();
	xws_best_rgb_stdmap= &map_info;
      }
#else
      generate_sun_rgbmap();
#endif
    } 
    else {
      XStandardColormap *mymaps = XAllocStandardColormap();
      int count;
      int map_index;
      if (XGetRGBColormaps(dpy, RootWindow(dpy,screen), &mymaps, &count,
			   XA_RGB_DEFAULT_MAP)) {
	for (map_index=0; map_index<count; map_index++) {
	  if (mymaps[map_index].visualid==XVisualIDFromVisual(attrib.visual))
	    break;
	}
	if (map_index<count) {
	  map_info= mymaps[map_index]; // bitwise copy will work
	}
	else {
	  if (XmuLookupStandardColormap(dpy, screen,
					XVisualIDFromVisual(attrib.visual),
					attrib.depth, 
					XA_RGB_DEFAULT_MAP, 
					False, True)) {
	    if (XGetRGBColormaps(dpy, RootWindow(dpy,screen),
				 &mymaps, &count,
				 XA_RGB_DEFAULT_MAP)) {
	      for (map_index=0; map_index<count; map_index++) {
		if (mymaps[map_index].visualid
		    == XVisualIDFromVisual(attrib.visual))
		  break;
	      }
	      if (map_index<count) {
		map_info= mymaps[map_index]; // bitwise copy will work
	      }
	      else {
		fprintf(stderr,"Can't get appropriate colormap!\n");
		exit(-1);
	      }
	    } 
	    else {
	      fprintf(stderr,
		      "Can't get appropriate colormap!\n");
	      exit(-1);
	    }
	  }
	  else {
	    fprintf(stderr,"Can't get appropriate colormap!\n");
	    exit(-1);
	  }
	}
      } 
#ifdef SHARE_XWS_CMAP
      else {
	if (xws_best_rgb_stdmap)
	  map_info= *xws_best_rgb_stdmap; // bitwise copy will work
	else {
	  generate_sun_rgbmap();
	  xws_best_rgb_stdmap= &map_info;
	  }
      }
#else  // SHARE_XWS_CMAP
#endif // SHARE_XWS_CMAP
      else {
	if (XmuLookupStandardColormap(dpy, screen,
				      XVisualIDFromVisual(attrib.visual),
				      attrib.depth, 
				      XA_RGB_DEFAULT_MAP, 
				      False, True)) {
	  if (XGetRGBColormaps(dpy, RootWindow(dpy,screen),
			       &mymaps, &count,
			       XA_RGB_DEFAULT_MAP)) {
	      for (map_index=0; map_index<count; map_index++) {
		if (mymaps[map_index].visualid
		    == XVisualIDFromVisual(attrib.visual))
		  break;
	      }
	      if (map_index<count) {
		map_info= mymaps[map_index]; // bitwise copy will work
	      }
	      else {
		fprintf(stderr,"Can't get appropriate colormap!\n");
		exit(-1);
	      }
	  }
	  else {
	    fprintf(stderr,"Can't get appropriate colormap!\n");
	    exit(-1);
	  }
	}
	else {
	  fprintf(stderr,"Can't get appropriate colormap!\n");
	  exit(-1);
	}
      }
    }
    map_info_set= 1;
  }
  XSetWindowColormap(dpy, win, map_info.colormap);
}

void XImageHandler::directcolor_init()	
{
    inited=True;

    // We assume this is a 24 bit system with 8 bits per color, but
    // what order are the colors stored in?
    XColor red, green, blue;
    (void)XParseColor(dpy,DefaultColormap(dpy,screen),"rgb:ff/00/00",&red);
    (void)XParseColor(dpy,DefaultColormap(dpy,screen),"rgb:00/ff/00",&green);
    (void)XParseColor(dpy,DefaultColormap(dpy,screen),"rgb:00/00/ff",&blue);
    (void)XAllocColor(dpy,DefaultColormap(dpy,screen), &red);
    (void)XAllocColor(dpy,DefaultColormap(dpy,screen), &green);
    (void)XAllocColor(dpy,DefaultColormap(dpy,screen), &blue);

    map_info.base_pixel= 0;
    map_info.colormap= DefaultColormap(dpy,screen);

#define COLOR_MAP_MATH( a, b, c ) \
	map_info.c##_mult= 1; \
	map_info.c##_max= c.pixel; \
	map_info.b##_mult= (map_info.c##_max+1)*(map_info.c##_mult); \
	map_info.b##_max= b.pixel/(map_info.b##_mult); \
	map_info.a##_mult= (map_info.b##_max+1)*(map_info.b##_mult); \
	map_info.a##_max= a.pixel/map_info.a##_mult; 

    if (red.pixel>green.pixel) {
      if (green.pixel>blue.pixel) {
	// RGB
	COLOR_MAP_MATH(red,green,blue);
      }
      else if (red.pixel>blue.pixel) {
	// RBG
	COLOR_MAP_MATH(red,blue,green);
      }
      else {
	// BRG
	COLOR_MAP_MATH(blue,red,green);
      }
    }
    else {
      if (green.pixel>blue.pixel) {
	if (blue.pixel>red.pixel) {
	  // GBR
	  COLOR_MAP_MATH(green,blue,red);
	}
	else {
	  // GRB
	  COLOR_MAP_MATH(green,red,blue);
	}
      }
      else {
	// BGR
	COLOR_MAP_MATH(blue,green,red);
      }
    }
#undef COLOR_MAP_MATH
}
