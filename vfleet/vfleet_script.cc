/****************************************************************************
 * vfleet_script.cc
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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

/* This module provides scripting functionality for VFleet */

#include <stdlib.h>
#ifdef ATTCC
#include <osfcn.h>
#define F_OK 0
#define R_OK 4
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <math.h>

#include <tcl.h>

#include <Xm/Xm.h>
#include <Mrm/MrmAppl.h>

#include "vfleet.h"

#if ((TCL_MAJOR_VERSION >= 8) && (TCL_MINOR_VERSION >= 4))
#define TCL_POST_8_4 1
#endif

int vfleet_tcl_script_pending;
static int abort_script_flag= 0;

static char* script_name= NULL;


static Tcl_Interp *tcl_interp= NULL;

static int check_abort_tcl()
{
  if (abort_script_flag) {
    Tcl_AppendResult(tcl_interp, "Script Aborted!", NULL);
    abort_script_flag= 0;
    return 1;
  }
  else return 0;
}

#ifdef TCL_POST_8_4
static int tclRenderCmd( ClientData clientData, Tcl_Interp *interp,
			int argc, const char *argv[] )
#else
static int tclRenderCmd( ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[] )
#endif
{
  if (argc != 1) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    do_render();
    watch_events_and_await_render();
    return TCL_OK;
  }
}

#ifdef TCL_POST_8_4
static int tclRotateCmd( ClientData clientData, Tcl_Interp *interp,
			int argc, const char *argv[] )
#else
static int tclRotateCmd( ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[] )
#endif
{
  if (argc != 5) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }

  if (check_abort_tcl()) return TCL_ERROR;
  else {
    double axis_x, axis_y, axis_z, angle;
    int code= 0;
    
    code= Tcl_GetDouble(tcl_interp, argv[1], &angle);
    if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[2], &axis_x);
    if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[3], &axis_y);
    if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[4], &axis_z);
    if (code!=TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
		       argv[0],argv[1],argv[2],argv[3],argv[4],"\"",NULL);
      return code;
    }
    
    gVector axis= gVector(axis_x,axis_y,axis_z);
    orient_model( *gTransfm::rotation( &axis, angle )
		 * current_model_trans() );
    quick_event_check();
    return TCL_OK;
  }
}

#ifdef TCL_POST_8_4
static int tclOrientCmd( ClientData clientData, Tcl_Interp *interp,
			 int argc, const char *argv[] )
#else
static int tclOrientCmd( ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[] )
#endif
{
  if (argc != 5) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }

  if (check_abort_tcl()) return TCL_ERROR;
  else {
    double axis_x, axis_y, axis_z, angle;
    int code= 0;
    
    code= Tcl_GetDouble(tcl_interp, argv[1], &angle);
    if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[2], &axis_x);
    if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[3], &axis_y);
    if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[4], &axis_z);
    if (code!=TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
		       argv[0],argv[1],argv[2],argv[3],argv[4],"\"",NULL);
      return code;
    }
    
    gVector axis= gVector(axis_x,axis_y,axis_z);
    orient_model( *gTransfm::rotation( &axis, angle ) );
    quick_event_check();
    return TCL_OK;
  }
}

#ifdef TCL_POST_8_4
static int tclCameraCmd( ClientData clientData, Tcl_Interp *interp,
			 int argc, const char *argv[] )
#else
static int tclCameraCmd( ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[] )
#endif
{
  if (check_abort_tcl()) return TCL_ERROR;
  else {

    if (!strcmp(argv[1],"set_from")) {
      // Setting from point
      if (argc != 5) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      double x, y, z;

      int code= Tcl_GetDouble(tcl_interp, argv[2], &x);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[3], &y);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[4], &z);
      if (code!=TCL_OK) {
	Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
			 argv[0],argv[1],argv[2],argv[3],argv[4],"\"",NULL);
	return code;
      }

      Camera* cam= current_camera();

      Camera new_cam( gPoint(x,y,z), cam->at(), cam->updir(),
		      cam->fov(), cam->hither_dist(), cam->yon_dist(),
		      cam->parallel_proj() );
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"set_at")) {
      // Setting at point
      if (argc != 5) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      double x, y, z;

      int code= Tcl_GetDouble(tcl_interp, argv[2], &x);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[3], &y);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[4], &z);
      if (code!=TCL_OK) {
	Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
			 argv[0],argv[1],argv[2],argv[3],argv[4],"\"",NULL);
	return code;
      }

      Camera* cam= current_camera();

      Camera new_cam( cam->from(), gPoint(x,y,z), cam->updir(),
		      cam->fov(), cam->hither_dist(), cam->yon_dist(),
		      cam->parallel_proj() );
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"set_up")) {
      // Setting up direction
      if (argc != 5) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      double x, y, z;

      int code= Tcl_GetDouble(tcl_interp, argv[2], &x);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[3], &y);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[4], &z);
      if (code!=TCL_OK) {
	Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
			 argv[0],argv[1],argv[2],argv[3],argv[4],"\"",NULL);
	return code;
      }

      Camera* cam= current_camera();

      Camera new_cam( cam->from(), cam->at(), gVector(x,y,z),
		      cam->fov(), cam->hither_dist(), cam->yon_dist(),
		      cam->parallel_proj() );
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"set_fovea")) {
      // Setting fovea
      if (argc != 3) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      double fov;

      int code= Tcl_GetDouble(tcl_interp, argv[2], &fov);
      if (code!=TCL_OK) {
	Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
			 argv[0],argv[1],argv[2],"\"",NULL);
	return code;
      }

      Camera* cam= current_camera();

      Camera new_cam( cam->from(), cam->at(), cam->updir(),
		      (float)fov, cam->hither_dist(), cam->yon_dist(),
		      cam->parallel_proj() );
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"set_hither_yon")) {
      // Setting hither and yon
      if (argc != 4) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      double hither, yon;

      int code= Tcl_GetDouble(tcl_interp, argv[2], &hither);
      if (code==TCL_OK) code= Tcl_GetDouble(tcl_interp, argv[3], &yon);
      if (code!=TCL_OK) {
	Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
			 argv[0],argv[1],argv[2],argv[3],"\"",NULL);
	return code;
      }

      Camera* cam= current_camera();

      Camera new_cam( cam->from(), cam->at(), cam->updir(),
		      cam->fov(), (float)(-hither), (float)(-yon),
		      cam->parallel_proj() );
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"parallel")) {
      // Adding cam to list
      if (argc != 2) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      Camera* cam= current_camera();
      Camera new_cam= *current_camera();
      new_cam.set_parallel_proj();
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"perspective")) {
      // Adding cam to list
      if (argc != 2) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      Camera* cam= current_camera();
      Camera new_cam= *current_camera();
      new_cam.set_perspective_proj();
      vfleet_nav_set_camera( new_cam );
    }
    else if (!strcmp(argv[1],"add_cam_to_list")) {
      // Adding cam to list
      if (argc != 2) {
	Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }

      vfleet_nav_list_camera();
    }
    else {
      // Unrecognized
      if (argc != 4) {
	Tcl_AppendResult(tcl_interp, "Unknown subcommand ", argv[0], argv[1], 
			 NULL);
	return TCL_ERROR;
      }
    }

    quick_event_check();
    return TCL_OK;
  }
}

#ifdef TCL_POST_8_4
static int tclResizeImageCmd( ClientData clientData, Tcl_Interp *interp,
			      int argc, const char *argv[] )
#else
static int tclResizeImageCmd( ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[] )
#endif
{
  if (argc != 3) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    int width;
    int height;
    int code;
    if ((code=Tcl_GetInt(tcl_interp, argv[1], &width)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if ((code=Tcl_GetInt(tcl_interp, argv[2], &height)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    
    resize_image( width, height );
    quick_event_check();
    return TCL_OK;
  }
  return TCL_OK; // not reached
}

#ifdef TCL_POST_8_4
static int tclSaveImageCmd( ClientData clientData, Tcl_Interp *interp,
			    int argc, const char *argv[] )
#else
static int tclSaveImageCmd( ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[] )
#endif
{
  if (argc != 2) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    char* tstr= strdup(argv[1]);
    save_image( tstr );
    free(tstr);
    quick_event_check();
    return TCL_OK;
  }
}

#ifdef TCL_POST_8_4
static int tclLoadDataCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, const char *argv[] )
#else
static int tclLoadDataCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[] )
#endif
{
  if (argc != 2) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    char* tstr= strdup(argv[1]);
    if ( load_data(tstr) ) {
      quick_event_check();
      free(tstr);
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(tcl_interp, "Data load failed on ", argv[1], NULL);
      free(tstr);
      return TCL_ERROR;
    }
  }
  return TCL_OK; // not reached
}

#ifdef TCL_POST_8_4
static int tclLoadRmtDataCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, const char *argv[] )
#else
static int tclLoadRmtDataCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[] )
#endif
{
  if (argc != 2) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    char* tstr= strdup(argv[1]);
    if ( load_remote_data(tstr) ) {
      quick_event_check();
      free(tstr);
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(tcl_interp, 
		       "Remote data load failed on ", argv[1], NULL);
      free(tstr);
      return TCL_ERROR;
    }
  }
  return TCL_OK; // not reached
}

#ifdef TCL_POST_8_4
static int tclReplaceDataCmd( ClientData clientData, Tcl_Interp *interp,
			      int argc, const char *argv[] )
#else
static int tclReplaceDataCmd( ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[] )
#endif
{
  if (argc != 3) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  int which_dvol;
  int code;
  if ((code=Tcl_GetInt(tcl_interp, argv[1], &which_dvol)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }

  which_dvol--; // count from zero rather than one
    
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    char* tstr= strdup(argv[2]);
    if ( load_data(tstr,which_dvol) ) {
      quick_event_check();
      free(tstr);
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(tcl_interp, "Data replace failed on ",argv[2], NULL);
      free(tstr);
      return TCL_ERROR;
    }
  }
  return TCL_OK; // not reached
}

#ifdef TCL_POST_8_4
static int tclReplaceRmtDataCmd( ClientData clientData, Tcl_Interp *interp,
				 int argc, const char *argv[] )
#else
static int tclReplaceRmtDataCmd( ClientData clientData, Tcl_Interp *interp,
				 int argc, char *argv[] )
#endif
{
  if (argc != 3) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  int which_dvol;
  int code;
  if ((code=Tcl_GetInt(tcl_interp, argv[1], &which_dvol)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Syntax error in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }

  which_dvol--; // count from zero rather than one
    
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    char* tstr= strdup(argv[2]);
    if ( load_remote_data(tstr,which_dvol) ) {
      quick_event_check();
      free(tstr);
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(tcl_interp, 
		       "Remote data replace failed on ",argv[2], NULL);
      free(tstr);
      return TCL_ERROR;
    }
  }
  return TCL_OK; // not reached
}

#ifdef TCL_POST_8_4
static int tclSetCtrlCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, const char *argv[] )
#else
static int tclSetCtrlCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[] )
#endif
{
  if (argc != 3) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }

  if (check_abort_tcl()) return TCL_ERROR;

  int intval;
  double dblval;
  int code;
  int all_flags_on= 0xffffffff;
  VRenOptions new_opts= main_vren->getOptionFlags();
  if (!strncmp(argv[1],"use_light",strlen("use_light"))) {
    if ((code=Tcl_GetBoolean(tcl_interp, argv[2], &intval)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Unrecognized parameter value in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if (intval) new_opts |= OPT_USE_LIGHTING;
    else new_opts &= ( all_flags_on ^ OPT_USE_LIGHTING );
    if (main_vren) main_vren->setOptionFlags( new_opts );
  }
  else if (!strncmp(argv[1],"fast_light",strlen("fast_light"))) {
    if ((code=Tcl_GetBoolean(tcl_interp, argv[2], &intval)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Unrecognized parameter value in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if (intval) new_opts |= OPT_FAST_LIGHTING;
    else new_opts &= ( all_flags_on ^ OPT_FAST_LIGHTING );
    if (main_vren) main_vren->setOptionFlags( new_opts );
  }
  else if (!strncmp(argv[1],"trilin",strlen("trilin"))) {
    if ((code=Tcl_GetBoolean(tcl_interp, argv[2], &intval)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Unrecognized parameter value in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if (intval) new_opts |= OPT_TRILINEAR_INTERP;
    else new_opts &= ( all_flags_on ^ OPT_TRILINEAR_INTERP );
    if (main_vren) main_vren->setOptionFlags( new_opts );
  }
  else if (!strncmp(argv[1],"specular",strlen("specular"))) {
    if ((code=Tcl_GetBoolean(tcl_interp, argv[2], &intval)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Unrecognized parameter value in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if (intval) new_opts |= OPT_SPECULAR_LIGHTING;
    else new_opts &= ( all_flags_on ^ OPT_SPECULAR_LIGHTING );
    if (main_vren) main_vren->setOptionFlags( new_opts );
  }
  else if (!strncmp(argv[1],"mipmap",strlen("mipmap"))) {
    if ((code=Tcl_GetBoolean(tcl_interp, argv[2], &intval)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Unrecognized parameter value in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if (intval) new_opts |= OPT_THREED_MIPMAP;
    else new_opts &= ( all_flags_on ^ OPT_THREED_MIPMAP );
    if (main_vren) main_vren->setOptionFlags( new_opts );
  }
  else if (!strncmp(argv[1],"opacity",strlen("opacity"))) {
    if ((code=Tcl_GetDouble(tcl_interp, argv[2], &dblval)) != TCL_OK) {
      Tcl_AppendResult(tcl_interp, "Unrecognized parameter value in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
      return code;
    }
    if (main_svol) main_svol->set_size_scale( (float)dblval );
  }
  else {
    Tcl_AppendResult(tcl_interp, "Unrecognized parameter in call \"",
		       argv[0],argv[1],argv[2],"\"", NULL);
    return TCL_ERROR;
  }

  vfleet_update_ren_ctrl_widgets();
  return TCL_OK;
}

#ifdef TCL_POST_8_4
static int tclLoadTfunCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, const char *argv[] )
#else
static int tclLoadTfunCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[] )
#endif
{
  if (argc != 2) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else {
    if ( load_tfun(argv[1]) ) {
      quick_event_check();
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(tcl_interp, "tfun load failed on ", argv[1], NULL);
      return TCL_ERROR;
    }
  }
  return TCL_OK; // not reached
}

#ifdef TCL_POST_8_4
static int tclQuitVFleetCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, const char *argv[] )
#else
static int tclQuitVFleetCmd( ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[] )
#endif
{
  if (argc != 1) {
    Tcl_AppendResult(tcl_interp, "wrong # args in ", argv[0], NULL);
    return TCL_ERROR;
  }
  if (check_abort_tcl()) return TCL_ERROR;
  else vfleet_quit();
  return TCL_OK; // not reached
}

void vfleet_tcl_init()
{
  tcl_interp= Tcl_CreateInterp();
  Tcl_CreateCommand(tcl_interp, "render", tclRenderCmd, 
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "rotate", tclRotateCmd, 
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "resize_image", tclResizeImageCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "save_image", tclSaveImageCmd, 
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "load_data", tclLoadDataCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "replace_data", tclReplaceDataCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "load_remote_data", tclLoadRmtDataCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "replace_remote_data", tclReplaceRmtDataCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "load_tfun", tclLoadTfunCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "set_control", tclSetCtrlCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "quit_vfleet", tclQuitVFleetCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "orient", tclOrientCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(tcl_interp, "camera", tclCameraCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

}

void vfleet_tcl_expect_script( char* fname )
{
  // Note that the script exists, but execute it later to avoid confusing X.
  if (script_name) delete [] script_name;
  script_name= new char[ strlen(fname)+1 ];
  strcpy( script_name, fname );
  vfleet_tcl_script_pending= 1;
}

int vfleet_tcl_exec_script()
{
  // This must be called from top level, as opposed to from an X callback,
  // to avoid confusing X.
  if (vfleet_tcl_script_pending) {
    vfleet_tcl_script_pending= 0;
    int code= Tcl_EvalFile(tcl_interp, script_name);

    if (code==TCL_OK) {
      char* msg_string= new char[strlen(script_name)+64];
      sprintf(msg_string,"Finished running script %s",script_name);
      logger->comment(msg_string);
      delete [] msg_string;
      return 0;
    }
    else {
      char* msg_string= new char[strlen(tcl_interp->result) + 64];
      sprintf(msg_string,"Tcl error: %s",tcl_interp->result);

      logger->comment(msg_string);
      delete [] msg_string;
      return -1;
    }
  }
  else return 0;
}

static void tcl_script_cb( Widget w,
			   XtPointer blah,
			   XmFileSelectionBoxCallbackStruct *call_data )
{
  char *the_name, *label_name, *s;
  char *msg_string;

  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  for (s = the_name; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  msg_string= new char[ strlen(the_name)+64 ];
  
  if (! *label_name) {
    sprintf(msg_string,"%s not a valid script file!",the_name);
    logger->comment(msg_string);
    return;
  }
  
  if (access(the_name,R_OK)) {
    sprintf(msg_string,"%s doesn't exist or is not readable!",the_name);
    logger->comment(msg_string);
    return;
  }

  sprintf(msg_string,"Loading script file %s",the_name);
  logger->comment(msg_string);
  
  vfleet_tcl_expect_script( the_name );

  XtUnmanageChild(w);
  XmUpdateDisplay(w);

  delete msg_string;
}

static void abort_script_cb( Widget w,
			    XtPointer blah,
			    XmFileSelectionBoxCallbackStruct *call_data )
{
  abort_script_flag= 1;
}

static MRMRegisterArg mrm_names[] = {
  {"tcl_script_cb", (caddr_t)tcl_script_cb},
  {"abort_script_cb", (caddr_t)abort_script_cb}
};

void vfleet_script_reg()
// Register appropriate callback names
{
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));
}

