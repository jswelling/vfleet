/****************************************************************************
 * xcamhandler.h
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

#include "lists.h"

class XCamHandler {
public:
  XCamHandler( Widget parent_widget_in, MrmHierarchy mrm_id_in,
	       Camera* cam_to_manage, 
	       void (*cam_update_cb_in)(XCamHandler* this_handler, 
					const Camera& new_cam)=NULL);
  ~XCamHandler();
  void popup();
  void register_drag_window( Widget drag_window_in );
  static Camera* cam_from_viewed_volume( const gBoundBox& bbox );
  void save_cam_list_to_file( char* fname );
  void cam_to_dlog( const Camera& display_me );
  void set();
  void add_displayed_cam_to_list();
protected:
  struct MousePosition { 
    int x, y, maxx, maxy; 
    float max_size() const { return( (maxx >= maxy) ? maxx : maxy ); }
    float frac_x() const 
    { return( (float)(x - (max_size()/2))/(float)(max_size()/2) ); }
    float frac_y() const
    { return( (float)(y - (max_size()/2))/(float)(max_size()/2) ); }
  };
  static int initialized;
  MrmHierarchy mrm_id;
  void (*cam_update_cb)(XCamHandler* this_handler, const Camera& new_cam);
  Widget parent_widget;
  Widget widget;
  Widget drag_window;
  Widget list_count_widget;
  Widget rotate_widget;
  Widget dolly_widget;
  Widget hither_widget;
  Widget yon_widget;
  Widget from_x_widget;
  Widget from_y_widget;
  Widget from_z_widget;
  Widget at_x_widget;
  Widget at_y_widget;
  Widget at_z_widget;
  Widget up_x_widget;
  Widget up_y_widget;
  Widget up_z_widget;
  Widget fovea_scale_widget;
  Widget shift_at_tb;
  Widget shift_both_tb;
  Widget cam_list_add_pb;
  Widget cam_list_prev_pb;
  Widget cam_list_next_pb;
  Widget cam_list_save_pb;
  Widget save_list_file_dlog;
  Widget parallel_proj_tb;
  MousePosition drag_start, drag_end;
  int drag_start_valid;
  Camera* managed_camera;
  void register_object_name();
  static XCamHandler* get_object( Widget w );
  static void create_cb( Widget w, int *id, unsigned long *reason );
  void create( Widget w, int *id, unsigned long *reason );
  static void expose_cb( Widget w, int *id, unsigned long *reason );
  void expose( Widget w, int *id, unsigned long *reason );
  static void set_cb( Widget w, int *id, unsigned long *reason );
  static void reset_cb( Widget w, int *id, unsigned long *reason );
  void reset( Widget w, int *id, unsigned long *reason );
  static void button_press_cb( Widget w, int *id, unsigned long *reason );
  void button_press( Widget w, int *id, unsigned long *reason );
  static void window_input_cb( Widget w, XCamHandler* handler, XEvent* event );
  void window_input( Widget w, XEvent* event );
  void save_list();
  static void save_list_file_cb( Widget w, int *id, 
				 XmFileSelectionBoxCallbackStruct *call_data);
  void save_list_file( Widget w, int *id, 
		       XmFileSelectionBoxCallbackStruct *call_data);
  int int_from_text_widget( Widget w );
  int int_from_text_widget( Widget w, const int min, const int max );
  float float_from_text_widget( Widget w );
  float float_from_text_widget( Widget w, const float min, const float max );
  Camera cam_from_dlog();
  void dolly( const float dist );
  void rotate( const float degrees );
  void drag_shift( const MousePosition& drag_start, 
		   const MousePosition& drag_end );
  dList<Camera*> cam_list;
  dList_iter<Camera*>* cam_list_iter;
  int cams_in_list;
  Camera* cam_list_current;
};

