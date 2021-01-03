/****************************************************************************
 * xphotocopy.h
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

class XPhotocopy {
public:
  XPhotocopy( Widget parent_widget_in, MrmHierarchy mrm_id_in,
	      rgbImage *image_in,
	      void (*deletion_cb_in)(XPhotocopy* delete_me) );
  ~XPhotocopy();
protected:
  void (*deletion_cb)( XPhotocopy* );
  virtual void issue_delete_request( Widget w, int *id, unsigned long *reason )
  { if (deletion_cb) (*deletion_cb)( this ); }
  static int initialized;
  MrmHierarchy mrm_id;
  Widget parent_widget;
  Widget widget;
  Widget image_widget;
  rgbImage *image; // valid only during construction
  XImageHandler *ihandler;
  void register_object_name();
  static XPhotocopy* get_object( Widget w );
  static void delete_cb( Widget w, int *id, unsigned long *reason );
  static void create_cb( Widget w, int *id, unsigned long *reason );
  void create( Widget w, int *id, unsigned long *reason );
  static void expose_cb( Widget w, int *id, unsigned long *reason );
  void expose( Widget w, int *id, unsigned long *reason );
};

