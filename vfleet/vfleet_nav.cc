/****************************************************************************
 * vfleet_nav.cc
 * Authors Joel Welling, Robert Earhart
 * Copyright 1993, 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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

/* vfleet_nav.cc */

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

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Scale.h>
#include <Xm/ToggleBG.h>
#include <Xm/SelectioB.h>
#include <Mrm/MrmAppl.h>

#include "vfleet.h"

#include "xdrawih.h"
#include "cball.h"
#include "xcball.h"
#include "tinydraw.h"
#include "xcamhandler.h"

/* Notes-
 */

// Storage space for the various widgets the UI tells us about.
static Widget steering_box_id = NULL;
static Widget drag_window = NULL;

// The transformation of the model wrt. world coordinates
static gTransfm model_trans= gTransfm::identity;

// The camera handler
static XCamHandler* cam_handler= NULL;

// The camera currently in force
static Camera* rendering_camera= NULL;

// Has vfleet_nav_init_camera() been called?
static int cam_initialized= 0;

// The crystal ball controller, and tools for drawing the ball model
static CBall *cball= NULL;
static XDrawImageHandler* cball_ihandler= NULL;
static TDrawGeom* cball_geom= NULL;
static TDrawEngine* cball_engine= NULL;
static TDrawLights* cball_lights= NULL;
static Camera* cball_camera= NULL;
const float CBALL_CAMERA_DIST= 10.0;

const gTransfm& current_model_trans()
{ 
  return model_trans;
}

// Callbacks for the UI.

static TDrawGeom* build_cball_geom()
{
  cball_geom= new TDrawGeom;

  Poly* poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(-1.0, -1.0, -1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 0.3;   poly->vert[0].b= 0.3;
  poly->vert[1]= gPoint(1.0, -1.0, -1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 0.3;   poly->vert[1].b= 0.3;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(-1.0, -1.0, -1.0);
  poly->vert[0].r= 0.3; poly->vert[0].g= 1.0;   poly->vert[0].b= 0.3;
  poly->vert[1]= gPoint(-1.0, 1.0, -1.0);
  poly->vert[1].r= 0.3; poly->vert[1].g= 1.0;   poly->vert[1].b= 0.3;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(-1.0, -1.0, -1.0);
  poly->vert[0].r= 0.3; poly->vert[0].g= 0.3;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(-1.0, -1.0, 1.0);
  poly->vert[1].r= 0.3; poly->vert[1].g= 0.3;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(1.0, -1.0, -1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(1.0, -1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(-1.0, 1.0, -1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(-1.0, 1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(1.0, 1.0, -1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(1.0, 1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(1.0, 1.0, -1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(-1.0, 1.0, -1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(1.0, 1.0, 1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(-1.0, 1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(-1.0, -1.0, 1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(1.0, -1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(-1.0, -1.0, 1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(-1.0, 1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(1.0, -1.0, -1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(1.0, 1.0, -1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  poly= new Poly;
  poly->n= 2;
  poly->vert[0]= gPoint(1.0, -1.0, 1.0);
  poly->vert[0].r= 1.0; poly->vert[0].g= 1.0;   poly->vert[0].b= 1.0;
  poly->vert[1]= gPoint(1.0, 1.0, 1.0);
  poly->vert[1].r= 1.0; poly->vert[1].g= 1.0;   poly->vert[1].b= 1.0;
  cball_geom->add( poly );

  cball_geom->add_sphere( 1.0, 1.0, 0.0 );

  return cball_geom;
}

static void cball_draw_init()
{
  static int initialized= 0;

  if (!initialized) {
    Drawable root;
    int x, y;
    unsigned int width, height, border_width, depth_dummy;
    if ( !XGetGeometry(XtDisplay(steering_box_id), XtWindow(steering_box_id),
		       &root, &x, &y, &width, &height,
		       &border_width, &depth_dummy) ) {
	fprintf(stderr,"XImageHandler::get_window_size: fatal error!\n");
	exit(-1);
    }
    cball_ihandler= new XDrawImageHandler( XtDisplay(steering_box_id),
					   DefaultScreen(
					       XtDisplay(steering_box_id)),
					   XtWindow(steering_box_id),
					   width, height );
    cball_engine= new TDrawEngine( cball_ihandler );
    cball_lights= new TDrawLights( gColor(0.8, 0.8, 0.8),
				   current_lighting_dir,
				   gColor(0.3, 0.3, 0.3) );

    cball_engine->set_lights( *cball_lights );
    cball_engine->set_camera(cball_camera);
    cball_geom= build_cball_geom();

    initialized= 1;
  }
}

void orient_model( gTransfm& trans )
{
  cball_draw_init();
  model_trans= trans;
  cball_engine->set_world_trans( trans, trans );
  cball_engine->draw( cball_geom );
}

static void cball_draw_shutdown()
{
  delete cball_engine;
  delete cball_geom;
  delete cball_lights;
  delete cball_ihandler;
}

static void steering_motion_cb( gTransfm *roll, void *cb_data,
				      CBall *calling_cball )
{
  orient_model( *roll * model_trans );
}

static void steering_box_expose_cb (Widget w, int *uil_id, caddr_t call_data)
{
  orient_model( model_trans );
}

static MRMRegisterArg mrm_names[] = {
    {"steering_box_expose_cb", (caddr_t) steering_box_expose_cb}
};

static void cam_update_cb( XCamHandler* handler, const Camera& new_cam )
{
  gPoint at= cball_camera->at();
  gPoint from= at + (new_cam.pointing_dir() * -CBALL_CAMERA_DIST);
  *cball_camera= Camera( from,
			 at, 
			 new_cam.updir(),
			 cball_camera->fov(),
			 cball_camera->hither_dist(), 
			 cball_camera->yon_dist(),
			 new_cam.parallel_proj() );
  cball_engine->draw( cball_geom );
}

void vfleet_nav_init( Widget drag_window_in )
{
  // The following is made necessary because the AT&T CC compiler seems
  // to fail to do the initialization properly sometimes.
  model_trans= gTransfm::identity;

  // Window to use for drag input to the handler
  drag_window= drag_window_in;

  // Initial camera for use with crystal ball
  cball_camera= new Camera( gPoint( 0.0, 0.0, CBALL_CAMERA_DIST ),
			    gPoint( 0.0, 0.0, 0.0 ),
			    gVector( 0.0, 1.0, 0.0 ),
			    15.0, -5.0, -15.0 );

  // Just for now, use this handy camera.  This will be replaced
  // when the user loads a datavolume.
  rendering_camera= new Camera( *cball_camera );

  cam_handler= new XCamHandler( app_shell, mrm_id, rendering_camera,
				cam_update_cb );
  cam_handler->register_drag_window( drag_window );
}

void vfleet_nav_reg()
{
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));
}

void vfleet_nav_cleanup()
{
  cball_draw_shutdown();
}

void vfleet_nav_create( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case k_steering_box_id:
    steering_box_id= w;
    cball= new XmCBall( w, steering_motion_cb, NULL );
    break;
  };
}

void vfleet_nav_open( Widget w, int *tag, caddr_t cb )
{
  switch (*tag) {
  case k_camera_controls_id:
    {
      cam_handler->popup();
    }
    break;
  }
}

Camera* current_camera()
{
  return rendering_camera;
}

void vfleet_nav_init_camera( const gBoundBox& bbox )
{
  delete rendering_camera;
  delete cam_handler;

  rendering_camera= XCamHandler::cam_from_viewed_volume( bbox );
    
  cam_handler= new XCamHandler( app_shell, mrm_id, rendering_camera,
				cam_update_cb );
  cam_handler->register_drag_window( drag_window );

  cam_initialized= 1;
}

void vfleet_nav_set_camera( const Camera& cam_in )
{
  cam_handler->cam_to_dlog( cam_in );
  cam_handler->set();
}

void vfleet_nav_list_camera()
{
  cam_handler->add_displayed_cam_to_list();
}

int vfleet_nav_cam_initialized()
{
  return( cam_initialized );
}
