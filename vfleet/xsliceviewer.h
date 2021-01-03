/****************************************************************************
 * xsliceviewer.h 
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

class XSliceViewer: public baseSliceViewer {
public:
  XSliceViewer( Widget parent_widget_in, MrmHierarchy mrm_id_in, 
		const GridInfo& grid_in,
		baseTransferFunction *tfun_in, 
		const int ndata_in, DataVolume** dtbl_in,
		const int which_dir_in,
		void (*deletion_cb_in)(XSliceViewer* delete_me),
		const int manage_dvols_in=0 );
  ~XSliceViewer();
private:  
  void update_image();
  static int initialized;
  static int min_image_form_width;
  MrmHierarchy mrm_id;
  Widget parent_widget;
  Widget widget;
  Widget image_widget;
  Widget alpha_button_widget;
  XImageHandler *ihandler;
  rgbImage *image;
  void (*deletion_cb)( XSliceViewer* );
  void register_object_name();
  static XSliceViewer* get_object( Widget w );
  static void delete_cb( Widget w, int *id, unsigned long *reason );
  void issue_delete_request( Widget w, int *id, unsigned long *reason );
  static void create_cb( Widget w, int *id, unsigned long *reason );
  void create( Widget w, int *id, unsigned long *reason );
  static void expose_cb( Widget w, int *id, unsigned long *reason );
  void expose( Widget w, int *id, unsigned long *reason );
  static void slider_update_cb( Widget w, int *id, unsigned long *reason );
  void slider_update( Widget w, int *id, unsigned long *reason );
  static void update_image_cb( Widget w, int *id, unsigned long *reason );
  void center_image();
};


