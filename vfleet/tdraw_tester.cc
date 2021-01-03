/****************************************************************************
 * tdraw_tester.cc
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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
#include <sys/time.h>
#include <math.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/CascadeB.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>

#include "camera.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "xdrawih.h"
#include "cball.h"
#include "xcball.h"
#include "tinydraw.h"

// Constants
static const int MAX_ARGS= 20;

// The drawing area
static Widget canvas;
static Widget zbuffer_button;
static Widget phong_button;
static Widget spec_button;

// The crystal ball controller, and tools for drawing the ball model
static CBall *cball= NULL;
static XDrawImageHandler* cball_ihandler= NULL;
static TDrawGeom* cball_geom= NULL;
static TDrawEngine* cball_engine= NULL;
static TDrawLights* cball_lights= NULL;
static Camera* cball_camera= NULL;

// Rotation of model
static gTransfm model_trans= gTransfm::identity;

// Callbacks for the UI.

static TDrawGeom* build_cball_geom()
{
  cball_geom= new TDrawGeom;

  cball_geom->add_sphere( 0.7, 0.7, 0.0 );

  gPoint strip_pts[3];
  gVector strip_norms[3];
  gColor strip_clrs[3];
  for (int i=0; i<100; i++) {
    float frac= i/99.0;
    float theta= 2.0*M_PI*frac;
    float x= (4.0*frac)-2.0;
    float y;
    float z;
    if (i%2) {
      y= 1.5*cos(theta);
      z= 1.5*sin(theta);
    }
    else {
      y= -1.5*cos(theta);
      z= -1.5*sin(theta);
    }
    strip_pts[i%3]= gPoint(x,y,z);
    strip_norms[i%3]= gVector(0.0, -sin(theta), cos(theta));
    strip_clrs[i%3]= gColor(frac, 0.3, 1.0-frac);
    Poly* poly;
    if (i>2) {
      poly= new Poly;
      poly->n= 3;
      if (i%2) {
	poly->vert[1]= strip_pts[(i-2)%3];
	poly->vert[1].nx= strip_norms[(i-2)%3].x();
	poly->vert[1].ny= strip_norms[(i-2)%3].y();
	poly->vert[1].nz= strip_norms[(i-2)%3].z();
	poly->vert[1].r= strip_clrs[(i-2)%3].r();
	poly->vert[1].g= strip_clrs[(i-2)%3].g();
	poly->vert[1].b= strip_clrs[(i-2)%3].b();

	poly->vert[0]= strip_pts[(i-1)%3];
	poly->vert[0].nx= strip_norms[(i-1)%3].x();
	poly->vert[0].ny= strip_norms[(i-1)%3].y();
	poly->vert[0].nz= strip_norms[(i-1)%3].z();
	poly->vert[0].r= strip_clrs[(i-1)%3].r();
	poly->vert[0].g= strip_clrs[(i-1)%3].g();
	poly->vert[0].b= strip_clrs[(i-1)%3].b();

	poly->vert[2]= strip_pts[i%3];
	poly->vert[2].nx= strip_norms[i%3].x();
	poly->vert[2].ny= strip_norms[i%3].y();
	poly->vert[2].nz= strip_norms[i%3].z();
	poly->vert[2].r= strip_clrs[i%3].r();
	poly->vert[2].g= strip_clrs[i%3].g();
	poly->vert[2].b= strip_clrs[i%3].b();
      }
      else {
	poly->vert[0]= strip_pts[(i-2)%3];
	poly->vert[0].nx= strip_norms[(i-2)%3].x();
	poly->vert[0].ny= strip_norms[(i-2)%3].y();
	poly->vert[0].nz= strip_norms[(i-2)%3].z();
	poly->vert[0].r= strip_clrs[(i-2)%3].r();
	poly->vert[0].g= strip_clrs[(i-2)%3].g();
	poly->vert[0].b= strip_clrs[(i-2)%3].b();

	poly->vert[1]= strip_pts[(i-1)%3];
	poly->vert[1].nx= strip_norms[(i-1)%3].x();
	poly->vert[1].ny= strip_norms[(i-1)%3].y();
	poly->vert[1].nz= strip_norms[(i-1)%3].z();
	poly->vert[1].r= strip_clrs[(i-1)%3].r();
	poly->vert[1].g= strip_clrs[(i-1)%3].g();
	poly->vert[1].b= strip_clrs[(i-1)%3].b();

	poly->vert[2]= strip_pts[i%3];
	poly->vert[2].nx= strip_norms[i%3].x();
	poly->vert[2].ny= strip_norms[i%3].y();
	poly->vert[2].nz= strip_norms[i%3].z();
	poly->vert[2].r= strip_clrs[i%3].r();
	poly->vert[2].g= strip_clrs[i%3].g();
	poly->vert[2].b= strip_clrs[i%3].b();
      }
      cball_geom->add( poly );
    }
  }

  return cball_geom;
}

static void draw_model()
{
  cball_engine->set_spec( XmToggleButtonGadgetGetState(spec_button) );
  cball_engine->set_phong( XmToggleButtonGadgetGetState(phong_button) );
  if (XmToggleButtonGadgetGetState(zbuffer_button))
      cball_engine->draw_nicely( cball_geom );
  else cball_engine->draw( cball_geom );
}

static void cball_draw_init()
{
  static int initialized= 0;

  if (!initialized) {
    Drawable root;
    int x, y;
    unsigned int width, height, border_width, depth_dummy;
    if ( !XGetGeometry(XtDisplay(canvas), XtWindow(canvas),
		       &root, &x, &y, &width, &height,
		       &border_width, &depth_dummy) ) {
	fprintf(stderr,"XImageHandler::get_window_size: fatal error!\n");
	exit(-1);
    }
    cball_ihandler= new XDrawImageHandler( XtDisplay(canvas),
					   DefaultScreen(XtDisplay(canvas)),
					   XtWindow(canvas),
					   width, height );
    cball_engine= new TDrawEngine( cball_ihandler );
    gVector cball_ltdir= gVector(0.0, -5.0, -10.0);
    cball_lights= new TDrawLights( gColor(0.8, 0.8, 0.8),
				   cball_ltdir,
				   gColor(0.3, 0.3, 0.3) );

    cball_engine->set_lights( *cball_lights );
    cball_camera= new Camera( gPoint( 0.0, 0.0, 10.0 ),
			      gPoint( 0.0, 0.0, 0.0 ),
			      gVector( 0.0, 1.0, 0.0 ),
			      25.0, -5.0, -15.0 );
    cball_engine->set_camera( cball_camera );
    cball_geom= build_cball_geom();

    draw_model();

    initialized= 1;
  }
}

void orient_model( gTransfm& trans )
{
  model_trans= trans;
  cball_engine->set_world_trans( trans, trans );
  draw_model();
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

static void canvas_resize_cb (Widget w,
			      XtPointer foo,
			      XtPointer bar) {
  if (cball_ihandler)
    cball_ihandler->resize();
}

static void canvas_expose_cb (Widget w, int *uil_id, caddr_t call_data)
{
  if (!cball) {
    cball= new XmCBall( w, steering_motion_cb, NULL );
    cball_draw_init();
  }
  else cball_engine->redraw();
}

static void exit_cb (Widget w, XtPointer data, XtPointer cb) 
{
  cball_draw_shutdown();
  exit(0);
}

int main(int argc,
	 char **argv) {
  Widget toplevel;
  Widget main_window;
  Widget panel_frame;
  Widget button;
  Widget menu_bar;
  Widget menu_pane;
  Widget work_area_form;
  XmString title_string;
  XmString dist_mstring= NULL;
  XtAppContext app_context;

  Arg    args[MAX_ARGS];
  int    i, n;

  // The following is made necessary because the AT&T CC compiler seems
  // to fail to do the initialization properly sometimes.
  model_trans= gTransfm::identity;

  toplevel = XtAppInitialize(&app_context, "TDraw_tester", 
			     (XrmOptionDescList)NULL, 0, 
			     &argc, argv, 
			     (String*)NULL, (ArgList)NULL, 0);

  /* Create main window */
  n = 0;
  XtSetArg(args[n],XmNcommandWindowLocation,XmCOMMAND_BELOW_WORKSPACE); n++;
  main_window = XmCreateMainWindow(toplevel, "main", args, n);

  /* Create menu_bar in main_window */
  n = 0;
  menu_bar = XmCreateMenuBar(main_window, "menu_bar", args, n);
  XtManageChild(menu_bar);
  
  n = 0;
  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, n);
  
  /* Create File Menu */
  title_string= XmStringCreate("File", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'F'); n++;
  button = XmCreateCascadeButton(menu_bar, "file", args, n);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);
  
  /* Control buttons */
  title_string= XmStringCreate("Z Buffer", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'Z'); n++;
  zbuffer_button = XmCreateToggleButtonGadget(menu_pane, "zbuffer", args, n);
  if (title_string) XmStringFree(title_string);
  XtManageChild(zbuffer_button);

  title_string= XmStringCreate("Phong", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'P'); n++;
  phong_button = XmCreateToggleButtonGadget(menu_pane, "phong", args, n);
  if (title_string) XmStringFree(title_string);
  XtManageChild(phong_button);

  title_string= XmStringCreate("Specular", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'S'); n++;
  spec_button = XmCreateToggleButtonGadget(menu_pane, "specular", args, n);
  if (title_string) XmStringFree(title_string);
  XtManageChild(spec_button);

  /* Create Exit button */
  title_string= XmStringCreate("Exit", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'E'); n++;
  button = XmCreatePushButton(menu_pane, "exit", args, n);
  XtAddCallback(button, XmNactivateCallback, exit_cb, NULL);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);

  /* Create work area form */
  n= 0;
  work_area_form = XmCreateForm(main_window, "work_area_form", args, n);

  /* Create Drawing Canvas */
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNheight, 300); n++;
  XtSetArg(args[n], XmNwidth, 300); n++;
  canvas = XmCreateDrawingArea(work_area_form, "canvas", args, n);
  XtAddCallback(canvas, XmNexposeCallback, 
		(XtCallbackProc)canvas_expose_cb, NULL);
  XtManageChild(canvas);

  XtManageChild(work_area_form);
  XmMainWindowSetAreas(main_window, menu_bar, NULL,
		       NULL, NULL, work_area_form);

  XtManageChild(main_window);

  XtRealizeWidget(toplevel);

  XFlush(XtDisplay(main_window)); 

  XtAppMainLoop(app_context);

  return(0);
}

