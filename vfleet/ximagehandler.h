
/****************************************************************************
 * ximagehandler.h for pvm 2.4
 * Author Joel Welling
 * Motif extension added by Robert Earhart
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

#include <math.h>

#ifndef INCL_XIMAGEHANDLER_H

#define INCL_XIMAGEHANDLER_H 1

class rgbImage;

#ifndef AVOID_XVIEW
class xvautoimagehandler_window1_objects;
#endif

XVisualInfo *get_best_visual(Display *dpy, int screen);

class XImageHandler : public baseImageHandler {
public:
  XImageHandler( Display *dpy_in, int screen_in, Window win_in);
  ~XImageHandler();
  void display( rgbImage *image , short refresh=1);
  virtual void redraw();
  virtual void resize();
  int xsize() const { return (int)win_x_size; }
  int ysize() const { return (int)win_y_size; }
  Window get_win() const { return win; }
protected:
  Boolean inited;
  virtual void get_window_size();
  Display *dpy;
  int screen;
  Window win;
  XWindowAttributes attrib; 
  Pixmap current_pixmap;
  static XStandardColormap map_info;
  static int map_info_set;
  GC gc;
  unsigned int win_x_size, win_y_size, image_x_size, image_y_size;
  long rgb_to_intensity( long r, long g, long b )
    {
      // We assume incoming values are 0 to 255
      return( (77*r + 151*g + 27*b) / 256 );
    }
  XImage *ximage;
#ifdef LOCAL
  Boolean using_local;
#else
#ifdef SHM
  Boolean using_shm;
  XShmSegmentInfo shminfo;
#endif /* SHM */
#endif /* LOCAL */
  void (XImageHandler::*display_fun)(XImage *ximage);
  void (XImageHandler::*init_fun)();
private:
  void monochrome_display(XImage *ximage);
  void pseudocolor_display(XImage *ximage);
  void directcolor_display(XImage *ximage);
  void monochrome_init();
  void pseudocolor_init();
  void directcolor_init();
  int server_is_sun_or_hp();
  void generate_sun_rgbmap();
}; 

#ifndef AVOID_XVIEW
class XvautoImageHandler : public baseImageHandler {
public:
  XvautoImageHandler( rgbImage *image, char *name );
  ~XvautoImageHandler();
  void display( rgbImage *image, short refresh=1);
  static Attr_attribute WIN_IHANDLER_KEY;
  static Attr_attribute WIN_AUTOHANDLER_KEY;
  int xsize() const { return ihandler->xsize(); }
  int ysize() const { return ihandler->ysize(); }
private:
  xvautoimagehandler_window1_objects *ui_objects;
  XImageHandler *ihandler;
};
#endif

#ifdef MOTIF
class XmautoImageHandler: public baseImageHandler {
private:
  Atom atomcolormapwindows;
  int redrawn_before;
protected:
  XImageHandler *the_image;
  Widget draw_me, draw_parent;
  Boolean does_top;
  virtual Boolean check();
  
public:
  FILE *fp;
  XmautoImageHandler(Widget parent, int height, int width,
		     Widget base_parent = NULL,
		     Arg *top_args = NULL, int top_n = 0,
		     rgbImage *initial_image = NULL,
		     FILE *fp = NULL);
  ~XmautoImageHandler();
  void redraw();
  void resize();
  void display(rgbImage *image, short refresh=1);
  Widget widget() {return(draw_me);};
  int xsize() const { return the_image->xsize(); }
  int ysize() const { return the_image->ysize(); }
  Window get_win() { if (check()) return the_image->get_win();
                   else return 0; }
};

#endif /*ifdef MOTIF*/

#endif /* ifndef INCL_XIMAGEHANDLER_H */
