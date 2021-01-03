/****************************************************************************
 * uildriver.cc
 * Authors  Robert Earhart, Joel Welling
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

/* vfleet.cc
   A really cool way to view volumetric data under Mootif.
   
   This program really ought to do Everything in the world.*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Mrm/MrmAppl.h>

#include <p3dgen.h>
#include <drawp3d.h>

#include "logger.h"
#include "xlogger.h"
#include "geometry.h"
#include "cball.h"
#include "xcball.h"

baseLogger *logger;
int datavol_present_flag= 0;
int svol_ready_flag= 0;
int tfun_ready_flag= 0;

const int MAX_ARGS= 20;

// Here begin the shared integer declarations for communication with the UI.
const int k_open_id=2;
const int k_canvas_id=3;
const int k_quality_settings_id=4;
const int k_debugger_controls_id=5;
const int k_logger_id=6;
const int k_dataset_label_id=7;
const int k_photo_id=8;
const int k_tfun_open_id= 9;
const int k_steering_box_id= 10;
const int k_tfun_label_id= 11;
const int k_image_save_id= 12;
const int k_grad_tfun_open_id= 13;
const int k_grad_tfun_label_id= 14;
const int k_data_xsize_text_id= 15;
const int k_data_ysize_text_id= 16;
const int k_data_zsize_text_id= 17;
const int k_renderer_controls_id= 18;
const int k_opac_length_id= 19;
const int k_tfun_dlog_id= 27;
const int k_tfun_save_id= 28;
const int k_tfun_dlog_image_id= 29;
const int k_tfun_dlog_edit_id= 30;
const int k_grad_tfun_save_id=31;
const int k_grad_tfun_dlog_image_id=32;
const int k_grad_tfun_dlog_edit_id=33;
const int k_grad_tfun_dlog_id=34;

const int hdf_file_type=1;

// This bunk needs fixed.

static int file_type = hdf_file_type; // Global file type? ICK.

static Boolean debug_shooting = False;
static Boolean showing_debugger = False;

static float opac_limit_in = .9;
static float color_comp_in = .05;
static float grad_comp_in = .05;
static float grad_mag_in = .05;

static gTransfm model_trans;

// These are a few misc MRM declarations that really ought to be global.
static MrmHierarchy mrm_id;
static char *mrm_vec[]={"vfleet.uid"};
static MrmCode mrm_class;

// Storage space for the various widgets the UI tells us about.
static XtAppContext app_context;
static Widget open_id = NULL;
static Widget tfun_open_id= NULL;
static Widget canvas_id = NULL;
static Widget quality_settings_id = NULL;
static Widget debugger_controls_id = NULL;
static Widget steering_box_id = NULL;
static Widget dataset_label_id = NULL;
static Widget tfun_label_id = NULL;
static Widget grad_tfun_open_id = NULL;
static Widget grad_tfun_label_id = NULL;
static Widget image_save_id= NULL;
static Widget data_xsize_text_id= NULL;
static Widget data_ysize_text_id= NULL;
static Widget data_zsize_text_id= NULL;
static Widget renderer_controls_id= NULL;
static Widget opac_length_id= NULL;
static Widget tfun_dlog_id= NULL;
static Widget tfun_save_id= NULL;
static Widget tfun_dlog_image_id= NULL;
static Widget tfun_dlog_edit_id= NULL;
static Widget grad_tfun_save_id= NULL;
static Widget grad_tfun_dlog_image_id= NULL;
static Widget grad_tfun_dlog_edit_id= NULL;
static Widget grad_tfun_dlog_id= NULL;

// For communication with DrawP3D
extern "C" Window window_id;
extern "C" Window top_window_id;
extern "C" Display *display_ptr;

// Our main dialog shell
static Widget app_shell = NULL;

// The crystal ball controller
static CBall *cball= NULL;

char **argv;

class XImageHandler;
class rgbImage;

// Callbacks for the UI.
static void photo_expose_cb(Widget w,
			   XImageHandler *photo,
			   unsigned long *reason) {
}

static void photo_set_cb(Widget w,
			   rgbImage *image,
			   unsigned long *reason) {
}

static void num_text_check_cb(Widget text_w, caddr_t unused, 
			      XmTextVerifyCallbackStruct *cbs)
/* This callback verifies that a text field change is leaving a valid 
 * positive number. 
 */
{
  int len= 0;
  int c;

  /* Ignore backspace */
  if (cbs->startPos < cbs->currInsert) return;

  /* Make sure it's all either numbers or '.', deleting anything that isn't */
  for (len=0; len<cbs->text->length; len++) {
    c= cbs->text->ptr[len];
    if (!isdigit(c) && (c != '.')) { /* reject it */
      int i;
      for (i=len; (i+1)<cbs->text->length; i++)
	cbs->text->ptr[i]= cbs->text->ptr[i+1];
      cbs->text->length--;
      len--;
    }
  }
  if (cbs->text->length == 0) cbs->doit= False;
}

static void cball_draw_init()
{
  static int initialized= 0;

  if (!initialized) {
    top_window_id= XtWindow(app_shell);
    window_id= XtWindow(steering_box_id);
    display_ptr= XtDisplay(steering_box_id);
    
    (void)pg_init_ren("myrenderer","painter","xws","-");
    
    P_Point lookfrom;
    lookfrom.x= 0.0; lookfrom.y= 0.0; lookfrom.z= 10.0;
    P_Point lookat;
    lookat.x= 0.0; lookat.y= 0.0; lookat.z= 0.0;
    P_Vector up;
    up.x= 0.0; up.y= 1.0; up.z= 0.0;
    P_Point light_loc;
    light_loc.x= 0.0; light_loc.y= 5.0; light_loc.z= 10.0;
    P_Color light_color;
    light_color.ctype= P3D_RGB; 
    light_color.r= light_color.g= light_color.b= 0.8; light_color.a= 1.0;
    P_Color ambient_color;
    ambient_color.ctype= P3D_RGB; 
    ambient_color.r= ambient_color.g= ambient_color.b= 0.3; 
    ambient_color.a= 1.0;
    P_Color yellow;
    yellow.ctype= P3D_RGB; 
    yellow.r= yellow.g= 1.0; yellow.b= 0.0; yellow.a= 1.0;
    float fovea= 20.0;
    float hither= -5.0;
    float yon= -15.0;
    
    (void)pg_open("mylights");
    (void)pg_light(&light_loc, &light_color);
    (void)pg_ambient(&ambient_color);
    (void)pg_close();
    (void)pg_camera("mycamera",&lookfrom, &lookat, &up, fovea, hither, yon);
    
    P_Point llf;
    llf.x= llf.y= llf.z= -1.0;
    P_Point trb;
    trb.x= trb.y= trb.z= 1.0;
    (void)pg_open("mygob");
    (void)pg_boundbox( &llf, &trb );
    (void)pg_open("");
    (void)pg_gobcolor(&yellow);
    (void)pg_sphere();
    (void)pg_close();
    (void)pg_close();

    (void)pg_open("universe");
    (void)pg_child("mygob");
    (void)pg_close();
    initialized= 1;
  }
}

static void cball_redraw( gTransfm& trans )
{
  cball_draw_init();
  model_trans= trans * model_trans;
  (void)pg_open("universe");
  (void)pg_transform((P_Transform*)(model_trans.floatrep()));
  (void)pg_child("mygob");
  (void)pg_close();
  (void)pg_snap( "universe", "mylights", "mycamera" );
}

static void cball_draw_shutdown()
{
  (void)pg_shutdown();
}

static void steering_motion_cb( gTransfm *roll, void *cb_data,
				      CBall *calling_cball )
{
  cball_redraw( *roll );
}

static void create_cb(Widget w,
		      int *id,
		      unsigned long *reason) {
    switch (*id) {
    case k_open_id:
	open_id = w;
	break;
    case k_tfun_open_id:
        tfun_open_id= w;
	break;
    case k_canvas_id:
	canvas_id = w;
	break;
    case k_quality_settings_id:
	quality_settings_id = w;
	break;
    case k_renderer_controls_id:
	renderer_controls_id = w;
	break;
    case k_debugger_controls_id:
	debugger_controls_id = w;
	break;
    case k_logger_id:
	logger = new motifLogger(argv[0], (void *)w, XmNlabelString);
        break;
    case k_photo_id:
	break;
    case k_steering_box_id:
	steering_box_id= w;
	cball= new XmCBall( w, steering_motion_cb, NULL );
	break;
    case k_dataset_label_id:
	dataset_label_id= w;
	break;
    case k_tfun_label_id:
	tfun_label_id= w;
	break;
    case k_grad_tfun_open_id:
        grad_tfun_open_id= w;
	break;
    case k_grad_tfun_label_id:
	grad_tfun_label_id= w;
	break;
    case k_image_save_id:
	image_save_id= w;
	break;
    case k_data_xsize_text_id:
	data_xsize_text_id= w;
	break;
    case k_data_ysize_text_id:
	data_ysize_text_id= w;
	break;
    case k_data_zsize_text_id:
	data_zsize_text_id= w;
	break;
    case k_opac_length_id:
	opac_length_id= w;
	break;
    case k_tfun_dlog_id:
	tfun_dlog_id= w;
	break;
    case k_tfun_save_id:
	tfun_save_id= w;
	break;
    case k_tfun_dlog_image_id:
	tfun_dlog_image_id= w;
	break;
    case k_tfun_dlog_edit_id:
	tfun_dlog_edit_id= w;
	break;
    case k_grad_tfun_save_id:
	grad_tfun_save_id= w;
	break;
    case k_grad_tfun_dlog_image_id:
	grad_tfun_dlog_image_id= w;
	break;
    case k_grad_tfun_dlog_edit_id:
	grad_tfun_dlog_edit_id= w;
	break;
    case k_grad_tfun_dlog_id:
	grad_tfun_dlog_id= w;
	break;
      default:
	fprintf(stderr,"create_cb got unknown id %d\n",*id);
      }
  }


static void do_render_cb (Widget w,
			  XtPointer foo,
			  XtPointer bar) {
// Do a render
  static Widget no_d_loaded_dlog= NULL;
  if (!datavol_present_flag) { // No data file yet loaded
      MrmFetchWidget (mrm_id, "no_d_loaded_dlog", app_shell, 
		      &no_d_loaded_dlog, &mrm_class);
      if (no_d_loaded_dlog)
	XtManageChild(no_d_loaded_dlog);
      return;
  }
  if (svol_ready_flag) { // new datavol just got loaded
    logger->comment("Precalculations begin");
    svol_ready_flag= 1;
    logger->comment("Precalculations complete");
  }
  logger->comment("Settings begin");
  logger->comment("Settings complete; beginning render");
  logger->comment("Render complete");
}

static void canvas_resize_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
  if (datavol_present_flag) do_render_cb(w, foo, bar);
}

static void canvas_expose_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
  static int inited = False;
    
  if (! inited) {
    inited = True;
    
    // Set the top window property so that the window manager pays some
    // attention to the canvas color map.
    Atom atomcolormapwindows= 
      XInternAtom(XtDisplay(app_shell), "WM_COLORMAP_WINDOWS", False);
    
    Window canvas_win = XtWindow(canvas_id);
    XChangeProperty( XtDisplay(app_shell), XtWindow(app_shell), 
		     atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		     (unsigned char *)&canvas_win, 1 );
  } 
}

static void steering_box_expose_cb (Widget w, int *uil_id, caddr_t call_data)
{
  cball_redraw( gTransfm::identity );
}

static void help_cb (Widget w,
		     char *text,
		     XtPointer cb) {
  Arg args[MAX_ARGS];
  int n;
  Widget help_widget= NULL;
  MrmType widget_class= NULL;
  
  if (text == NULL) return;

  MRMRegisterArg names;
  names.name= "help_text_id";
  names.value= text;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  MrmFetchWidget(mrm_id, "help_dialog", app_shell, 
		 &help_widget, &widget_class);
  XtManageChild(help_widget);
}

static void show_renderer_controls_cb (Widget w, XtPointer foo, XtPointer cb) {
  if (renderer_controls_id)
    XtManageChild(renderer_controls_id);
}

static void show_debugger_controls_cb (Widget w, XtPointer foo,
				       XmToggleButtonCallbackStruct *cb) {
  if (debugger_controls_id)
      if (showing_debugger = cb->set)
	  XtManageChild(debugger_controls_id);
      else
	  XtUnmanageChild(debugger_controls_id);
}

static void debug_shoot_ray_cb (Widget w, XtPointer foo,
				XmToggleButtonCallbackStruct *cb) {
  debug_shooting= 1;
}

static void canvas_input_cb(Widget w, int *uil_id, 
			  XmDrawingAreaCallbackStruct *event_data) {
  fprintf(stderr,"Got canvas input!\n");
    if (debug_shooting) {
      switch (event_data->event->type) {
      case ButtonPress:
	fprintf(stderr,"Button press\n");
	break;
      case ButtonRelease:
	fprintf(stderr,"Button release\n");
	break;
      }
    }
}

static void set_opac_limit_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
}

static void set_color_comp_error_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
}

static void set_grad_comp_error_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
}

static void set_grad_mag_error_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
}

static void exit_cb (Widget w,
		     XtPointer data,
		     XtPointer cb) {
  cball_draw_shutdown();
  exit(0);
}

static void open_file_cb (Widget w,
			  XtPointer blah,
			  XmFileSelectionBoxCallbackStruct *call_data) {
    char *the_name, *label_name, *s;

    if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
	return;
    
    for (s = the_name; *s; s++)
      if (*s == '/')
	label_name = s+1;

    if (! *label_name) {
      logger->comment("Not a valid image file.");
      return;
    }

    // Set the value of the text string giving the label
    if (dataset_label_id) {
      Arg args[MAX_ARGS];
      int n;
      XmString label_name_string= NULL;
      label_name_string= XmStringCreate(label_name, XmSTRING_DEFAULT_CHARSET);
      n= 0;
      XtSetArg(args[n], XmNlabelString, label_name_string); n++;
      XtSetValues(dataset_label_id, args, n);
      if (label_name_string) XmStringFree(label_name_string);
    }
    
    XtUnmanageChild(w);
    XmUpdateDisplay(w);

    datavol_present_flag= 1;

    logger->comment("Dataset loaded");
}

static void open_tfun_file_cb (Widget w,
			       XtPointer blah,
			       XmFileSelectionBoxCallbackStruct *call_data) {
    char *the_name, *label_name, *s;

    if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
	return;
    
    for (s = the_name; *s; s++)
      if (*s == '/')
	label_name = s+1;

    if (! *label_name) {
      logger->comment("Not a valid tfun file.");
      return;
    }

    // Set the value of the text string giving the label
    if (tfun_label_id) {
      Arg args[MAX_ARGS];
      int n;
      XmString label_name_string= NULL;
      label_name_string= XmStringCreate(label_name, XmSTRING_DEFAULT_CHARSET);
      n= 0;
      XtSetArg(args[n], XmNlabelString, label_name_string); n++;
      XtSetValues(tfun_label_id, args, n);
      if (label_name_string) XmStringFree(label_name_string);
    }
    
    XtUnmanageChild(w);
    XmUpdateDisplay(w);

    tfun_ready_flag= 1;

    logger->comment("Tfun loaded");
}

static void open_cb (Widget w,
		     int *tag,
		     caddr_t cb) {
  switch (*tag) {
  case k_open_id:
    XtManageChild( open_id );
    break;
  case k_tfun_open_id:
    XtManageChild( tfun_open_id );
    break;
  case k_grad_tfun_open_id:
    XtManageChild( grad_tfun_open_id );
    break;
  case k_image_save_id:
    XtManageChild( image_save_id );
    break;
  case k_quality_settings_id:
    XtManageChild( quality_settings_id );
    break;
  case k_renderer_controls_id:
    XtManageChild( renderer_controls_id );
    break;
  case k_tfun_dlog_id:
    XtManageChild( tfun_dlog_id );
    break;
  case k_tfun_save_id:
    XtManageChild( tfun_save_id );
    break;
  case k_grad_tfun_save_id:
    XtManageChild( grad_tfun_save_id );
    break;
  case k_grad_tfun_dlog_id:
    XtManageChild( grad_tfun_dlog_id );
    break;
  default:
    fprintf(stderr,"open_cb got unknown tag %d\n",*tag);
  }
}

static void photocopy_cb (Widget w,
			  XtPointer foo,
			  XtPointer bar) {
    Widget photo = NULL;
    if (MrmFetchWidget (mrm_id, "photocopy_dlog", app_shell,
			&photo, &mrm_class)) XtManageChild(photo);
}

static void set_filetype_cb (Widget w,
			     int *type,
			     XtPointer bar) {
  file_type = *type;
}

static void close_help_cb( Widget w, int *type, caddr_t call_data )
{
  XtDestroyWidget( XtParent( XtParent( XtParent( w ) ) ) );
}

static void close_dialog_cb( Widget w, int *type, caddr_t call_data )
{
  XtUnmanageChild( XtParent( XtParent( w ) ) );
}

static MRMRegisterArg mrm_names[] = {
    {"create_cb",  (caddr_t) create_cb},
    {"help_cb",  (caddr_t) help_cb},
    {"canvas_resize_cb",  (caddr_t) canvas_resize_cb},
    {"canvas_expose_cb",  (caddr_t) canvas_expose_cb},
    {"open_cb",  (caddr_t) open_cb},
    {"open_file_cb",  (caddr_t) open_file_cb},
    {"open_tfun_file_cb", (caddr_t) open_tfun_file_cb},
    {"exit_cb",  (caddr_t) exit_cb},
    {"do_render_cb",  (caddr_t) do_render_cb},
    {"show_renderer_controls_cb",  (caddr_t) show_renderer_controls_cb},
    {"show_debugger_controls_cb",  (caddr_t) show_debugger_controls_cb},
    {"debug_shoot_ray_cb",  (caddr_t) debug_shoot_ray_cb},
    {"canvas_input_cb",  (caddr_t) canvas_input_cb},
    {"steering_box_expose_cb", (caddr_t) steering_box_expose_cb},
    {"set_opac_limit_cb",  (caddr_t) set_opac_limit_cb},
    {"set_color_comp_error_cb",  (caddr_t) set_color_comp_error_cb},
    {"set_grad_comp_error_cb",  (caddr_t) set_grad_comp_error_cb},
    {"set_grad_mag_error_cb",  (caddr_t) set_grad_mag_error_cb},
    {"photocopy_cb", (caddr_t) photocopy_cb},
    {"set_filetype_cb", (caddr_t) set_filetype_cb},
    {"close_help_cb", (caddr_t)close_help_cb},
    {"num_text_check_cb", (caddr_t)num_text_check_cb},
    {"close_dialog_cb", (caddr_t)close_dialog_cb}
};

int main(int argc,
	 char *argv_in[]) {
  argv = argv_in;
  Display *display;
  Widget app_main = NULL;
  Arg args[3];
  
  char *progname = argv[0];
  
  fprintf(stderr, "Starting...\n");
  
  MrmInitialize ();
  
  XtToolkitInitialize();
  app_context = XtCreateApplicationContext();
  display = XtOpenDisplay(app_context, NULL, "Vfleet", "Vfleet",
			  NULL, 0, &argc, argv);
  
  if (display == NULL) {
    fprintf(stderr, "%s:  Can't open display\n", progname);
    exit(1);
  }
  
  XtSetArg (args[0], XtNallowShellResize, True);
  app_shell = XtAppCreateShell("Vfleet", "Vfleet", applicationShellWidgetClass,
			       display, args, 1);
  
  if (MrmOpenHierarchy (1, mrm_vec, NULL, &mrm_id) != MrmSUCCESS) {
    fprintf(stderr, 
	    "%s:  Can't find the User Interface Definition file\n",
	    progname);
    exit(1);
  }
  
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));
  MrmFetchWidget (mrm_id, "app_main", app_shell, &app_main, &mrm_class);
  XtManageChild(app_main);
  XFlush(XtDisplay(app_main));
  XtRealizeWidget(app_shell);
  
  XtAppMainLoop(app_context);
  delete logger;
  
  return(0);
}
