/****************************************************************************
 * glximagehandler.h for pvm 2.4
 * Author Joel Welling
 * Motif extension added by Robert Earhart
 * glx support added by Daniel Martinez
 *
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon
 * University
 *
 * Permission to use, copy, and modify this software and its
 * documentation without fee for personal use or use within your
 * organization is hereby granted, provided that the above copyright
 * notice is preserved in all copies and that that copyright and this
 * permission notice appear in supporting documentation. Permission to
 * redistribute this software to other organizations or individuals is
 * not granted; that must be negotiated with the PSC. Neither the PSC
 * nor Carnegie Mellon University make any representations about the
 * suitability of this software for any purpose. It is provided "as
 * is" without express or implied warranty.
 *****************************************************************************/

#include <math.h>

class rgbImage;


class GLXImageHandler : public XImageHandler {
 public:
  GLXImageHandler(Display *dpy_in, int screen_in, Window win_in);
  ~GLXImageHandler();
  void display(rgbImage *image, short refresh = 1);
  void redraw();
  void resize();
 protected:
  void (GLXImageHandler::*display_fun)(unsigned long *image);
  void (GLXImageHandler::*init_fun)();
  unsigned long *current_lrect;
 private:
  void glx_ih_display(unsigned long *image);
  void glx_ih_init();
}; 


#ifdef UNREADY
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
#endif /* UNREADY */
