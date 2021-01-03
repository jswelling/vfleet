/****************************************************************************
 * voltool.cc
 * Author Joel Welling
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
#include <stdio.h>
#include <math.h>

#ifdef __GNUC__
#include <unistd.h>
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/notice.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gfm.h>
#include "netv.h"
#include "basenet.h"
#include "vren.h"
#include "tfun.h"
#include "raycastvren.h"
#include "logger.h"
#include "servman.h"
#include "voltool_ui.h"
#include "voltool.h"
#include "image.h"
#include "imagehandler.h"
#include "ximagehandler.h"

//
// Global object definitions.
//
voltool_mainwindow_objects	Voltool_mainwindow;
gfm_popup_objects *file_dialog;

//
// Instance XV_KEY_DATA key.  An instance is a set of related
// user interface objects.  A pointer to an object's instance
// is stored under this key in every object.  This must be a
// global variable.
//
Attr_attribute	INSTANCE;
Attr_attribute  WIN_IHANDLER_KEY;  // Unique number for XV_KEY_DATA

static baseLogger *logger= NULL;
static XImageHandler *main_ihandler;
#ifdef never
static baseVRen *vren= NULL;
#endif
static raycastVRen *vren= NULL;
static DataVolume *datavol= NULL;
static baseSampleVolume *svol= NULL;
static VolGob *volgob= NULL;
static TransferFunction *tfun= NULL;

// Forward definitions
static void start_timer(Notify_client client);

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
{
  if (ndata<1) {
    fprintf(stderr,"tfun_method: %d is not enough data volumes!\n",ndata);
    exit(-1);
  }

  smpl.clr= gColor( 1.0, 1.0, 1.0, 
		    (float)(data_table->val(i,j,k)-DataTypeMin)
		    /(float)(DataTypeMax-DataTypeMin) );

  smpl.grad= data_table->gradient( i, j, k );
}

static Notify_value net_service( Notify_client client, int which )
{
  baseNet::service();  // Check and possibly process baseNet events
  start_timer(client);  // Restart the timer
  return (NOTIFY_DONE) ;
}

static void start_timer(Notify_client client)
{
  struct itimerval itimevalue;
  
  Notify_value frame_monitor() ;
  
  /* set timer */
  itimevalue.it_value.tv_sec = 0 ; 
  itimevalue.it_interval.tv_sec = 0 ;
  itimevalue.it_value.tv_usec = 5000 ;   // 5 milliseconds
  itimevalue.it_interval.tv_usec = 0 ;   // Do not automatically restart
  notify_set_itimer_func(client, (Notify_func)net_service, ITIMER_REAL, 
			      &itimevalue, ((struct itimerval *) 0));
}

main(int argc, char **argv)
{
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

  //
  // Initialize XView.
  //
  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
  INSTANCE = xv_unique_key();
  WIN_IHANDLER_KEY = xv_unique_key();
  
  //
  // Initialize user interface components.
  // Do NOT edit the object initializations by hand.
  //
  Voltool_mainwindow.objects_initialize((Xv_opaque)NULL);
  file_dialog= gfm_initialize(NULL, Voltool_mainwindow.mainwindow,
			      "Load Image File");
    
  // Build the main image handler
  Display *dpy= (Display *)xv_get( Voltool_mainwindow.rendercanvas, 
				   XV_DISPLAY );
  main_ihandler= 
    new XImageHandler( dpy, 
	(int)xv_get( (Xv_Screen)xv_get( Voltool_mainwindow.rendercanvas, 
					XV_SCREEN ), SCREEN_NUMBER ),
	(Window)xv_get(canvas_paint_window(Voltool_mainwindow.rendercanvas), 
		       XV_XID) );
  xv_set( Voltool_mainwindow.rendercanvas, 
	  XV_KEY_DATA, WIN_IHANDLER_KEY, main_ihandler );

  // Build the renderer
  vren= new raycastVRen( 128, 128, 128, logger, main_ihandler,
			 error_handler, fatal_handler, NULL );
#ifdef never
  vren= new netVRen( 128, 128, 128, 0, logger, main_ihandler,
			 error_handler, fatal_handler );
  ((netVRen *)vren)->debug_on();
#endif

  tfun= new TransferFunction( 1, tfun_method );

#ifdef never
  // Start timer that periodically checks baseNet activity.
  start_timer(Voltool_mainwindow.mainwindow);
#endif

  //
  // Turn control over to XView.
  //
  xv_main_loop(Voltool_mainwindow.mainwindow);

  delete vren;
  delete main_ihandler;
  delete logger;
#ifdef never
  netLogger::shutdown();
  netVRen::shutdown();
#endif
  exit(0);
}

/* Pop up a notice box, with the given text. */
void popup_notice(Frame owner, char *text)
{
  Xv_notice notice;

  notice= xv_create(owner, NOTICE, 
		    NOTICE_MESSAGE_STRING, text,
		    XV_SHOW, TRUE,
		    NULL);

  xv_destroy_safe(notice);
}

static Notify_value image_window_destroy( Notify_client client,
					 Destroy_status status )
{
  if (status == DESTROY_CLEANUP) {
    fprintf(stderr,"Destroying cell\n");
    return( notify_next_destroy_func(client,status) );
  }
  return( NOTIFY_DONE );
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

int file_select_callback( gfm_popup_objects *ip, char *directory, 
			  char *file )
{
  FILE *fp;
  char *fullname;
  rgbImage *image;

  /* Build the fullname name string, adding a '/' if needed. */
  fullname= new char[ strlen(directory) + strlen(file) + 2 ];
  strcpy(fullname, directory);
  if (strlen(fullname) && fullname[strlen(fullname)-1] != '/')
    strcat(fullname,"/");
  strcat(fullname, file);

  fprintf(stderr,"selected <%s>\n",fullname);

  logger->comment(fullname);

#ifdef never
  if ((fp = fopen(fullname, "r")) == NULL) {
    logger->comment("Input data file not found!");
    popup_notice(ip->popup,"Input data file not found!");
    delete fullname;
    return( GFM_ERROR );
  }

  image= new rgbImage( fp, fullname );
  if (!(image->valid())) {
    logger->comment("Unable to read file; it may be corrupted!");
    popup_notice(ip->popup,"Unable to read file; it may be corrupted!");
    delete fullname;
    return( GFM_ERROR );
  }

  logger->comment("Image read successful");

  // Create and pop up the image window
  voltool_ImageWindow_objects	*IWindow=
    new voltool_ImageWindow_objects;
  IWindow->objects_initialize(Voltool_mainwindow.mainwindow);
  notify_interpose_destroy_func( IWindow->ImageWindow,
				 (Notify_func)image_window_destroy );

  xv_set( IWindow->ImageWindow, WIN_SHOW, TRUE, NULL );

  Window canvas_win= (Window)xv_get(canvas_paint_window(IWindow->canvas1), 
				    XV_XID);
  Window top_win= (Window)xv_get(IWindow->ImageWindow, XV_XID);
  Display *dpy= (Display *)xv_get( IWindow->canvas1, XV_DISPLAY );
  baseImageHandler *ihandler= 
    new XImageHandler( dpy, 
		       (int)xv_get( (Xv_Screen)xv_get( IWindow->canvas1, 
						       XV_SCREEN ),
				    SCREEN_NUMBER ),
		       canvas_win );
  xv_set( IWindow->canvas1, XV_KEY_DATA, WIN_IHANDLER_KEY, ihandler );
  ihandler->display( image );

  // Set the top window property so that the window manager pays some
  // attention to the canvas color map.
  Atom atomcolormapwindows= XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
  XChangeProperty( dpy, top_win, atomcolormapwindows, XA_WINDOW, 32, 
		   PropModeAppend, (unsigned char *)&canvas_win, 1 );

  // Clean up
  delete fullname;
  logger->comment("Image read and displayed");
#endif

  if ((fp = fopen(fullname, "r")) == NULL) {
    logger->comment("Input data file not found!");
    popup_notice(ip->popup,"Input data file not found!");
    delete fullname;
    return( GFM_ERROR );
  }

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

  logger->comment("Dataset loaded");
  delete [xdim*ydim] buffer;
  
  return(GFM_OK);
}

void voltool_quit()
// Exit the application.
{
  delete vren;
  delete main_ihandler;
  delete logger;
#ifdef never
  netLogger::shutdown();
  netVRen::shutdown();
#endif
  
  exit(0);
}

void voltool_render_start( voltool_mainwindow_objects *ip )
// Do a render
{
  gBoundBox bbox( 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 );
  if (!datavol) { // No data file yet loaded
    popup_notice(ip->mainwindow,"No dataset has been loaded!");
    return;
  }
  if (!svol) { // new datavol just got loaded
    logger->comment("Precalculations begin");
    svol= vren->create_sample_volume( bbox, *tfun, 1, &datavol );
    volgob= vren->create_volgob( svol, gTransfm::identity );
    logger->comment("Precalculations complete");
  }
  logger->comment("Settings begin");
  vren->setCamera( gPoint( 3.5, 3.5, 10.5 ), gPoint( 0.5, 0.5, 0.5 ), 
		   gVector( 0.0, 1.0, 0.0 ), 25.0, -2.0, -20.0 );
  vren->setLightInfo( LightInfo( gColor( 1.0, 1.0, 1.0 ),
			       gVector( 0.0, -1.0, -0.0 ), 
			       gColor( 0.3, 0.3, 0.3 ) ) );
  vren->setQualityMeasure( QualityMeasure() );
  vren->setGeometry( volgob );
  logger->comment("Settings complete; beginning render");
  vren->StartRender( main_ihandler->xsize(), main_ihandler->ysize() );
  logger->comment("Render complete");
}
