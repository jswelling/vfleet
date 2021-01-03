/****************************************************************************
 * feet.cc for pvm 2.4
 * Author Robert Earhart
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

/* feet.cc
   A really cool way to view volumetric data under Mootif.
   
   This program really ought to do Everything in the world.*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/time.h>

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

#include <im.h>
#include "crystal.h"

#include "datafile.h"
#include "basenet.h"
#include "vren.h"
#include "tfun.h"
#include "logger.h"
#include "servman.h"
#include "rgbimage.h"
#include "raycastvren.h"
#include "imagehandler.h"
#include "ximagehandler.h"

// Here begin the shared integer declarations for communication with the UI.
const int k_help_id=1;
const int k_open_id=2;
const int k_canvas_id=3;
const int k_renderer_controls_id=4;
const int k_debugger_controls_id=5;
const int k_logger_id=6;
const int k_dataset_list_id=7;
const int k_photo_id=8;

const int hdf_file_type=1;

// This bunk needs fixed.

static int file_type = hdf_file_type; // Global file type? ICK.

static baseLogger *logger= NULL;
static XImageHandler *main_ihandler= NULL;
static raycastVRen *vren= NULL;
static DataVolume *datavol= NULL;
static baseSampleVolume *svol= NULL;
static VolGob *volgob= NULL;
static MethodTransferFunction *method_tfun= NULL;
static BoundBoxTransferFunction *bbox_tfun= NULL;
static SumTransferFunction *sum_tfun= NULL;

static Boolean debug_shooting = False;
static Boolean showing_debugger = False;

static float opac_limit_in = .9;
static float color_comp_in = .05;
static float grad_comp_in = .05;
static float grad_mag_in = .05;

// These are a few misc MRM declarations that really ought to be global.
static MrmHierarchy mrm_id;
static char *mrm_vec[]={"feet.uid"};
static MrmCode mrm_class;

// Storage space for the various widgets the UI tells us about.
static Widget help_id = NULL;
static Widget open_id = NULL;
static Widget canvas_id = NULL;
static Widget renderer_controls_id = NULL;
static Widget debugger_controls_id = NULL;
static Widget dataset_list_id = NULL;

// Our main dialog shell
static Widget app_shell = NULL;

char **argv;

// some procedures
static void error_handler( int error_id, baseVRen *renderer )
{
  fprintf(stderr,"Just got error %d from a renderer\n", error_id);
}

static void fatal_handler( int error_id, baseVRen *renderer )
{
  fprintf(stderr,"Just got fatal error %d from a renderer; exiting!\n",
	  error_id);
  exit(-1);
}

static void tfun_method( Sample& smpl, int i, int j, int k, int ndata,
			 DataVolume *data_table )
// Note that generated colors must be multiplied by alpha, because
// the color accumulation algorithm assumes pre-multiplication.
{
  if (ndata<1) {
    fprintf(stderr,"tfun_method: %d is not enough data volumes!\n",ndata);
    exit(-1);
  }
  
  float r, g, b, alpha, value;
  value= data_table->fval(i,j,k);
  if (value < 0.1)
    r= g= b= alpha= 0.0;
  else if (value < 0.3) {
    alpha= 0.3*value;
    b= 1.0;
    r= g= 0.0;
  }
  else if (value < 0.7) {
    alpha= value;
    r= b= 0.0;
    g= 1.0;
  }
  else {
    alpha= 1.0;
    r= b= 1.0;
    g= 0.0;
  }
 
  smpl.clr= gBColor( r, g, b, alpha );
  smpl.grad= gBVector( data_table->gradient( i, j, k ),
		       data_table->max_gradient_magnitude() );
  smpl.grad.normalize(); // can normalize a gBVector of any length
  if (i==32&&j==32&&k==32) {
    fprintf(stderr,"tfun_method: in center, set (%f %f %f %f) (%f %f %f)\n",
	    smpl.clr.r(),smpl.clr.g(),smpl.clr.b(),smpl.clr.a(),
	    smpl.grad.x(),smpl.grad.y(),smpl.grad.z());
  }
}

static void make_sheet( DataType *buffer,
			int k,
			int xdim,
			int ydim,
			int zdim ) {
    int i, j;
    float x, y, z, r= 0.5, val;
    // Throw together some data to render
    
    z= (float)(k - (zdim/2))/(float)zdim;
    for (i=0; i<xdim; i++) {
	x= (float)(i - (xdim/2))/(float)xdim;
	for (j=0; j<ydim; j++) {
	    y= (float)(j - (ydim/2))/(float)ydim;
	    val= 0.5 - sqrt( x*x + y*y + z*z );
	    if (val>1.0) val= 1.0;
	    if (val<0.0) val= 0.0;
	    *(buffer + i*ydim + j)= 
		(DataType)((DataTypeMax-DataTypeMin)*val + DataTypeMin);
	}
    }
}


// Callbacks for the UI.
static void photo_expose_cb(Widget w,
			   XImageHandler *photo,
			   unsigned long *reason) {
    photo->redraw();
}

static void photo_set_cb(Widget w,
			   rgbImage *image,
			   unsigned long *reason) {
    XImageHandler *photo_handler =
	new XImageHandler(XtDisplay(w), DefaultScreen(XtDisplay(w)),
			  XtWindow(w));
    photo_handler->display(image, True);
    XtRemoveCallback(w, XmNexposeCallback, (XtCallbackProc)photo_set_cb,
		     (XtPointer)image);
    XtAddCallback(w, XmNexposeCallback, (XtCallbackProc)photo_expose_cb,
		  (XtPointer)photo_handler);
}

static void create_cb(Widget w,
		      int *id,
		      unsigned long *reason) {
    switch (*id) {
    case k_help_id:
	help_id = w;
        XtUnmanageChild(XmMessageBoxGetChild(help_id,
					     XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(help_id,
					     XmDIALOG_HELP_BUTTON));
	break;
    case k_open_id:
	open_id = w;
	break;
    case k_canvas_id:
	canvas_id = w;
	break;
    case k_renderer_controls_id:
	renderer_controls_id = w;
	break;
    case k_debugger_controls_id:
	debugger_controls_id = w;
	break;
    case k_logger_id:
	baseLogger *loggers[2];
	loggers[0] = new fileLogger("test.log", argv[0]);
	loggers[1] = new motifLogger(argv[0], (void *)w, XmNlabelString);
	logger = new multiLogger(argv[0], 2, loggers);
    case k_dataset_list_id:
	dataset_list_id = w;
	break;
    case k_photo_id:
	{
	    XtAddCallback(w, XmNexposeCallback, (XtCallbackProc)photo_set_cb,
			  (XtPointer)main_ihandler->last_image());
	}
	break;
      }
  }

static void do_render_cb (Widget w,
			  XtPointer foo,
			  XtPointer bar) {
// Do a render
  static Widget no_d_loaded_dlog= NULL;
  gBoundBox bbox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 );
  if (!datavol) { // No data file yet loaded
      MrmFetchWidget (mrm_id, "no_d_loaded_dlog", app_shell,
		      &no_d_loaded_dlog, &mrm_class);
    if (no_d_loaded_dlog)
	XtManageChild(no_d_loaded_dlog);
    return;
  }
  if (!svol) { // new datavol just got loaded
    logger->comment("Precalculations begin");
    if (! vren) { // Build the renderer
	vren= new raycastVRen( 64, 64, 64, logger, main_ihandler,
			       error_handler, fatal_handler, NULL );
	vren->setQualityMeasure( QualityMeasure(opac_limit_in, color_comp_in,
						grad_comp_in, grad_mag_in) );
    }
    svol= vren->create_sample_volume( bbox, *sum_tfun, 1, &datavol );
    volgob= vren->create_volgob( svol, gTransfm::identity );
    logger->comment("Precalculations complete");
  }
  logger->comment("Settings begin");
  vren->setCamera( gPoint( 2.0, 2.0, 8.0 ), gPoint( 0.0, 0.0, 0.3 ), 
		   gVector( 0.0, 1.0, 0.0 ), 15.0, -2.0, -20.0 );
  vren->setLightInfo( LightInfo( gColor( 1.0, 1.0, 1.0 ),
			       gBVector( 0.0, -1.0, 0.0, 1.0 ), 
			       gColor( 0.6, 0.6, 0.6 ) ) );
  vren->setGeometry( volgob );
  logger->comment("Settings complete; beginning render");
  vren->StartRender( main_ihandler->xsize(), main_ihandler->ysize() );
  logger->comment("Render complete");
}

static void canvas_resize_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
  if (datavol) do_render_cb(w, foo, bar);
}

static void canvas_expose_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
    static int inited = False;
    
    if (! inited) {
	inited = True;
	main_ihandler= new XImageHandler( XtDisplay(canvas_id),
					  DefaultScreen(XtDisplay(canvas_id)), 
					  XtWindow(canvas_id) );
	
	// Set the top window property so that the window manager pays some
	// attention to the canvas color map.
	Atom atomcolormapwindows= 
	    XInternAtom(XtDisplay(app_shell), "WM_COLORMAP_WINDOWS", False);
	
	Window canvas_win = XtWindow(canvas_id);
	XChangeProperty( XtDisplay(app_shell), XtWindow(app_shell), 
			 atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
			 (unsigned char *)&canvas_win, 1 );
	
	method_tfun= new MethodTransferFunction( 1, tfun_method );
	bbox_tfun= new BoundBoxTransferFunction(64);
	BaseTransferFunction *tfun_list[2];
	tfun_list[0] = method_tfun;
	tfun_list[1] = bbox_tfun;
	sum_tfun= new SumTransferFunction(1, 2, tfun_list);
    } else
	main_ihandler->redraw();
}

static void help_cb (Widget w,
		     XmString name,
		     XtPointer cb) {
    Arg args[1];
    
    if (help_id == NULL) return;
    if (name == NULL) return;
    XtSetArg (args[0], XmNmessageString, name);
    XtSetValues(help_id, args, 1);
    XtManageChild(help_id);
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
    static Cursor cross_hair = NULL;
    if (canvas_id) 
	if (debug_shooting = cb->set) {
	    if (! cross_hair)
		cross_hair = XCreateFontCursor(XtDisplay(canvas_id), XC_cross_reverse);
	    XDefineCursor(XtDisplay(canvas_id), XtWindow(canvas_id), cross_hair);
	} else
	    XUndefineCursor(XtDisplay(canvas_id), XtWindow(canvas_id));
}

static void canvas_pointer_down_cb(Widget w, XtPointer foo, XEvent *event) {
    if (debug_shooting) {
	printf("\n\n\n\nShooting at %d, %d.\n", event->xbutton.x,
	       main_ihandler->ysize() - event->xbutton.y);
	vren->TraceOneRay(event->xbutton.x, main_ihandler->ysize() - event->xbutton.y, True);
	main_ihandler->redraw();
    }
}

static void set_opac_limit_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
    if (vren)
      vren->setOpacLimit(((float)value)/((float)max_value));
    else
      opac_limit_in = ((float)value)/((float)max_value);
}

static void set_color_comp_error_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
    if (vren)
      vren->setColorCompError(((float)value)/((float)max_value));
    else
      color_comp_in = ((float)value)/((float)max_value);
}

static void set_grad_comp_error_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
    if (vren)
      vren->setGradCompError(((float)value)/((float)max_value));
    else
      grad_comp_in = ((float)value)/((float)max_value);
}

static void set_grad_mag_error_cb (Widget w, XtPointer foo, XmScaleCallbackStruct *cb) {
    Arg args[2];
    int max_value, value;
    XtSetArg (args[0], XmNmaximum, &max_value);
    XtSetArg (args[1], XmNvalue, &value);
    XtGetValues(w, args, 2);
    if (vren)
      vren->setGradMagError(((float)value)/((float)max_value));
    else
      grad_mag_in = ((float)value)/((float)max_value);
}

static void exit_cb (Widget w,
		     XtPointer data,
		     XtPointer cb) {
    exit(0);
}

static void open_file_cb (Widget w,
			  XtPointer blah,
			  XmFileSelectionBoxCallbackStruct *call_data) {
    char *the_name, *label_name, *s;
    baseDataFile *fp;

    if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
	return;
    
    for (s = the_name; *s; s++)
      if (*s == '/')
	label_name = s+1;

    if (! *label_name) {
      logger->comment("Not a valid image file.");
      return;
    }

    XmListAddItem(dataset_list_id, XmStringCreateSimple(label_name), 0);
    
    logger->comment(the_name);
    
    switch(file_type) {
    case hdf_file_type:
	fp = new hdfDataFile(the_name);
	break;
    default:
	fp = NULL;
    }
    if (! fp || ! fp->valid()) {
	Widget bad_file_dlog = NULL;
	logger->comment("File / Format mismatch.");
	if (MrmFetchWidget (mrm_id, "bad_file_dlog", app_shell,
			    &bad_file_dlog, &mrm_class)) {
	    XtManageChild(bad_file_dlog);
	}
	return;
    }
    
    XtUnmanageChild(w);
    XmUpdateDisplay(w);

    /* Okay... so fp points to valid data... now to read it in... */

    int xdim= 64;
    int ydim= 64;
    int zdim= 64;
    gBoundBox bbox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 );
    
    delete datavol;
    delete svol;  // since new datavol will require new svol
    delete volgob;
    svol= NULL;  // serves as flag to cause regen of svol and volgob
    datavol= vren->create_data_volume( xdim, ydim, zdim, bbox );
    
    for (int k=0; k<zdim; k++)
        datavol->load_zplane( fp->get_zplane(k, baseDataFile::ByteU), k);
    
    //delete(fp);

    datavol->finish_init();
    
    logger->comment("Dataset loaded");
}

static void open_cb (Widget w,
		     XmString name,
		     XtPointer cb) {
    if (open_id == NULL) return;
    XtManageChild(open_id);
}

static void photocopy_cb (Widget w,
			  XtPointer foo,
			  XtPointer bar) {
    Widget photo = NULL;
    if (MrmFetchWidget (mrm_id, "photocopy_dlog", app_shell,
			&photo, &mrm_class)) XtManageChild(photo);
}

static void dataset_edit_cb (Widget w,
			     XtPointer foo,
			     XtPointer bar) {
  fprintf(stderr, "Editing...\n");
}

static void set_filetype_cb (Widget w,
			     int *type,
			     XtPointer bar) {
  file_type = *type;
}

static MRMRegisterArg mrm_names[] = {
    {"create_cb",  (caddr_t) create_cb},
    {"help_cb",  (caddr_t) help_cb},
    {"canvas_resize_cb",  (caddr_t) canvas_resize_cb},
    {"canvas_expose_cb",  (caddr_t) canvas_expose_cb},
    {"open_cb",  (caddr_t) open_cb},
    {"open_file_cb",  (caddr_t) open_file_cb},
    {"exit_cb",  (caddr_t) exit_cb},
    {"do_render_cb",  (caddr_t) do_render_cb},
    {"show_renderer_controls_cb",  (caddr_t) show_renderer_controls_cb},
    {"show_debugger_controls_cb",  (caddr_t) show_debugger_controls_cb},
    {"debug_shoot_ray_cb",  (caddr_t) debug_shoot_ray_cb},
    {"canvas_pointer_down_cb",  (caddr_t) canvas_pointer_down_cb},
    {"set_opac_limit_cb",  (caddr_t) set_opac_limit_cb},
    {"set_color_comp_error_cb",  (caddr_t) set_color_comp_error_cb},
    {"set_grad_comp_error_cb",  (caddr_t) set_grad_comp_error_cb},
    {"set_grad_mag_error_cb",  (caddr_t) set_grad_mag_error_cb},
    {"photocopy_cb", (caddr_t) photocopy_cb},
    {"dataset_edit_cb", (caddr_t) dataset_edit_cb},
    {"set_filetype_cb", (caddr_t) set_filetype_cb}
};

int main(unsigned int argc,
	 char *argv_in[]) {
    argv = argv_in;
    Widget app_main = NULL;
    Display *display;
    Arg args[3];
    XtAppContext app_context;

    char *progname = argv[0];

    fprintf(stderr, "Starting...\n");
    
    logger = NULL;
    
    MrmInitialize ();
    CrystalMrmInitialize ();

    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();
    display = XtOpenDisplay(app_context, NULL, "Feet", "Feet",
			    NULL, 0, &argc, argv);

    if (display == NULL) {
	fprintf(stderr, "%s:  Can't open display\n", progname);
        exit(1);
    }

    XtSetArg (args[0], XtNallowShellResize, True);
    app_shell = XtAppCreateShell("Feet", "Feet", applicationShellWidgetClass,
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
    XtRealizeWidget(app_shell);

    XtAppMainLoop(app_context);
    delete vren;
    delete bbox_tfun;
    delete method_tfun;
    delete sum_tfun;
    delete main_ihandler;
    delete logger;

    return(0);
}
