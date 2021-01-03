/****************************************************************************
 * mvoltool.cc for pvm 2.4
 * Author Robert Earhart
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

/*mvoltool.cc
  Very simple framework to control volume rendering under Motif
  */

#ifdef __GNUC__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/DrawingA.h>

#include <im.h>
#include "basenet.h"
#include "vren.h"
#include "tfun.h"
#include "raycastvren.h"
#include "logger.h"
#include "servman.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"

static Widget bad_file_dlog= NULL;

static baseLogger *logger= NULL;
static XImageHandler *main_ihandler= NULL;
#ifdef never
static baseVRen *vren= NULL;
#endif
static raycastVRen *vren= NULL;
static DataVolume *datavol= NULL;
static baseSampleVolume *svol= NULL;
static VolGob *volgob= NULL;
static TransferFunction *tfun= NULL;

// Forward definitions
static void net_service();
static void start_timer();
static void render_start();
static void make_sheet( DataType *buffer, int k, 
			int xdim, int ydim, int zdim );
static void voltool_render_start( Widget w );

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
    r= 1.0;
    g= b= 0.0;
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

  // Add axes and boundbox
  if (j<=1 && k<=1) { // x axis
    r= alpha= 1.0;
    g= b= 0.0;
  }
  if (k<=1 && i<=1) { // y axis
    g= alpha= 1.0;
    r= b= 0.0;
  }
  if (i<=1 && j<=1) { // z axis
    b= alpha= 1.0;
    r= g= 0.0;
  }
  if ( (j==63 && k==63)
       || (k==63 && i==63)
       || (i==63 && j==63)
       || (i==0 && j==63)
       || (j==0 && i==63)
       || (j==0 && k==63)
       || (k==0 && j==63)
       || (i==0 && k==63)
       || (k==0 && i==63) ) {
    r= g= b= 1.0;
    alpha= 1.0;
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

static Widget about_init(Widget w)
{
    Arg args[10];
    register int n;
    char *text= "\
Mvoltool version 0.0001\n\
\n\
By Joel Welling and Robert Earhart\n\
\n\
This will someday be a fine tool for \n\
distributed volume rendering.\n\
\n\
Copyright 1993, Carnegie Mellon University.";

    n=0;
    XmString helpText=XmStringCreateLtoR(text, XmSTRING_DEFAULT_CHARSET);
    XmString okText=XmStringCreateSimple("Close");
    XtSetArg(args[n], XmNmessageString, helpText); n++;
    XtSetArg(args[n], XmNokLabelString, okText); n++;
    XtSetArg(args[n], XmNmessageAlignment, XmALIGNMENT_CENTER); n++;
    XtSetArg(args[n], XmNtitle, "Help"); n++;
    XtSetArg(args[n], XmNmwmDecorations, -MWM_DECOR_RESIZEH
	     -MWM_DECOR_BORDER); n++;
    Widget message_box=XmCreateMessageDialog(w, "help", args, n);
 
    Widget button=XmMessageBoxGetChild(message_box, XmDIALOG_CANCEL_BUTTON);
    XtUnmanageChild(button);
    
    button=XmMessageBoxGetChild(message_box, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(button);
    return(message_box);
}

static void render_goCB( Widget w, Widget client_data, caddr_t call_data )
{
  voltool_render_start(w);
}

static void manage_meCB(Widget w, Widget client_data, caddr_t call_data)
{
  XtManageChild(client_data);
}

static void unmanageCB(Widget w, Widget me, caddr_t call_data)
{
  XtUnmanageChild(me);
}

static void destroy_handlerCB(Widget w, XmautoImageHandler *me,
			      caddr_t call_data)
     
/*What happens when we\'re XtDestroyed...*/
{
  if (me->last_image()) {
    fclose(me->fp);
    delete(me->last_image());
  }
  delete(me);
}

static void redraw_meCB(Widget w, XImageHandler *client_data,
			caddr_t call_data)

/*Sends a redraw message to it\'s image.*/
{
    client_data->redraw();
}

static void resize_meCB(Widget w, XImageHandler *client_data,
			caddr_t call_data)

/*Sends a resize message to it\'s image.*/
{
    client_data->resize();
}

static void open_a_fileCB(Widget w, Widget top,
			  XmFileSelectionBoxCallbackStruct *call_data)
{
  Arg args[10];
  register int n;
  
  char *the_name;
  FILE *fp;
  
  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  fprintf(stderr,"selected <%s>\n",the_name);
  
  logger->comment(the_name);
  
  if (! (fp=fopen(the_name, "r"))) {
    logger->comment("Input data file not found!");
    XtManageChild(bad_file_dlog);
    return;
  }
  
  XtUnmanageChild(w);

  int xdim= 64;
  int ydim= 64;
  int zdim= 64;
  gBoundBox bbox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 );
  
  delete datavol;
  delete svol;  // since new datavol will require new svol
  delete volgob;
  svol= NULL;  // serves as flag to cause regen of svol and volgob
  datavol= vren->create_data_volume( xdim, ydim, zdim, bbox );
  
  DataType *buffer= new DataType[xdim*ydim];
  int k;
  for (k=0; k<zdim; k++) {
    make_sheet( buffer, k, xdim, ydim, zdim );
    datavol->load_zplane( buffer, k );
  }
  datavol->finish_init();
  
  logger->comment("Dataset loaded");
  delete [] buffer;
  
  fclose(fp);
}

static Widget create_error_dlog(Widget w, char *warn_text)
{
  Arg args[10];
  register int n;
  Widget error_dlog;
  
  XmString okText=XmStringCreateSimple("Close");
  XmString dtitle=XmStringCreateSimple("Error!");
  XmString warnText=XmStringCreateSimple(warn_text);
  
  n=0;
  XtSetArg(args[n], XmNfileTypeMask, XmFILE_REGULAR); n++;
  XtSetArg(args[n], XmNokLabelString, okText); n++;
  XtSetArg(args[n], XmNmessageString, warnText); n++;
  XtSetArg(args[n], XmNdialogTitle, dtitle); n++;
  error_dlog = XmCreateErrorDialog(w, "bad_file", args, n);
  
  Widget button;
  
  if (button=XmMessageBoxGetChild(error_dlog, XmDIALOG_HELP_BUTTON))
    XtUnmanageChild(button);
  if (button=XmMessageBoxGetChild(error_dlog, XmDIALOG_CANCEL_BUTTON))
    XtUnmanageChild(button);
  
  XmStringFree(okText);
  XmStringFree(warnText);
  XmStringFree(dtitle);
  return(error_dlog);
}

static Widget file_init(Widget w)
{
  Arg args[10];
  register int n;
  
  n=0;
  XtSetArg(args[n], XmNfileTypeMask, XmFILE_REGULAR); n++;
  XtSetArg(args[n], XmNtitle, "Open Data File"); n++;
  Widget file_box=XmCreateFileSelectionDialog(w, "file", args, n);
  
  XtAddCallback(file_box, XmNokCallback,
		(XtCallbackProc)(&open_a_fileCB), w);
  
  XtAddCallback(file_box, XmNcancelCallback,
		(XtCallbackProc)(&unmanageCB), file_box);
  
  Widget button;
  
  if (button=XmMessageBoxGetChild(file_box, XmDIALOG_HELP_BUTTON))
    XtUnmanageChild(button);
  
  if (!bad_file_dlog) bad_file_dlog=create_error_dlog(w,"Bad file name!");
  
  return(file_box);
}

static void quitCB(Widget w, Boolean *done, caddr_t call_data)
{
  *done=True;
}

int main(int argc, char *argv[]) {
  Arg args[10];
  register int n;
  XtAppContext app_context;
  
  fprintf(stderr,"Starting...\n");
  
#ifdef never
  netLogger::initialize( argv[0] );
  logger= new netLogger( argv[0] );
  logger->debug_on();
  netLogger::debug_static_on();
  netVRen::initialize( argv[0] );
  netImageHandler::initialize( argv[0], NULL );
  netVRen::debug_static_on();
#endif
  logger= new fileLogger("/afs/psc/tmp/users/welling/test.log",argv[0]);
  
  n=0;
//  XtSetArg(args[n], XmNgeometry, "256x300"); n++;
  Widget toplevel=XtAppInitialize(&app_context, "Mvoltool", NULL,
				  0, &argc, argv, NULL, args, n);
  
  /*Main window*/
  n=0;
  Widget main_window=XmCreateMainWindow(toplevel, "main", args, n);
  XtManageChild(main_window);

  /* Drawing canvas */
  n=0;
  XtSetArg(args[n], XmNheight, 256); n++;
  XtSetArg(args[n], XmNwidth, 256); n++;
  Widget canvas= XmCreateDrawingArea(main_window, "canvas", args, n);
  XtManageChild(canvas);
  
  /*Menu bar*/
  n=0;
  Widget menu_bar = XmCreateMenuBar(main_window, "menubar", args, n);
  XtManageChild(menu_bar);
  
  /*File Menu*/
  n=0;
  Widget menu_pane = XmCreatePulldownMenu(menu_bar, "file_menu", args, n);
  
  n=0;
  XmString labelString=XmStringCreateSimple("Open");
  XmString accelString=XmStringCreateSimple("Ctrl+O");
  XtSetArg(args[n], XmNaccelerator, "Ctrl<Key>O"); n++;
  XtSetArg(args[n], XmNacceleratorText, accelString); n++;
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNmnemonic, XK_O); n++;
  Widget button = XmCreatePushButton(menu_pane, "open_datafile", args, n);
  XtManageChild(button);
  XmStringFree(labelString);
  XtAddCallback(button, XmNactivateCallback,
		(XtCallbackProc) manage_meCB,
		file_init(toplevel));
  
  n=0;
  Widget separator = XmCreateSeparatorGadget(menu_pane, "", args, n);
  XtManageChild(separator);
  
  Boolean done;
  
  n=0;
  labelString=XmStringCreateSimple("Quit");
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNmnemonic, XK_Q); n++;
  button = XmCreatePushButton(menu_pane, "quit", args, n);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback,
		(XtCallbackProc) quitCB, &done);
  XmStringFree(labelString);
  
  n=0;
  labelString=XmStringCreateSimple("File");
  XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNmnemonic, XK_F); n++;
  Widget cascade = XmCreateCascadeButtonGadget(menu_bar, "file", args, n);
  XtManageChild(cascade);
  XmStringFree(labelString);
  
  /*Render Menu*/
  n=0;
  menu_pane = XmCreatePulldownMenu(menu_bar, "render_menu", args, n);
  
  n=0;
  labelString=XmStringCreateSimple("Go");
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNmnemonic, XK_G); n++;
  button = XmCreatePushButton(menu_pane, "go", args, n);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback,
		(XtCallbackProc) render_goCB, NULL);
  XmStringFree(labelString);
  
  n=0;
  labelString=XmStringCreateSimple("Render");
  XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNmnemonic, XK_R); n++;
  cascade = XmCreateCascadeButtonGadget(menu_bar, "render", args, n);
  XtManageChild(cascade);
  XmStringFree(labelString);
  
  /*Help Menu*/
  n=0;
  menu_pane = XmCreatePulldownMenu(menu_bar, "help_menu", args, n);
  
  n=0;
  labelString=XmStringCreateSimple("About");
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNmnemonic, XK_A); n++;
  button = XmCreatePushButton(menu_pane, "about", args, n);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback,
		(XtCallbackProc) manage_meCB,
		(XtPointer) about_init(toplevel));
  XmStringFree(labelString);
  
  n=0;
  labelString=XmStringCreateSimple("Help");
  XtSetArg(args[n], XmNlabelString, labelString); n++;
  XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
  XtSetArg(args[n], XmNmnemonic, XK_H); n++;
  cascade=XmCreateCascadeButtonGadget(menu_bar, "help", args, n);
  XtManageChild(cascade);
  XmStringFree(labelString);
  
  n=0;
  XtSetArg(args[n], XmNmenuHelpWidget, cascade); n++;
  XtSetValues(menu_bar, args, n);
  
  XmMainWindowSetAreas(main_window, menu_bar, NULL, NULL, NULL, canvas);
  
  // Realize the top level widget
  XtRealizeWidget(toplevel);
  
  // Create the image handler
  main_ihandler= new XImageHandler( XtDisplay(canvas),
				    DefaultScreen(XtDisplay(canvas)), 
				    XtWindow(canvas) );
  XtAddCallback(canvas, XmNresizeCallback,
		(XtCallbackProc) resize_meCB,
		(XtPointer) main_ihandler);
  XtAddCallback(canvas, XmNexposeCallback,
		(XtCallbackProc) redraw_meCB,
		(XtPointer) main_ihandler);
  
  // Set the top window property so that the window manager pays some
  // attention to the canvas color map.
  Atom atomcolormapwindows= 
    XInternAtom(XtDisplay(toplevel), "WM_COLORMAP_WINDOWS", False);
  Window canvas_win= XtWindow(canvas);
  XChangeProperty( XtDisplay(toplevel), XtWindow(toplevel), 
		   atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		   (unsigned char *)&canvas_win, 1 );

  // Build the renderer
  vren= new raycastVRen( 64, 64, 64, logger, main_ihandler,
			 error_handler, fatal_handler, NULL );
#ifdef never
  vren= new netVRen( 64, 64, 64, 0, logger, main_ihandler,
			 error_handler, fatal_handler );
  ((netVRen *)vren)->debug_on();
#endif

  tfun= new TransferFunction( 1, tfun_method );

#ifdef never
  // Start timer that periodically checks baseNet activity.
  void (*orig_sigfun)()= signal( SIGALRM, net_service );
  start_timer(Voltool_mainwindow.mainwindow);
#endif

  // The main loop
  XEvent the_event;
  for (done = False; ! done ;) {
    XtAppNextEvent(app_context, &the_event);
    XtDispatchEvent(&the_event);
  };

  XtDestroyWidget(toplevel);
  delete vren;
  delete main_ihandler;
  delete logger;
#ifdef never
  (void)signal( SIGALRM, orig_sigfun );
  netLogger::shutdown();
  netVRen::shutdown();
#endif
}

static void make_sheet( DataType *buffer, int k, int xdim, int ydim, int zdim )
{
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

static void voltool_render_start( Widget w )
// Do a render
{
  static Widget no_d_loaded_dialog= NULL;
  gBoundBox bbox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 );
  if (!datavol) { // No data file yet loaded
    if (!no_d_loaded_dialog)
      no_d_loaded_dialog= 
	create_error_dlog(w,"No data file has been loaded!");
    XtManageChild( no_d_loaded_dialog );
    return;
  }
  if (!svol) { // new datavol just got loaded
    logger->comment("Precalculations begin");
    svol= vren->create_sample_volume( bbox, *tfun, 1, &datavol );
    volgob= vren->create_volgob( svol, gTransfm::identity );
    logger->comment("Precalculations complete");
  }
  logger->comment("Settings begin");
  vren->setCamera( gPoint( 0.5, 1.5, 10.5 ), gPoint( 0.5, 0.5, 0.5 ), 
		   gVector( 0.0, 1.0, 0.0 ), 15.0, -2.0, -20.0 );
#ifdef never
  vren->setCamera( gPoint( 3.5, 3.5, 10.5 ), gPoint( 0.5, 0.5, 0.5 ), 
		   gVector( 0.0, 1.0, 0.0 ), 15.0, -2.0, -20.0 );
  vren->setCamera( gPoint( 3.0, 3.0, 10.0 ), gPoint( 0.0, 0.0, 0.3 ), 
		   gVector( 0.0, 1.0, 0.0 ), 15.0, -2.0, -20.0 );
#endif
  vren->setLightInfo( LightInfo( gColor( 1.0, 1.0, 1.0 ),
			       gBVector( 0.0, -1.0, 0.0, 1.0 ), 
			       gColor( 0.6, 0.6, 0.6 ) ) );
  vren->setQualityMeasure( QualityMeasure(0.9, 0.05, 0.05, 0.05) );
  vren->setGeometry( volgob );
  logger->comment("Settings complete; beginning render");
  vren->StartRender( main_ihandler->xsize(), main_ihandler->ysize() );
  logger->comment("Render complete");
}

static void net_service()
{
  baseNet::service();  // Check and possibly process baseNet events
  start_timer();  // Restart the timer
}

static void start_timer()
{
  itimerval itimevalue;
  
  /* set timer */
  itimevalue.it_value.tv_sec = 0 ; 
  itimevalue.it_interval.tv_sec = 0 ;
  itimevalue.it_value.tv_usec = 5000 ;   // 5 milliseconds
  itimevalue.it_interval.tv_usec = 0 ;   // Do not automatically restart
  if ( setitimer(ITIMER_REAL, &itimevalue, NULL) ) {
    perror("Mvoltool: timer setting error");
    exit(-1);
  }
}

