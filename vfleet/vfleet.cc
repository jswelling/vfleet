/****************************************************************************
 * vfleet.cc
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

/* vfleet.cc */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#if ( ATTCC || CRAY_ARCH_C90 )
#include <signal.h>
#endif
#include <sys/time.h>
#ifdef ATTCC
#include <osfcn.h>
#define F_OK 0
#define R_OK 4
#else
#include <unistd.h>
#endif
#include <ctype.h>
#include <math.h>

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
#include <Xm/TextF.h>
#include <Xm/Scale.h>
#include <Xm/ToggleBG.h>
#include <Xm/SelectioB.h>
#include <Mrm/MrmAppl.h>

#include "vfleet.h"

#include "datafile.h"
#include "xlogger.h"
#include "raycastvren.h"
#ifndef LOCAL_VREN
#include "basenet.h"
#include "netlogger.h"
#include "servman.h"
#include "netvren.h"
#endif
#include "logimagehandler.h"
#include "xdrawih.h"
#include "cball.h"
#include "xcball.h"
#include "tinydraw.h"

// This is the environment variable name used for file loading
const char* ENV_HOME_DIR="VFLEET_ROOT";

// This is the name of the file to which logging info is written
static char* LOG_FILE_NAME="/dev/null";

// This is the time between calls to the baseNet::service routine.
const int service_wait_time= 100;
static XtIntervalId service_interval_id;

const int hdf_file_type=1;
const int pghmri_file_type=2;

int num_remote_procs= 0; // 0 in local mode
int num_threads; // 0 in single_thread

int render_in_progress= 0; // non-zero between render req and image arrival

float current_opac_scale= 1.0;

gVector current_lighting_dir;

static char* initial_tcl_script= NULL; // command line arg tcl script

#ifdef INCL_HDF
static int file_type = hdf_file_type;
static int remote_file_type= hdf_file_type;
#else
#ifdef INCL_FIASCO
static int file_type = pghmri_file_type;
static int remote_file_type= pghmri_file_type;
#else
static int file_type = hdf_file_type;
static int remote_file_type= hdf_file_type;
#endif
#endif
char *current_image_save_type= NULL;

baseLogger *logger= NULL;
XImageHandler *main_ihandler= NULL;
baseVRen *main_vren= NULL;
dList<dvol_struct*> datavol_list;
baseSampleVolume *main_svol= NULL;
VolGob *main_volgob= NULL;

static Boolean debug_shooting = False;
static Boolean showing_debugger = False;

static void update_datavol_lists();
static void register_and_load_data( baseDataFile* datafile,
				   gBoundBox& new_boundbox, 
				   char* label_name,
				   const int replace_which );

// These are a few misc MRM declarations that really ought to be global.
MrmHierarchy mrm_id;
static char *mrm_vec[]={"vfleet.uid"};
static MrmType mrm_class;

// Storage space for the various widgets the UI tells us about.
static XtAppContext app_context;
static Widget file_menu_pb_id= NULL;
static Widget open_id = NULL;
static Widget open_remote_id = NULL;
static Widget free_datavol_id= NULL;
static Widget canvas_id = NULL;
static Widget datavol_list_id = NULL;
static Widget debugger_controls_id = NULL;
static Widget image_save_id= NULL;
static Widget tcl_script_id= NULL;
static Widget data_xsize_text_id= NULL;
static Widget data_ysize_text_id= NULL;
static Widget data_zsize_text_id= NULL;
static Widget remote_load_dir_id= NULL;
static Widget remote_load_file_id= NULL;
static Widget rmt_dta_xsize_text_id= NULL;
static Widget rmt_dta_ysize_text_id= NULL;
static Widget rmt_dta_zsize_text_id= NULL;

// Our main dialog shell
Widget app_shell = NULL;

// Size of the canvas window
static Dimension current_canvas_width;
static Dimension current_canvas_height;

// The crystal ball controller, and tools for drawing the ball model
static CBall *cball= NULL;
static XDrawImageHandler* cball_ihandler= NULL;
static TDrawGeom* cball_geom= NULL;
static TDrawEngine* cball_engine= NULL;
static TDrawLights* cball_lights= NULL;

char **argv;

#ifdef SC94
// Special code for SC'94 demo, to tie in to PVM AIM app
unsigned char* recv_ext_dataset (int buf_id, char *filename, 
				 int* ix, int* iy, int* iz)
{
  unsigned char *b_data;

  pvm_recv (-1, 1002);		/*  Info message  */
  pvm_upkstr (filename);
  pvm_upkint (ix, 1, 1);
  pvm_upkint (iy, 1, 1);
  pvm_upkint (iz, 1, 1);

  b_data= new unsigned char[ *ix * *iy * *iz ];

  pvm_recv (-1, 1003);		/*  Receive the data  */
  pvm_upkbyte ((char*)b_data, *ix * *iy * *iz, 1);

  return (b_data);
}  

void check_extern_pvm_event()
{
  char new_filename[100];
  int ix, iy, iz;
  int buf_id;
  unsigned char* data_ptr;

  buf_id = pvm_nrecv (-1, 1001);
  if (buf_id > 0) {
    data_ptr = recv_ext_dataset (buf_id, new_filename, &ix, &iy, &iz);
    char msgbuf[128];
    sprintf(msgbuf,"got PVM dataset <%s>",new_filename);
    logger->comment(msgbuf);

    gBoundBox new_boundbox;

    // Check to make sure the new dataset matches any previous datafiles
    // in dimensions and physical size
    DataVolume *existing_datavol= NULL;
    if ( datavol_list.head() ) { // Not first dvol
      existing_datavol= datavol_list.head()->dvol;
      new_boundbox= existing_datavol->boundbox();
      if ((existing_datavol->xsize() != ix)
	  || (existing_datavol->ysize() != iy)
	  || (existing_datavol->zsize() != iz)) {
	// New dataset does not match old
	pop_error_dialog("incommensurate_datavols_msg");
	delete [] data_ptr;
	return;
      }
    }
    else {
      // There is no previously existing datavol- guess boundbox size
      int max_dim= ix;
      if (iy>max_dim) max_dim= iy;
      if (iz>max_dim) max_dim= iz;
      float x_width= (float)ix/(float)max_dim;
      float y_width= (float)iy/(float)max_dim;
      float z_width= (float)iz/(float)max_dim;
      new_boundbox= gBoundBox( -x_width/2, -y_width/2, -z_width/2,
			      x_width/2, y_width/2, z_width/2 );
    }
      
    register_and_load_data( datafile, new_boundbox, new_filename, -1 );

    logger->comment("Dataset loaded");
    
    delete [] data_ptr;
  }
}

#endif

// Methods for things defined in vfleet.h
dvol_struct::dvol_struct( char *name_in, baseDataFile *file_in,
			  DataVolume *dvol_in )
{
  file= file_in;
  dvol= dvol_in;
  name= new char[strlen(name_in)+1];
  strcpy(name,name_in);
  xstring= XmStringCreateSimple(name);
}

dvol_struct::~dvol_struct()
{ 
  XmStringFree(xstring);
  delete name; 
  delete dvol;
  delete file;
}


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

static void logihandler_dpy_cb( baseImageHandler* caller )
{
  render_in_progress= 0;

  // Post a harmless event, to break us out of any event wait loop
  // that may be in progress.
  static Atom useless_atom= 0;
  if (!useless_atom) {
    useless_atom= 
      XInternAtom(XtDisplay(app_shell), "VFLEET_USELESS_EVENT", False);
    
    static const unsigned char junk[4]= {0,0,0,0};
    XChangeProperty( XtDisplay(app_shell), XtWindow(app_shell), 
		     useless_atom, XA_WINDOW, 8, PropModeAppend, 
		     junk, 4 );
  }
  XClientMessageEvent xevent;
  xevent.type= ClientMessage;
  xevent.display= XtDisplay(canvas_id);
  xevent.message_type= useless_atom;
  xevent.format= 8;
  (void)XSendEvent( XtDisplay(canvas_id), XtWindow(canvas_id), False,
		   0, (XEvent*)&xevent );
}

static void renderer_ready_cb( baseVRen* renderer, void* cb_data )
{
  if (file_menu_pb_id) XtSetSensitive(file_menu_pb_id, True);
  if (logger) logger->comment( "Renderer ready");
  if (initial_tcl_script) vfleet_tcl_expect_script(initial_tcl_script);
}

static void create_cb(Widget w,
		      int *id,
		      unsigned long *reason) 
{
  switch (*id) {
  case k_file_menu_pb_id:
    file_menu_pb_id= w;
  case k_open_id:
    open_id = w;
    break;
  case k_open_remote_id:
    open_remote_id = w;
    break;
  case k_open_remote_pb_id:
#ifndef LOCAL_VREN
      if (num_remote_procs > 0) {
	XtSetSensitive( w, True );
      }
#endif
    break;
  case k_free_datavol_id:
    free_datavol_id= w;
    XtUnmanageChild( XmSelectionBoxGetChild(free_datavol_id,
					    XmDIALOG_APPLY_BUTTON) );
    break;
  case k_canvas_id:
    {
      canvas_id = w;
      Arg args[2];      
      int n= 0;
      XtSetArg(args[n],XmNwidth, &current_canvas_width); n++;
      XtSetArg(args[n], XmNheight, &current_canvas_height); n++;
      XtGetValues( canvas_id, args, n );
    }
    break;
  case k_debugger_controls_id:
    debugger_controls_id = w;
    break;
  case k_logger_id:
    {
      baseLogger *loggers[2];
#ifdef LOCAL_VREN
      loggers[0] = new fileLogger(LOG_FILE_NAME, argv[0]);
#else
      if (num_remote_procs == 0) {
	loggers[0] = new fileLogger(LOG_FILE_NAME, argv[0]);
      }
      else {
	loggers[0]= new netLogger(argv[0]);
	// ((netLogger *)loggers[0])->debug_on();
      }
#endif
      loggers[1] = new motifLogger(argv[0], (void *)w, XmNlabelString);
      logger = new multiLogger(argv[0], 2, loggers);
      if ((!main_vren) && main_ihandler) {
	// this or ihandler creation must happen first
#ifdef LOCAL_VREN
	main_vren= new raycastVRen( logger,
				    new logImageHandler(main_ihandler, 
							logger,
						        logihandler_dpy_cb),
				    renderer_ready_cb, NULL,
				    error_handler, fatal_handler, NULL, 
				    num_threads );
#else
	if (num_remote_procs==0) {
	  main_vren= new raycastVRen( logger,
				      new logImageHandler(main_ihandler, 
							  logger,
							  logihandler_dpy_cb),
				      renderer_ready_cb, NULL,
				      error_handler, fatal_handler, NULL,
				      num_threads );
	}
	else {
	  int encoded_kid_type_and_procs= 
	    netVRen::encode_type( 1, num_remote_procs, num_threads );
	  main_vren= new netVRen( encoded_kid_type_and_procs, logger,
				  new logImageHandler(main_ihandler, logger,
						      logihandler_dpy_cb),
				  renderer_ready_cb, NULL,
				  error_handler, fatal_handler );
	  // main_vren->debug_on();
	}
#endif
      set_renderer_controls_cb(NULL,NULL,NULL);
      set_quality_settings_cb(NULL,NULL,NULL);
      }
    }
    break;
  case k_datavol_list_id:
    datavol_list_id= w;
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
  case k_tcl_script_id:
    tcl_script_id= w;
    break;
  case k_remote_load_dir_id:
    remote_load_dir_id= w;
    break;
  case k_remote_load_file_id:
    remote_load_file_id= w;
    break;
  case k_rmt_dta_xsize_text_id:
    rmt_dta_xsize_text_id= w;
    break;
  case k_rmt_dta_ysize_text_id:
    rmt_dta_ysize_text_id= w;
    break;
  case k_rmt_dta_zsize_text_id:
    rmt_dta_zsize_text_id= w;
    break;
  default:
    vfleet_ren_ctrl_create( w, id, reason );
    vfleet_tfun_create( w, id, reason );
    vfleet_nav_create( w, id, reason );
    break;
  }
}

static void abort_render_cb( Widget w, XtPointer foo, XtPointer bar )
{
  main_vren->AbortRender();
  render_in_progress= 0;
}

void do_render()
{
  // Do a render
  if (!datavol_list.head()) { // No data file yet loaded
    pop_error_dialog("no_dataset_loaded_msg");
    return;
  }
  int precalc_ok= 1;
  if (!main_svol || tfun_changed_flag) precalc_ok= precalculate_svol();
  if (precalc_ok) {
    render_in_progress= 1;
    logger->comment("Settings begin");
    if (main_volgob && !tfun_changed_flag) 
      main_volgob->update_trans( current_model_trans() );
    else {
      main_volgob= main_vren->create_volgob( main_svol, current_model_trans() );
      main_vren->setGeometry( main_volgob );
    }
    logger->comment("Settings complete; beginning render");
    main_vren->update_and_go( *current_camera(),
			      LightInfo( gColor( 1.0, 1.0, 1.0 ),
					 gBVector(current_lighting_dir,1.0),
					 gColor( 0.6, 0.6, 0.6 ) ),
			      main_ihandler->xsize(), main_ihandler->ysize() );
  }
}

static void do_render_cb (Widget w,
			  XtPointer foo,
			  XtPointer bar) {
  do_render();
}

static void canvas_resize_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
  if (main_ihandler) main_ihandler->resize();
}

static void canvas_expose_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
  static int inited = False;
    
  if (! inited) {
    inited = True;

    main_ihandler= AttachImageHandler( canvas_id );
    if ((!main_vren) && logger) {
      // this or logger creation must happen first
#ifdef LOCAL_VREN
      main_vren= new raycastVRen( logger,
				  new logImageHandler(main_ihandler, logger,
						      logihandler_dpy_cb),
				  renderer_ready_cb, NULL,
				  error_handler, fatal_handler, NULL, 
				  num_threads );
#else
      if (num_remote_procs == 0) {
	main_vren= new raycastVRen( logger,
				    new logImageHandler(main_ihandler, logger,
							logihandler_dpy_cb),
				    renderer_ready_cb, NULL,
				    error_handler, fatal_handler, NULL,
				    num_threads );
      }
      else {
	int encoded_kid_type_and_procs= 
	  netVRen::encode_type( 1, num_remote_procs, num_threads );
	main_vren= new netVRen( encoded_kid_type_and_procs, logger,
				new logImageHandler(main_ihandler, logger,
						    logihandler_dpy_cb),
			        renderer_ready_cb, NULL,
				error_handler, fatal_handler );
	// main_vren->debug_on();
      }
#endif
	set_renderer_controls_cb(NULL,NULL,NULL);
	set_quality_settings_cb(NULL,NULL,NULL);
      }
    
    // Set the top window property so that the window manager pays some
    // attention to the canvas color map.
    Atom atomcolormapwindows= 
      XInternAtom(XtDisplay(app_shell), "WM_COLORMAP_WINDOWS", False);
    
    Window canvas_win = XtWindow(canvas_id);
    XChangeProperty( XtDisplay(app_shell), XtWindow(app_shell), 
		     atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		     (unsigned char *)&canvas_win, 1 );
  } 
  else {
    main_ihandler->redraw();
  }
}

static void show_debugger_controls_cb (Widget w, XtPointer foo,
				       XmToggleButtonCallbackStruct *cb) {
  if (debugger_controls_id)
      if (showing_debugger = cb->set)
	  XtManageChild(debugger_controls_id);
      else
	  XtUnmanageChild(debugger_controls_id);
}

static void debug_tfunhandler_cb(Widget w, XtPointer foo,
				 XmToggleButtonCallbackStruct *cb)
{
  if (cb->set) baseTfunHandler::set_debug(1);
  else baseTfunHandler::set_debug(0);
}

static void debug_shoot_ray_cb (Widget w, XtPointer foo,
				XmToggleButtonCallbackStruct *cb) {
    static Cursor cross_hair = 0;
    if (canvas_id) 
	if (debug_shooting = cb->set) {
	    if (! cross_hair)
		cross_hair = XCreateFontCursor(XtDisplay(canvas_id), XC_cross_reverse);
	    XDefineCursor(XtDisplay(canvas_id), XtWindow(canvas_id), cross_hair);
	} else
	    XUndefineCursor(XtDisplay(canvas_id), XtWindow(canvas_id));
}

static void whack_tfun_dlog_cb( Widget w, XtPointer foo, XtPointer bar )
{
  if (xtfun_handler) {
    xtfun_handler->resize();
  }
}

static void canvas_input_cb(Widget w, int *uil_id, 
			  XmDrawingAreaCallbackStruct *event_data) {
  if (debug_shooting 
      && (event_data->event->type == ButtonPress)
      && (event_data->event->xbutton.button == Button1)) {
#ifdef LOCAL_VREN
    printf("\n\n\n\nShooting at %d, %d.\n", event_data->event->xbutton.x,
	   main_ihandler->ysize() - event_data->event->xbutton.y);
    ((raycastVRen *)main_vren)->TraceOneRay(event_data->event->xbutton.x, 
					    main_ihandler->ysize() 
					    - event_data->event->xbutton.y, 
					    True);
    main_ihandler->redraw();
#else
    if (num_remote_procs == 0) {
      printf("\n\n\n\nShooting at %d, %d.\n", event_data->event->xbutton.x,
	     main_ihandler->ysize() - event_data->event->xbutton.y);
      ((raycastVRen *)main_vren)->TraceOneRay(event_data->event->xbutton.x, 
					      main_ihandler->ysize() 
					      - event_data->event->xbutton.y, 
					      True);
      main_ihandler->redraw();
    }
    else {
      printf("\n\n\nCannot trace one ray when rendering remotely!\n");
    }
#endif
  }
}

void vfleet_quit()
{
  vfleet_nav_cleanup();
  delete main_vren;
  delete main_ihandler;
  vfleet_tfun_cleanup();
  delete logger;
  if (num_remote_procs) {
#ifndef LOCAL_VREN
    baseNet::shutdown();
#endif
  }
  XtDestroyApplicationContext(app_context);
  exit(0);
}

static void exit_really_cb(Widget w, caddr_t data, caddr_t data2)
{
  vfleet_quit();
}

static void exit_cancel_cb(Widget w, caddr_t data, caddr_t data2)
{
  // Do nothing
}

static void exit_cb (Widget w, XtPointer data, XtPointer cb) 
{
  pop_warning_dialog( "really_exit_msg",
		      exit_really_cb, exit_cancel_cb, NULL );
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

  /* Make sure it's all either numbers or '.' or leading '-', 
     deleting anything that isn't */
  for (len=0; len<cbs->text->length; len++) {
    c= cbs->text->ptr[len];
    if (!isdigit(c) && (c != '.') && ((c != '-')||(len!=0))) { /* reject it */
      int i;
      for (i=len; (i+1)<cbs->text->length; i++)
	cbs->text->ptr[i]= cbs->text->ptr[i+1];
      cbs->text->length--;
      len--;
    }
  }
  if (cbs->text->length == 0) cbs->doit= False;
}

static gBoundBox get_data_bbox( Widget x_w, Widget y_w, Widget z_w )
{
  char *xstring= XmTextFieldGetString( x_w );
  char *ystring= XmTextFieldGetString( y_w );
  char *zstring= XmTextFieldGetString( z_w );

  float x= atof( xstring );
  float y= atof( ystring );
  float z= atof( zstring );

  XtFree( xstring );
  XtFree( ystring );
  XtFree( zstring );

  return( gBoundBox( -0.5*x, -0.5*y, -0.5*z, 0.5*x, 0.5*y, 0.5*z ) );
}

static void update_datavol_lists()
{
  static XmStringTable stringtable= NULL;

  XmListDeleteAllItems(datavol_list_id);
  dList_iter<dvol_struct*> list_iter(datavol_list);
  dvol_struct** this_struct;
  while (this_struct= list_iter.next()) {
    XmListAddItemUnselected( datavol_list_id, (*this_struct)->xstring, 0 );
  }

  if (!free_datavol_id) return;

  int count= 0;
  dList_iter<dvol_struct*> count_iter(datavol_list);
  while (count_iter.next()) count++;

  delete [] stringtable;
  stringtable= new XmString[count];

  dList_iter<dvol_struct*> iter(datavol_list);
  int i= 0;
  while (this_struct=iter.next()) stringtable[i++]= (*this_struct)->xstring;

  Arg args[2];
  int n= 0;
  XtSetArg(args[n], XmNlistItems, stringtable); n++;
  XtSetArg(args[n], XmNlistItemCount, count); n++;
  XtSetValues( free_datavol_id, args, n );

  // datavol update list will have whacked the canvas widget size;  need
  // to reset it.
  resize_image( current_canvas_width, current_canvas_height );
}

static baseDataFile* open_datafile( char* fname )
{
  baseDataFile* datafile= NULL;
  float xSizePhysical= 0.0;
  float ySizePhysical= 0.0;
  float zSizePhysical= 0.0;
  int dataFileKnowsXSize= 0;
  int dataFileKnowsYSize= 0;
  int dataFileKnowsZSize= 0;

  switch(file_type) {
#ifdef INCL_HDF
  case hdf_file_type:
    datafile = new hdfDataFile(fname);
    break;
#endif
#ifdef INCL_FIASCO
  case pghmri_file_type:
    datafile = new PghMRIDataFile(fname);
    break;
#endif
  default:
    datafile = NULL;
  }
  if (! datafile || ! datafile->valid()) {
    Widget bad_file_dlog= NULL;
    logger->comment("File / Format mismatch.");
    if (MrmFetchWidget (mrm_id, "bad_file_dlog", app_shell,
			&bad_file_dlog, &mrm_class)) {
      XtManageChild(bad_file_dlog);
    }
    delete datafile;
    return NULL;
  }

  if (datafile->hasNamedValue("voxel_spacing.x")
      && datafile->getNamedValueType("voxel_spacing.x")==baseDataFile::Float32) {
    xSizePhysical= 
      datafile->xsize()*datafile->getNamedValue("voxel_spacing.x").float32;
    dataFileKnowsXSize= 1;
  }

  if (datafile->hasNamedValue("voxel_spacing.y")
      && datafile->getNamedValueType("voxel_spacing.y")==baseDataFile::Float32) {
    ySizePhysical= 
      datafile->ysize()*datafile->getNamedValue("voxel_spacing.y").float32;
    dataFileKnowsYSize= 1;
  }

  if (datafile->hasNamedValue("voxel_spacing.z")
      && datafile->getNamedValueType("voxel_spacing.z")==baseDataFile::Float32) {
    zSizePhysical= 
      datafile->zsize()*datafile->getNamedValue("voxel_spacing.z").float32;
    dataFileKnowsZSize= 1;
  }

  if (dataFileKnowsXSize || dataFileKnowsYSize || dataFileKnowsZSize) {
    char buf[128];
    pop_info_dialog( "adopt_datafile_phys_sz_msg" );
    if (dataFileKnowsXSize) {
      snprintf(buf,sizeof(buf),"%f",xSizePhysical);
      XmTextFieldSetString( data_xsize_text_id, buf );
    }
    if (dataFileKnowsYSize) {
      snprintf(buf,sizeof(buf),"%f",ySizePhysical);
      XmTextFieldSetString( data_ysize_text_id, buf );
    }
    if (dataFileKnowsZSize) {
      snprintf(buf,sizeof(buf),"%f",zSizePhysical);
      XmTextFieldSetString( data_zsize_text_id, buf );
    }
  }  

  return datafile;
}

static gBoundBox find_data_bbox( baseDataFile* datafile )
{
  if (datavol_list.head()) {
    // Previous datafile exists;  copy its bbox
    return datavol_list.head()->dvol->boundbox();
  }
  else {
    // This is the first datafile;  fake a bbox
    return gBoundBox(-0.5,-0.5,-0.5,0.5,0.5,0.5);
  }
}

static int check_remote_dfile_exists( const char* path )
{
#ifdef LOCAL_VREN
  return 0;
#else
  if (num_remote_procs>0) 
    return ((netVRen*)main_vren)->remote_file_readable( path );
  else return 0;
#endif
}

static int check_data_commensurate( baseDataFile* datafile, 
				   gBoundBox& new_boundbox )
{
  // Check to make sure the new datafile matches any previous datafiles
  // in dimensions and physical size
  DataVolume *existing_datavol;
  if ( datavol_list.head() ) { // Not first dvol
    existing_datavol= datavol_list.head()->dvol;
    if ((existing_datavol->xsize() != datafile->xsize())
	|| (existing_datavol->ysize() != datafile->ysize())
	|| (existing_datavol->zsize() != datafile->zsize())
	|| (existing_datavol->boundbox() != new_boundbox )) {
      // New dataset does not match old
      pop_error_dialog("incommensurate_datavols_msg");
      return 0;
    }
  }
  return 1;
}

static void register_and_load_data( baseDataFile* datafile,
				   gBoundBox& new_boundbox,
				   char* label_name, 
				   const int replace_which )
{
  int datavol_was_deleted= 0;

  DataVolume *new_dvol= main_vren->create_data_volume( datafile->xsize(), 
						       datafile->ysize(), 
						       datafile->zsize(),
						       new_boundbox );

  if (!vfleet_nav_cam_initialized()) {
    // First datavolume;  need to pick an appropriate camera
    vfleet_nav_init_camera( new_boundbox );
  }

  if (replace_which<0) { // Append to list as new datavol
    datavol_list.append( new dvol_struct(label_name, datafile, new_dvol) );
    datavol_was_deleted= 0;
  }
  else { // Replace existing dvol
    // Count current datavols and make sure index is in range
    int dvol_count= 0;
    dList_iter<dvol_struct*> count_iter(datavol_list);
    while (count_iter.next()) dvol_count++;

    if (replace_which >= dvol_count) {
      // Just append it
      datavol_list.append( new dvol_struct(label_name, datafile, new_dvol) );
      datavol_was_deleted= 0;
    }
    else {
      // Walk to the target element, insert the new element, and cut out
      // the old element
      dList_iter<dvol_struct*> iter(datavol_list);
      dvol_struct** this_struct;
      for (int i=0; i<=replace_which; i++) this_struct= iter.next();
      if (replace_which == dvol_count-1)
	iter.insert_ahead( new dvol_struct(label_name, datafile, new_dvol) );
      else
	iter.insert_behind( new dvol_struct(label_name, datafile, new_dvol) );
#ifdef INTEL_LINUX 
      // This hack avoids an apparent GNU C++ bug reported by
      // Paolo Zuliani on 3/31/97
      XmStringFree ((*this_struct)->xstring);
      delete (*this_struct)->file;
      delete (*this_struct)->dvol;
      delete (*this_struct)->name;
      iter.remove();
#else
      iter.remove();
      delete *this_struct;
#endif
      datavol_was_deleted= 1;
    }
  }
  
  new_dvol->load_datafile( datafile );

  update_datavol_lists();
  datavol_update_inform( datavol_was_deleted );

  // need to rebuild tfuns
  update_tfuns(); 
}

int load_data( char* fname, const int replace_which_dvol )
{
  baseDataFile *datafile;
  char* msgbuf= new char[strlen(fname)+32];
  char* label_name= fname;
  
  for (char* s = fname; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  if (! *label_name) {
    sprintf(msgbuf,"%s not a valid datafile!",fname);
    logger->comment(msgbuf);
    return 0;
  }
  
  sprintf(msgbuf,"Loading %s...",fname);
  logger->comment(msgbuf);
  
  if ( !(datafile= open_datafile(fname)) ) {
    delete [] msgbuf;
    return 0;
  }

  gBoundBox new_boundbox= find_data_bbox( datafile );

  if (!check_data_commensurate( datafile, new_boundbox)) {
    delete [] msgbuf;
    return 0;
  }

  register_and_load_data( datafile, new_boundbox, label_name, 
			 replace_which_dvol );

  sprintf(msgbuf,"Dataset %s loaded",fname);
  logger->comment(msgbuf);

  delete [] msgbuf;
  return 1;
}

static void open_datafile_cb (Widget w,
			  XtPointer blah,
			  XmFileSelectionBoxCallbackStruct *call_data) {
  char *the_name, *label_name, *s;
  baseDataFile *datafile;
  
  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  label_name= the_name;
  for (s = the_name; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  if (! *label_name) {
    logger->comment("Not a valid data file.");
    return;
  }
  
  logger->comment(the_name);
  
  if ( !(datafile= open_datafile(the_name)) ) return;

  gBoundBox new_boundbox= get_data_bbox( data_xsize_text_id, 
					 data_ysize_text_id, 
					 data_zsize_text_id );

  if (!check_data_commensurate( datafile, new_boundbox)) return;

  XtUnmanageChild(w);
  XmUpdateDisplay(w);
  
  register_and_load_data( datafile, new_boundbox, label_name, -1 );

  logger->comment("Dataset loaded");
}

static baseDataFile* open_remote_datafile( const char* fname )
{
  baseDataFile* datafile= NULL;
  float xSizePhysical= 0.0;
  float ySizePhysical= 0.0;
  float zSizePhysical= 0.0;
  int dataFileKnowsXSize= 0;
  int dataFileKnowsYSize= 0;
  int dataFileKnowsZSize= 0;

  switch(remote_file_type) {
#ifdef INCL_HDF
  case hdf_file_type:
    datafile= main_vren->create_data_file( fname, hdfDataFileType );
    break;
#endif
#ifdef INCL_FIASCO
  case pghmri_file_type:
    datafile= main_vren->create_data_file( fname, pghMRIDataFileType );
    break;
#endif
  default:
    datafile = NULL;
  }

  if (datafile->hasNamedValue("voxel_spacing.x")
      && datafile->getNamedValueType("voxel_spacing.x")==baseDataFile::Float32) {
    xSizePhysical= 
      datafile->xsize()*datafile->getNamedValue("voxel_spacing.x").float32;
    dataFileKnowsXSize= 1;
  }

  if (datafile->hasNamedValue("voxel_spacing.y")
      && datafile->getNamedValueType("voxel_spacing.y")==baseDataFile::Float32) {
    ySizePhysical= 
      datafile->ysize()*datafile->getNamedValue("voxel_spacing.y").float32;
    dataFileKnowsYSize= 1;
  }

  if (datafile->hasNamedValue("voxel_spacing.z")
      && datafile->getNamedValueType("voxel_spacing.z")==baseDataFile::Float32) {
    zSizePhysical= 
      datafile->zsize()*datafile->getNamedValue("voxel_spacing.z").float32;
    dataFileKnowsZSize= 1;
  }

  if (dataFileKnowsXSize || dataFileKnowsYSize || dataFileKnowsZSize) {
    char buf[128];
    pop_info_dialog( "adopt_datafile_phys_sz_msg" );
    if (dataFileKnowsXSize) {
      snprintf(buf,sizeof(buf),"%f",xSizePhysical);
      XmTextFieldSetString( rmt_dta_xsize_text_id, buf );
    }
    if (dataFileKnowsYSize) {
      snprintf(buf,sizeof(buf),"%f",ySizePhysical);
      XmTextFieldSetString( rmt_dta_ysize_text_id, buf );
    }
    if (dataFileKnowsZSize) {
      snprintf(buf,sizeof(buf),"%f",zSizePhysical);
      XmTextFieldSetString( rmt_dta_zsize_text_id, buf );
    }
  }  

  return datafile;
}

int load_remote_data( char* fname, const int replace_which_dvol )
{
  baseDataFile *datafile;
  char* msgbuf= new char[strlen(fname)+32];

  char* label_name= fname;
  
  for (char* s = fname; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  if (! *label_name) {
    sprintf(msgbuf,"%s not a valid datafile!",fname);
    logger->comment(msgbuf);
    return 0;
  }
  
  sprintf(msgbuf,"Loading %s remotely...",fname);
  logger->comment(msgbuf);
  
  if (!check_remote_dfile_exists(fname)) {
    logger->comment("File does not exist or is not readable.");
    return 0;
  }

  if ( !(datafile= open_remote_datafile(fname)) ) {
    delete [] msgbuf;
    return 0;
  }

  gBoundBox new_boundbox= find_data_bbox( datafile );

  if (!check_data_commensurate( datafile, new_boundbox)) {
    delete [] msgbuf;
    return 0;
  }

  register_and_load_data( datafile, new_boundbox, label_name, 
			 replace_which_dvol );

  sprintf(msgbuf,"Dataset %s loaded remotely",fname);
  logger->comment(msgbuf);

  delete [] msgbuf;
  return 1;
}

static void open_remote_datafile_cb (Widget w, int* type, 
				     caddr_t call_data )
{
  char* dir= XmTextFieldGetString( remote_load_dir_id );
  char* fname= XmTextFieldGetString( remote_load_file_id );

  int tot_path_length= 2;
  if (dir) tot_path_length += strlen(dir);
  if (fname) tot_path_length += strlen(fname);
  char* path= new char[tot_path_length];
  path[0]= '\0';
  if (dir) strcpy(path, dir);
  if ((strlen(dir)>0)&&(dir[strlen(dir)-1] != '/')) strcat(path,"/");
  if (fname) strcat(path, fname);

  if (path[strlen(path)-1] == '/') {
    pop_error_dialog("invalid_fname_msg");
    logger->comment("Not a valid data file.");
    XtFree( dir );
    XtFree( fname );
    return;
  }
  if (!check_remote_dfile_exists(path)) {
    pop_error_dialog("invalid_remote_fname_msg");
    logger->comment("File doesn't exist or is not readable.");
    XtFree( dir );
    XtFree( fname );
    return;
  }

  logger->comment(path);

  baseDataFile* datafile= open_remote_datafile(path);

  if (! datafile || ! datafile->valid()) {
    logger->comment("Can't read remote data; format mismatch?");
    pop_error_dialog("error_reading_remote_dta_msg");
    delete datafile;
    XtFree( dir );
    XtFree( fname );
    return;
  }

  gBoundBox new_boundbox= get_data_bbox( rmt_dta_xsize_text_id,
					 rmt_dta_ysize_text_id,
					 rmt_dta_zsize_text_id );

  if (!check_data_commensurate( datafile, new_boundbox)) {
    XtFree( dir );
    XtFree( fname );
    return;
  }

  XtUnmanageChild( XtParent( XtParent( w ) ) );

  register_and_load_data( datafile, new_boundbox, fname, -1 );

  XtFree( dir );
  XtFree( fname );
  
  logger->comment("Dataset loaded");
}

static void free_datavol_cb( Widget w, int *id, unsigned long *reason )
{
  Arg arg;
  XmString string;
  XtSetArg( arg, XmNtextString, &string );
  XtGetValues( w, &arg, 1 );

  dList_iter<dvol_struct*> iter(datavol_list);
  dvol_struct** this_struct;
  while (this_struct= iter.next()) {
    if (XmStringCompare( string, (*this_struct)->xstring)) {
#ifdef INTEL_LINUX 
      // This hack avoids an apparent GNU C++ bug reported by
      // Paolo Zuliani on 3/31/97
      XmStringFree ((*this_struct)->xstring);
      delete (*this_struct)->file;
      delete (*this_struct)->dvol;
      delete (*this_struct)->name;
      iter.remove();
#else
      iter.remove();
      delete *this_struct;
#endif
      update_datavol_lists();
      datavol_update_inform( 1 );
      logger->comment("Dataset removed");
#ifdef INTEL_LINUX
      // Added by Paolo Zuliani on 3/31/97
      XtUnmanageChild (w);
#endif
      return;
    }
  }

  fprintf(stderr,"free_datavol_cb: cannot find target in list!\n");
}

static void set_image_save_type_cb (Widget w,
		     char *text,
		     XtPointer cb) {
  current_image_save_type= text;
}

static int save_image_filename_ok(char *fname)
/* This routine checks for a filename which is valid for writing. */
{
  char *runner, *last;

  runner= fname;
  while (*runner) last= runner++;
  if ( *last == '/' ) return(0);

  return(1);
}

void save_image( char* fname )
{
  if (main_ihandler && main_ihandler->last_image()) {
    // Save the file
    main_ihandler->last_image()->save(fname,current_image_save_type);
    
    // Log that we saved the file
    char *string= new char[ strlen(fname) 
			   + strlen("Saved image to ") + 1 ];
    strcpy(string,"Saved image to ");
    strcat(string, fname);
    logger->comment(string);
    delete string;
  }
  else {
    logger->comment("No image to save!\n");
  }
}

static void save_image_anyway_cb( Widget w, caddr_t data_in, 
				  caddr_t call_data )
{
  file_write_warning_data *data= (file_write_warning_data*)data_in;
  save_image( data->fname );

  XtUnmanageChild(data->to_be_closed);
  delete data;
}

static void cancel_save_image_cb( Widget w, 
				  caddr_t data_in, 
				  caddr_t call_data )
{
  file_write_warning_data *data= (file_write_warning_data*)data_in;
  // Just pop down the warning dialog
  XtUnmanageChild(data->to_be_closed);
  delete data;
}

static void save_image_file_cb (Widget w,
			       XtPointer blah,
			       XmFileSelectionBoxCallbackStruct *call_data) {
  char *the_name, *label_name, *s;
  
  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  if (main_ihandler && main_ihandler->last_image()) {
    if (save_image_filename_ok(the_name)) {
      if (access(the_name,F_OK)) { // file does not already exist
	XtUnmanageChild(w);
	save_image(the_name);
      }
      else {
	file_write_warning_data *data= new file_write_warning_data;
	data->fname= the_name;
	data->to_be_closed= w;
	pop_warning_dialog( "save_anyway_msg", 
			    save_image_anyway_cb, 
			    cancel_save_image_cb,
			    (void *)data );
      }
    }
    else {
      pop_error_dialog("invalid_fname_msg");
    }
  }
  else {
    pop_error_dialog("no_image_to_save_msg");
    }
}

static void open_cb (Widget w,
		     int *tag,
		     caddr_t cb) 
{
  switch (*tag) {
  case k_open_id:
    XtManageChild( open_id );
    break;
#ifndef LOCAL_VREN
  case k_open_remote_id:
    XtManageChild( open_remote_id );
    break;
#endif
  case k_free_datavol_id:
    XtManageChild( free_datavol_id );
    break;
  case k_image_save_id:
    XtManageChild( image_save_id );
    break;
  case k_tcl_script_id:
    XtManageChild( tcl_script_id );
    break;
  default:
    vfleet_ren_ctrl_open( w, tag, cb );
    vfleet_tfun_open( w, tag, cb );
    vfleet_nav_open( w, tag, cb );
  }
}

static void photo_delete_cb( XPhotocopy *delete_me )
{
  delete delete_me;
}

static void photocopy_cb (Widget w,
			  XtPointer foo,
			  XtPointer bar) 
{
  if (main_ihandler->last_image()) {
    XPhotocopy* forget_me;
    forget_me= new XPhotocopy(app_shell, mrm_id, 
			      main_ihandler->last_image(),
			      photo_delete_cb);
  }
  else pop_error_dialog("no_image_to_photocopy_msg");
}

void resize_image( const int width, const int height )
{
  Dimension current_app_width;
  Dimension current_app_height;
  Arg args[2];

  int n= 0;
  XtSetArg(args[n],XmNwidth, &current_canvas_width); n++;
  XtSetArg(args[n], XmNheight, &current_canvas_height); n++;
  XtGetValues( canvas_id, args, n );

  if ((current_canvas_height != height) || (current_canvas_width != width)) {
    n= 0;
    XtSetArg(args[n],XmNwidth, &current_app_width); n++;
    XtSetArg(args[n],XmNheight, &current_app_height); n++;
    XtGetValues( app_shell, args, n );
    
    n= 0;
    XtSetArg(args[n], XmNwidth, 
	     width+current_app_width-current_canvas_width); n++;
    XtSetArg(args[n], XmNheight, 
	     height+current_app_height-current_canvas_height); n++;
    XtSetValues( app_shell, args, n );

    n= 0;
    XtSetArg(args[n], XmNwidth, width); n++;
    XtSetArg(args[n], XmNheight, height); n++;
    XtSetValues( canvas_id, args, n );
    main_ihandler->resize();
		      
    current_canvas_width= width;
    current_canvas_height= height;
  }
}

static void image_size_cb (Widget w,
			   int *which_size,
			   XtPointer bar) {
  int width, height;

  switch (*which_size) {
  case 0:
    width= 256;
    height= 256;
    break;
  case 1:
    width= 300;
    height= 300;
    break;
  case 2:
    width= 512;
    height= 512;
    break;
  case 3:
    width= 320;
    height= 240;
    break;
  case 4:
    width= 640;
    height= 480;
    break;
  default:
    fprintf(stderr,
	    "Error setting image size: unknown ID %d; UID version mismatch?\n",
	    *which_size);
    return;
  }

  resize_image( width, height );
}

static void set_filetype_cb (Widget w,
			     int *type,
			     XtPointer bar) {
  Arg args[MAX_ARGS];
  int n= 0;
  XmString s;

  switch (*type) {
  case hdf_file_type:
    s= XmStringCreate("*.hdf", XmSTRING_DEFAULT_CHARSET);
    break;
  case pghmri_file_type:
    s= XmStringCreate("*.mri", XmSTRING_DEFAULT_CHARSET);
    break;
  default:
    return;
  }
  file_type = *type;

  XtSetArg(args[n], XmNdirMask, s); n++;
  XtSetValues( open_id, args, n );
}

static void set_remote_filetype_cb( Widget w, int *type, XtPointer bar )
{
  remote_file_type= *type;
}

static void unmanage_cb( Widget w, caddr_t client_data, caddr_t call_data )
{
  XtUnmanageChild(w);
}

static void close_dialog_cb( Widget w, int *type, caddr_t call_data )
{
  XtUnmanageChild( XtParent( XtParent( w ) ) );
}

static MRMRegisterArg mrm_names[] = {
    {"create_cb",  (caddr_t) create_cb},
    {"canvas_resize_cb",  (caddr_t) canvas_resize_cb},
    {"canvas_expose_cb",  (caddr_t) canvas_expose_cb},
    {"open_cb",  (caddr_t) open_cb},
    {"open_datafile_cb",  (caddr_t) open_datafile_cb},
    {"open_remote_datafile_cb", (caddr_t) open_remote_datafile_cb},
    {"free_datavol_cb", (caddr_t)free_datavol_cb},
    {"save_image_file_cb", (caddr_t) save_image_file_cb},
    {"set_image_save_type_cb", (caddr_t)set_image_save_type_cb},
    {"exit_cb",  (caddr_t) exit_cb},
    {"do_render_cb",  (caddr_t) do_render_cb},
    {"show_debugger_controls_cb",  (caddr_t) show_debugger_controls_cb},
    {"debug_shoot_ray_cb",  (caddr_t) debug_shoot_ray_cb},
    {"debug_tfunhandler_cb", (caddr_t) debug_tfunhandler_cb},
    {"whack_tfun_dlog_cb", (caddr_t)whack_tfun_dlog_cb},
    {"canvas_input_cb",  (caddr_t) canvas_input_cb},
    {"photocopy_cb", (caddr_t) photocopy_cb},
    {"image_size_cb", (caddr_t)image_size_cb},
    {"set_filetype_cb", (caddr_t) set_filetype_cb},
    {"set_remote_filetype_cb", (caddr_t) set_remote_filetype_cb},
    {"unmanage_cb", (caddr_t)unmanage_cb},
    {"num_text_check_cb", (caddr_t)num_text_check_cb},
    {"close_dialog_cb", (caddr_t)close_dialog_cb},
    {"abort_render_cb", (caddr_t)abort_render_cb}
};

static void service_callback(Widget w)
{
#ifndef LOCAL_VREN
  if (num_remote_procs != 0) {
#ifdef SC94
    check_extern_pvm_event();
#endif 
    baseNet::service();
    service_interval_id= 
      XtAppAddTimeOut(app_context, service_wait_time, 
		      (XtTimerCallbackProc)service_callback, NULL);
  }
#endif
}

static void main_x_loop()
{
  XEvent xevent;

  while (1) {
    if (vfleet_tcl_script_pending) {
#ifndef LOCAL_VREN
      if (num_remote_procs != 0) XtRemoveTimeOut(service_interval_id);
#endif
      (void)vfleet_tcl_exec_script();
#ifndef LOCAL_VREN
      service_callback(NULL);
#endif
    }
    XtAppNextEvent( app_context, &xevent );
    XtDispatchEvent( &xevent );
  }
}

void quick_event_check()
{
#ifndef LOCAL_VREN
  // Check for network events
  if (num_remote_procs != 0) baseNet::service();
#endif
  // Check for X events
  while (XtAppPending(app_context)) {
    XEvent event;
    XtAppNextEvent(app_context,&event);
    XtDispatchEvent(&event);
  }
}

void watch_events_and_await_render()
{
  XEvent xevent;

  quick_event_check();

#ifndef LOCAL_VREN
  if (num_remote_procs != 0) {
    service_interval_id= 
      XtAppAddTimeOut(app_context, service_wait_time, 
		      (XtTimerCallbackProc)service_callback, NULL);
  }
#endif
  while (render_in_progress) {
    XtAppNextEvent( app_context, &xevent );
    XtDispatchEvent( &xevent );
  }
#ifndef LOCAL_VREN
  if (num_remote_procs != 0) XtRemoveTimeOut(service_interval_id);
#endif
}

int main(int argc,
	 char *argv_in[]) {
  argv = argv_in;
  Widget app_main = NULL;
  Display *display;
  Arg args[3];
  
  char *progname = argv[0];
  
  extern char *optarg;
  extern int optind; 
  int c;
  int errflg= 0;
  while ((c= getopt(argc, argv, "n:s:S:T:")) != -1)
    switch (c) {
    case 'n':
#ifndef LOCAL_VREN
      if (sscanf(optarg,"%d",&num_remote_procs) != 1) errflg++;
#endif
      break;
    case 's':
      initial_tcl_script= optarg;
      break;
    case 'S':
      {
#ifndef LOCAL_VREN
	int server_tid;
	if (sscanf(optarg,"%x",&server_tid) != 1) {
	  fprintf(stderr,"%s: invalid server tid <%s>\n",
		  progname, optarg);
	  errflg++;
	}
	else {
	  RemObjInfo::set_server_tid(server_tid);
	}
#endif
      }
    break;
    case 'T':
      {
	if (sscanf(optarg,"%d",&num_threads) != 1) {
	  fprintf(stderr,"%s: invalid number of threads <%s>\n",
		  progname, optarg);
	  errflg++;
	}
      }
      break;
    case '?': 
      errflg++;
      break;
    default: 
      errflg++;
      break;
    }
  if (errflg) {
    fprintf(stderr,
	    "usage: %s [-n nslaves] [-T nthreads] [-s tclscript] [-S server_tid_in_hex]\n",
	    progname);
    exit(2);
  }

  MrmInitialize ();

#ifdef AVOID_THREADS
  // Command line option -T might still be valid, if the other
  // command line options request that rendering be done elsewhere
  // (in an executable with threads).
  if ((num_threads>0) && (num_remote_procs==0)) {
    fprintf(stderr,
	    "%s: compiled without threads; -T invalid for local render!\n");
    exit(2);
  }
#endif

#ifndef LOCAL_VREN
  if (num_remote_procs != 0) {
    netLogger::initialize(progname);
    netVRen::initialize(progname);
    // netVRen::debug_static_on();
  }
#endif

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
  
  if (MrmOpenHierarchyPerDisplay(display, 1, mrm_vec, NULL, &mrm_id) != MrmSUCCESS) {
    fprintf(stderr, 
	    "%s:  Can't find the User Interface Definition file\n",
	    progname);
    exit(1);
  }
  
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));

#ifdef HPUX
  MrmRegisterClass(0, NULL, "CreateCanvas", 
		   (Widget(*)())CreateCanvas, NULL);
#else
  //MrmRegisterClass((MrmType)0, NULL, "CreateCanvas", 
  //		   (Widget(*)(...))CreateCanvas, NULL);
  MrmRegisterClass((MrmType)0, NULL, "CreateCanvas", 
		   (Widget(*)())CreateCanvas, NULL);

#endif

  vfleet_error_reg();
  vfleet_ren_ctrl_reg();
  vfleet_tfun_reg();
  vfleet_nav_reg();
  vfleet_script_reg();
  MrmFetchWidget (mrm_id, "app_main", app_shell, &app_main, &mrm_class);
  XtManageChild(app_main);
  XtRealizeWidget(app_shell);
  
#ifndef LOCAL_VREN
  if (num_remote_procs != 0) {
    XtAppAddTimeOut( app_context, service_wait_time, 
		     (XtTimerCallbackProc)service_callback, NULL );
  }
#endif
  
  XFlush(XtDisplay(app_main));

  current_lighting_dir= gVector( 0.0, -10.0, -10.0 );
  current_lighting_dir.normalize();

  vfleet_nav_init(canvas_id);
  vfleet_tcl_init();

  main_x_loop();

  vfleet_nav_cleanup();
  delete main_vren;
  delete main_ihandler;
  vfleet_tfun_cleanup();
  delete logger;
  
  return(0);
}

char *add_default_path( const char *fname_root )
{
  char *env_loaddir= getenv(ENV_HOME_DIR);
  if (!env_loaddir) {
    // Return a copy of name, so caller can free it
    char *result= new char[strlen(fname_root) + 1];
    (void)strcpy(result,fname_root);
    return result;
  }
  else {
    int dir_length= strlen(env_loaddir);
    int needs_slash= (env_loaddir[dir_length-1] != '/');

    char *fname;
    if (needs_slash) {
      fname= new char[dir_length + 1 + strlen(fname_root) + 1];
      strcpy(fname, env_loaddir);
      strcat(fname, "/");
    }
    else {
      fname= new char[dir_length + strlen(fname_root) + 1];
      strcpy(fname, env_loaddir);
    }
    strcat(fname, fname_root);
    return fname;
  }
}

FILE *fopen_read_default_dir( const char* fname_root )
{
  char *fname= add_default_path( fname_root );
  FILE *result= fopen(fname,"r");
  delete fname;
  return result;
}
