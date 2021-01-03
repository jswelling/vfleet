/****************************************************************************
 * vfleet.h
 * Authors Robert Earhart, Joel Welling
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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

// This provides info needed for the VFleet UI

#include "lists.h"
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "vren.h"
#include "tfun.h"
#include "tfunhandler.h"
#include "xtfunhandler.h"
#include "sliceviewer.h"
#include "xsliceviewer.h"
#include "xphotocopy.h"
#include "slicedvol.h"
#include "canvases.h"

// Here begin the shared integer declarations for communication with the UI.
const int k_file_menu_pb_id= 1;
const int k_open_id=2;
const int k_canvas_id=3;
const int k_quality_settings_id=4;
const int k_debugger_controls_id=5;
const int k_logger_id=6;
const int k_datavol_list_id= 7;
const int k_photo_id=8;
const int k_table_tfun_open_id= 9;
const int k_steering_box_id= 10;
const int k_free_datavol_id= 11;
const int k_image_save_id= 12;
const int k_open_remote_id= 13;
const int k_tcl_script_id= 14;
const int k_data_xsize_text_id= 15;
const int k_data_ysize_text_id= 16;
const int k_data_zsize_text_id= 17;
const int k_renderer_controls_id= 18;
const int k_opac_length_id= 19;
const int k_use_lighting_id= 20;
const int k_fast_lighting_id= 21;
const int k_fast_distances_id= 22;
const int k_opac_limit_id= 23;
const int k_color_comp_error_id= 24;
const int k_opac_min_id= 25;
const int k_open_remote_pb_id= 26;
const int k_table_tfun_id= 27;
const int k_table_tfun_save_id= 28;
const int k_table_tfun_image_id= 29;
const int k_table_tfun_edit_id= 30;
const int k_trilinear_interp_id= 31;
const int k_specular_lighting_id= 32;
const int k_threed_mipmap_id= 33;
const int k_camera_controls_id= 34;
const int k_remote_load_dir_id= 35;
const int k_remote_load_file_id= 36;
const int k_rmt_dta_xsize_text_id= 37;
const int k_rmt_dta_ysize_text_id= 38;
const int k_rmt_dta_zsize_text_id= 39;

const int MAX_ARGS= 20;

class baseDataFile;

// Structure for holding DataVolume information
struct dvol_struct {
  baseDataFile *file;
  DataVolume *dvol;
  char *name;
  XmString xstring;
  dvol_struct( char *name_in, baseDataFile* file_in, DataVolume* dvol_in );
  ~dvol_struct();
};

extern int num_remote_procs; // 0 in local mode

extern baseLogger *logger;
extern XImageHandler *main_ihandler;
extern baseVRen *main_vren;
extern VolGob *main_volgob;
extern baseSampleVolume *main_svol;
extern baseTfunHandler *tfun_handler;
extern baseXTfunHandler *xtfun_handler;
extern baseTransferFunction *main_tfun;
extern dList<dvol_struct*> datavol_list;
extern dList<XSliceViewer*> slice_viewer_list;

extern int tfun_changed_flag;

extern Widget app_shell;
extern MrmHierarchy mrm_id;

extern float current_opac_scale;

extern gVector current_lighting_dir;

// Data structure for overwrite-existing-file dialog business
struct file_write_warning_data {
  char *fname;
  Widget to_be_closed;
};

// Data structure for delete-something-important dialog business
struct delete_warning_data {
  void *info;
  Widget to_be_closed;
};

// Entry points in vfleet module
extern int render_in_progress;
extern char* add_default_path( const char *fname_root );
extern FILE* fopen_read_default_dir( const char *fname_root );
extern void quick_event_check();
extern void watch_events_and_await_render();
extern void save_image( char* fname );
extern void do_render();
extern int load_data( char* fname, 
		     const int replace_which= -1 ); // -1 means append new dvol
extern int load_remote_data( char* fname, 
		    const int replace_which= -1 ); // -1 means append new dvol
extern void resize_image( const int width, const int height );
extern void vfleet_quit();

// Entry points in vfleet_info module
extern void vfleet_error_reg();
extern void pop_error_dialog( char *text );
extern void pop_warning_dialog( char *text,
			       void (*ok_cb)(Widget, caddr_t, caddr_t),
			       void (*cancel_cb)(Widget, caddr_t, caddr_t),
			       void *client_data );
extern void pop_info_dialog( char *text );

// Entry points in vfleet_tfun module
extern void vfleet_tfun_create( Widget w, int *id, unsigned long *reason );
extern void vfleet_tfun_open( Widget w, int *tag, caddr_t cb );
extern void vfleet_tfun_reg();
extern void vfleet_tfun_cleanup();
extern void update_tfuns();
extern int precalculate_svol();
extern void datavol_update_inform( int datavol_removed );
extern int load_tfun( const char* fname );

// Entry points in vfleet_ren_ctrl module
extern void vfleet_ren_ctrl_create( Widget w, int *id, unsigned long *reason );
extern void vfleet_ren_ctrl_open( Widget w, int *tag, caddr_t cb );
extern void vfleet_ren_ctrl_reg();
extern void set_renderer_controls_cb( Widget w, XtPointer foo, XtPointer bar );
extern void set_quality_settings_cb( Widget w, XtPointer foo, XtPointer bar );
extern void vfleet_update_ren_ctrl_widgets();

// Entry points for the navigation module
extern void vfleet_nav_init( Widget drag_window_in );
extern void vfleet_nav_reg();
extern void vfleet_nav_cleanup();
extern void vfleet_nav_create( Widget w, int *id, unsigned long *reason );
extern void vfleet_nav_open( Widget w, int *tag, caddr_t cb );
extern void orient_model( gTransfm& new_trans );
extern void vfleet_nav_init_camera( const gBoundBox& bbox );
extern void vfleet_nav_set_camera( const Camera& cam_in );
extern void vfleet_nav_list_camera();
extern int vfleet_nav_cam_initialized(); // non-zero if init_camera called
extern Camera* current_camera();
extern const gTransfm& current_model_trans();

// Entry points for the Tcl script reading module
extern int vfleet_tcl_script_pending;
extern void vfleet_script_reg();
extern void vfleet_tcl_init();
extern void vfleet_tcl_expect_script( char* fname );
extern int vfleet_tcl_exec_script();
