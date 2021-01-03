/****************************************************************************
 * xdrawih.h
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

#include "ximagehandler.h"

#ifdef __cplusplus

class XDrawImageHandler : public XImageHandler {
protected:
  int pixheight, pixwidth;
  unsigned long calc_pixel( int r, int g, int b ) const
  { 
    unsigned long result= map_info.base_pixel
        +(((map_info.red_max*r)+128) >> 8) * map_info.red_mult
        +(((map_info.green_max*g)+128) >> 8) * map_info.green_mult
        +(((map_info.blue_max*b)+128) >> 8) * map_info.blue_mult;
    return result;
  }
public:
  XDrawImageHandler(Display *d_in, int s_in, Window w_in, 
		    int width, int height);
  ~XDrawImageHandler();
  void clear(int r, int g, int b);
  void winsize(int *height, int *width);
  void ppoint(int r, int g, int b, int pnum, XPoint *points);
  void pline(int r, int g, int b, int pnum, XPoint *points);
  void pgon(int r, int g, int b, int vnum, XPoint *points);
};

#ifdef MOTIF
class XmautodrawImageHandler : public XmautoImageHandler {
protected:
  Boolean check();
  int pixheight, pixwidth;
public:
  XmautodrawImageHandler(Widget parent, int height, int width, 
			 Widget base_parent = NULL,
			 Arg *top_args = NULL, int top_n = 0,
			 rgbImage *initial_image = NULL);
  ~XmautodrawImageHandler();
  void clear(int r, int g, int b);
  void winsize(int *height, int *width);
  void ppoint(int r, int g, int b, int pnum, XPoint *points);
  void pline(int r, int g, int b, int pnum, XPoint *points);
  void pgon(int r, int g, int b, int vnum, XPoint *points);
};
#endif /* ifdef MOTIF */

/* C-accessible entry points */
extern "C" void* xdrawih_create(Display *d_in, int s_in, Window w_in, 
				int width, int height);
extern "C" void xdrawih_delete(void *XdrawImageHandler);
extern "C" void xdrawih_ppoint(void *XdrawImageHandler,
			       int r, int g, int b, int pnum, XPoint *points);
extern "C" void xdrawih_pline(void *XdrawImageHandler,
			      int r, int g, int b, int pnum, XPoint *points);
extern "C" void xdrawih_pgon(void *XdrawImageHandler,
			     int r, int g, int b, int pnum, XPoint *points);
extern "C" void xdrawih_winsize(void *XdrawImageHandler, 
				int *height, int *width);
extern "C" void xdrawih_redraw(void *XdrawImageHandler);
extern "C" void xdrawih_clear(void *XdrawImageHandler, int r, int g, int b);
extern "C" Window xdrawih_window( void *XdrawImageHandler );

#ifdef MOTIF
extern "C" void* xmadih_create(Widget parent, int height, int width);
extern "C" void xmadih_delete(void *XmautodrawImageHandler);
extern "C" void xmadih_ppoint(void *XmautodrawImageHandler,
			      int r, int g, int b, int pnum, XPoint *points);
extern "C" void xmadih_pline(void *XmautodrawImageHandler,
			     int r, int g, int b, int pnum, XPoint *points);
extern "C" void xmadih_pgon(void *XmautodrawImageHandler,
			    int r, int g, int b, int pnum, XPoint *points);
extern "C" void xmadih_winsize(void *XmautodrawImageHandler, 
			       int *height, int *width);
extern "C" void xmadih_redraw(void *XmautodrawImageHandler);
extern "C" void xmadih_clear(void *XmautodrawImageHandler, 
			     int r, int g, int b);
extern "C" Widget xmadih_widget( void *XmautodrawImageHandler );
extern "C" Window xmadih_window( void *XmautodrawImageHandler );
#endif /* ifdef MOTIF */

#else /* ifdef __cplusplus */

typedef struct XdrawImageHandler_struct XdrawImageHandler;

extern XdrawImageHandler* xdrawih_create(Display *d_in, int s_in, Window w_in, 
					 int width, int height);
extern void xdrawih_delete(XdrawImageHandler *xdrawih);
extern void xdrawih_ppoint(XdrawImageHandler *xdrawih,
			   int r, int g, int b, int pnum, XPoint *points);
extern void xdrawih_pline(XdrawImageHandler *xdrawih,
			  int r, int g, int b, int pnum, XPoint *points);
extern void xdrawih_pgon(XdrawImageHandler *xdrawih,
			 int r, int g, int b, int pnum, XPoint *points);
extern void xdrawih_winsize(XdrawImageHandler *xdrawih,
			    int *height, int *width);
extern void xdrawih_redraw(XdrawImageHandler *xdrawih);
extern void xdrawih_clear(XdrawImageHandler *xdrawih, int r, int g, int b);
extern Window xdrawih_window( XdrawImageHandler *xdrawih );

#ifdef MOTIF
typedef struct XmautodrawImageHandler_struct XmautodrawImageHandler;

extern XmautodrawImageHandler* xmadih_create(Widget parent, 
					     int height, int width);
extern void xmadih_delete(XmautodrawImageHandler *xmadih);
extern void xmadih_ppoint(XmautodrawImageHandler *xmadih,
			  int r, int g, int b, int pnum, XPoint *points);
extern void xmadih_pline(XmautodrawImageHandler *xmadih,
			 int r, int g, int b, int pnum, XPoint *points);
extern void xmadih_pgon(XmautodrawImageHandler *xmadih,
			int r, int g, int b, int pnum, XPoint *points);
extern void xmadih_winsize(XmautodrawImageHandler *xmadih,
			   int *height, int *width);
extern void xmadih_redraw(XmautodrawImageHandler *xmadih);
extern void xmadih_clear(XmautodrawImageHandler *xmadih, int r, int g, int b);
extern Widget xmadih_widget( XmautodrawImageHandler *xmadih );
extern Window xmadih_window( XmautodrawImageHandler *xmadih );
#endif /* ifdef MOTIF */

#endif

