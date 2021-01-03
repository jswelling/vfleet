/****************************************************************************
 * xcamhandler.cc
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

#include <stdio.h>
#include <stdlib.h>
#ifdef ATTCC
#include <osfcn.h>
#define F_OK 0
#else
#include <unistd.h>
#endif

#include <Xm/Xm.h>
#include <Xm/TextF.h>
#include <Xm/Scale.h>
#include <Xm/ToggleBG.h>
#include <Mrm/MrmAppl.h>

#include "geometry.h"
#include "camera.h"
#include "vfleet.h"
#include "xcamhandler.h"

const float INITIAL_DOLLY_FRACTION= 0.1;
const float INITIAL_ROTATE_ANGLE= 5.0;

int XCamHandler::initialized= 0;

struct xcamhandler_file_write_warning_data {
  char *fname;
  Widget to_be_closed;
  XCamHandler *handler;
};

XCamHandler::XCamHandler( Widget parent_widget_in, MrmHierarchy mrm_id_in,
			  Camera* cam_to_manage,
			  void (cam_update_cb_in)( XCamHandler*,
						   const Camera& ) )
{
  mrm_id= mrm_id_in;
  parent_widget= parent_widget_in;
  managed_camera= cam_to_manage;
  cam_update_cb= cam_update_cb_in;
  drag_start_valid= 0;
  cam_list_iter= new dList_iter<Camera*>(cam_list);
  cams_in_list= 0;
  cam_list_current= NULL;

  save_list_file_dlog= NULL;

  // This will be set to something valid if a drag window is defined.
  drag_window= NULL;

  // The following are set appropriately during creation process
  widget= NULL;
  list_count_widget= NULL;
  rotate_widget= NULL;
  dolly_widget= NULL;
  hither_widget= NULL;
  yon_widget= NULL;
  from_x_widget= NULL;
  from_y_widget= NULL;
  from_z_widget= NULL;
  at_x_widget= NULL;
  at_y_widget= NULL;
  at_z_widget= NULL;
  up_x_widget= NULL;
  up_y_widget= NULL;
  up_z_widget= NULL;
  fovea_scale_widget= NULL;
  shift_at_tb= NULL;
  shift_both_tb= NULL;
  parallel_proj_tb= NULL;

  if (!initialized) {
    MRMRegisterArg mrm_names[6];
    mrm_names[0].name= "xcamhandler_create_cb";
    mrm_names[0].value= (caddr_t)XCamHandler::create_cb;
    mrm_names[1].name= "xcamhandler_expose_cb";
    mrm_names[1].value= (caddr_t)XCamHandler::expose_cb;
    mrm_names[2].name= "xcamhandler_set_cb";
    mrm_names[2].value= (caddr_t)XCamHandler::set_cb;
    mrm_names[3].name= "xcamhandler_reset_cb";
    mrm_names[3].value= (caddr_t)XCamHandler::reset_cb;
    mrm_names[4].name= "xcamhandler_button_press_cb";
    mrm_names[4].value= (caddr_t)XCamHandler::button_press_cb;
    mrm_names[5].name= "xcamhandler_save_list_file_cb";
    mrm_names[5].value= (caddr_t)XCamHandler::save_list_file_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    initialized= 1;
  }

  register_object_name();
  MrmType mrm_class;
  if (!MrmFetchWidget(mrm_id, "xcamhandler_dlog", parent_widget,
		      &widget, &mrm_class) || !widget) {
    fprintf(stderr,
	    "XCamHandler::XCamHandler: can't get UIL resource!\n");
    return;
  }

}

XCamHandler::~XCamHandler()
{
  delete cam_list_iter;

  Camera* this_cam;
  while (this_cam= cam_list.head()) delete cam_list.pop();

  if (drag_window) {
    // remove old callback
    XtRemoveEventHandler( drag_window, ButtonPressMask | ButtonReleaseMask, 
			  FALSE, (XtEventHandler)window_input_cb, this );
  }

  if (save_list_file_dlog) XtDestroyWidget(save_list_file_dlog);

  if (widget) XtDestroyWidget(widget);
}

void XCamHandler::register_object_name()
{
  // This sticks a pointer to this XTfunHandler into the mrm hierarchy
  MrmRegisterArg names;
  names.name= "owner_object";
  names.value= (caddr_t)this;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
}

XCamHandler* XCamHandler::get_object( Widget w )
{
  // This recovers the object pointer registered with register_object_name
  // from the widget
  Arg arg;
  void *object;
  XtSetArg( arg, XmNuserData, &object );
  XtGetValues( w, &arg, 1 );
  if (!object) 
    fprintf(stderr,
	    "XCamHandler::get_object: couldn't get obj pointer!\n");
  return (XCamHandler*)object;
}

void XCamHandler::create_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->create( w, id, reason );
}

void XCamHandler::create( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 0:
    // cam_list_count_text
    list_count_widget= w;
    break;
  case 1:
    // cam_rotate_text
    rotate_widget= w;
    break;
  case 2:
    // cam_dolly_text
    dolly_widget= w;
    break;
  case 3:
    // cam_hither
    hither_widget= w;
    break;
  case 4:
    // cam_yon
    yon_widget= w;
    break;
  case 5:
    // cam_from_x
    from_x_widget= w;
    break;
  case 6:
    // cam_from_y
    from_y_widget= w;
    break;
  case 7:
    // cam_from_z
    from_z_widget= w;
    break;
  case 8:
    // cam_at_x
    at_x_widget= w;
    break;
  case 9:
    // cam_at_y
    at_y_widget= w;
    break;
  case 10:
    // cam_at_z
    at_z_widget= w;
    break;
  case 11:
    // cam_up_x
    up_x_widget= w;
    break;
  case 12:
    // cam_up_y
    up_y_widget= w;
    break;
  case 13:
    // cam_up_z
    up_z_widget= w;
    break;
  case 14:
    // cam_fov_scale
    fovea_scale_widget= w;
    break;
  case 15:
    // cam_shift_both
    shift_both_tb= w;
    break;
  case 16:
    // cam_shift_at
    shift_at_tb= w;
    break;
  case 17:
    // cam_add_info_pb
    cam_list_add_pb= w;
    break;
  case 18:
    // cam_prev_info_pb
    cam_list_prev_pb= w;
    break;
  case 19:
    // cam_next_info_pb
    cam_list_next_pb= w;
    break;
  case 20:
    // cam_save_list_pb
    cam_list_save_pb= w;
    break;
  case 21:
    // cam_parallel_proj_tb
    parallel_proj_tb= w;
    break;
  default:
    fprintf(stderr,"XCamHandler::create: got %d; out of sync with UIL file!\n",
	    *id);
  }
  if (list_count_widget && rotate_widget && dolly_widget
      && hither_widget && yon_widget 
      && from_x_widget && from_y_widget && from_z_widget
      && at_x_widget && at_y_widget && at_z_widget
      && up_x_widget && up_y_widget && up_z_widget
      && fovea_scale_widget && shift_both_tb && shift_at_tb
      && cam_list_add_pb && cam_list_prev_pb && cam_list_next_pb
      && cam_list_save_pb && parallel_proj_tb ) 
    {
      cam_to_dlog( *managed_camera );
      
      char string[16];
      sprintf(string,"%5d",cams_in_list);
      XmTextFieldSetString( list_count_widget, string );
      
      float view_dist= 
	(managed_camera->at() - managed_camera->from()).length();
      sprintf(string,"%-7.3f",INITIAL_DOLLY_FRACTION * view_dist);
      XmTextFieldSetString( dolly_widget, string );

      sprintf(string,"%-7.3f",INITIAL_ROTATE_ANGLE);
      XmTextFieldSetString( rotate_widget, string );
    }
}

void XCamHandler::expose_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->expose( w, id, reason );
}

void XCamHandler::expose( Widget w, int *id, unsigned long *reason )
{
  // Nothing to do
}

void XCamHandler::popup()
{
  XtManageChild( widget );
}

void XCamHandler::set_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->set();
}

void XCamHandler::set()
{
  Camera new_cam= cam_from_dlog();
  if (cam_update_cb) (*cam_update_cb)( this, new_cam );
  *managed_camera= new_cam;
}

void XCamHandler::reset_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->reset( w, id, reason );
}

void XCamHandler::reset( Widget w, int *id, unsigned long *reason )
{
  cam_to_dlog( *managed_camera );
}

void XCamHandler::button_press_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->button_press( w, id, reason );
}

void XCamHandler::button_press( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 0: // cam_add_info_pb
    add_displayed_cam_to_list();
    break;
  case 1: // cam_prev_info_pb
    {
      if (cam_list_iter->last_entry()) 
	(void)cam_list_iter->prev(); // avoid sticking at end
      Camera** new_cam= cam_list_iter->prev();
      // will get a repeat if we just changed direction
      if (new_cam
	  && (*new_cam == cam_list_current)) new_cam= cam_list_iter->prev();
      if (new_cam) cam_list_current= *new_cam;
      else cam_list_current= NULL;
      if (cam_list_current) cam_to_dlog( *cam_list_current );
      if (cam_list_iter->first_entry())
	XtSetSensitive( cam_list_prev_pb, False );
      XtSetSensitive( cam_list_next_pb, True );
    }
    break;
  case 2: // cam_next_info_pb
    {
      if (cam_list_iter->first_entry()) 
	(void)cam_list_iter->next(); // avoid sticking at start
      Camera** new_cam= cam_list_iter->next();
      // will get a repeat if we just changed direction
      if (new_cam 
	  &&(*new_cam == cam_list_current)) new_cam= cam_list_iter->next();
      if (new_cam) cam_list_current= *new_cam;
      else cam_list_current= NULL;
      if (cam_list_current) cam_to_dlog( *cam_list_current );
      if (cam_list_iter->first_entry())
	XtSetSensitive( cam_list_next_pb, False );
      XtSetSensitive( cam_list_prev_pb, True );
    }
    break;
  case 3: // cam_save_list_pb
    save_list();
    break;
  case 4: // cam_rotate_plus_pb
    rotate( float_from_text_widget( rotate_widget ) );
    break;
  case 5: // cam_rotate_minus_pb
    rotate( - float_from_text_widget( rotate_widget ) );
    break;
  case 6: // cam_dolly_in_pb
    dolly( float_from_text_widget( dolly_widget ) );
    break;
  case 7: // cam_dolly_out_pb
    dolly( - float_from_text_widget( dolly_widget ) );
    break;
  }
}

void XCamHandler::dolly( const float dist )
{
  Camera displayed_cam= cam_from_dlog();
  gPoint from= displayed_cam.from();
  gPoint at= displayed_cam.at();
  gVector look_vec= at - from;
  if (look_vec.length() > fabs(dist)) {
    // Move only look from point
    from= from + ( displayed_cam.pointing_dir() * dist );
  }
  else {
    // Move both look from and look at
    from= from + ( displayed_cam.pointing_dir() * dist );
    at= at + ( displayed_cam.pointing_dir() * dist );
  }
  cam_to_dlog( Camera( from, at, displayed_cam.updir(),
		       displayed_cam.fov(),
		       displayed_cam.hither_dist(), 
		       displayed_cam.yon_dist(), 
		       displayed_cam.parallel_proj() ) );
}

void XCamHandler::rotate( const float degrees )
{
  Camera displayed_cam= cam_from_dlog();
  gVector up= displayed_cam.updir();
  gVector point= displayed_cam.pointing_dir();

  // Get normal component of up
  gVector up_perp= up - ( point * ( up*point ) );
  up_perp.normalize();

  // Get cross vector, and mix the new up vector
  gVector cross= point ^ up_perp;
  gVector new_up= (up_perp * cos(degrees*DegtoRad))
    + (cross * sin(degrees*DegtoRad));

  cam_to_dlog( Camera( displayed_cam.from(), displayed_cam.at(), 
		       new_up,
		       displayed_cam.fov(),
		       displayed_cam.hither_dist(), 
		       displayed_cam.yon_dist(),
		       displayed_cam.parallel_proj() ) );
}

void XCamHandler::drag_shift( const MousePosition& drag_start, 
			      const MousePosition& drag_end ) 
{
  Camera displayed_cam= cam_from_dlog();
  gVector up= displayed_cam.updir();
  gVector point= displayed_cam.pointing_dir();

  // Get normal component of up
  gVector up_perp= up - ( point * ( up*point ) );
  up_perp.normalize();

  // Get a perpendicular vector (normalized since point and up_perp are)
  gVector right_vec= point ^ up_perp;

  float view_length= (displayed_cam.at() - displayed_cam.from()).length();
  float screen_width= view_length * sin( 0.5*DegtoRad*displayed_cam.fov() );
  float dx= drag_end.frac_x() - drag_start.frac_x();
  float dy= drag_end.frac_y() - drag_start.frac_y();

  gVector shift= (right_vec*dx + up_perp*dy) * (-screen_width);

  if (XmToggleButtonGadgetGetState(shift_at_tb)) {
    cam_to_dlog( Camera( displayed_cam.from(), 
			 displayed_cam.at() + shift,
			 displayed_cam.updir(),
			 displayed_cam.fov(),
			 displayed_cam.hither_dist(), 
			 displayed_cam.yon_dist(),
			 displayed_cam.parallel_proj() ) );
  }
  else if (XmToggleButtonGadgetGetState(shift_both_tb)) {
    cam_to_dlog( Camera( displayed_cam.from() + shift, 
			 displayed_cam.at() + shift,
			 displayed_cam.updir(),
			 displayed_cam.fov(),
			 displayed_cam.hither_dist(), 
			 displayed_cam.yon_dist(),
			 displayed_cam.parallel_proj() ) );
  }
}

Camera* XCamHandler::cam_from_viewed_volume( const gBoundBox& bbox )
{
  float xsize= fabs( bbox.xmax() - bbox.xmin() );
  float ysize= fabs( bbox.ymax() - bbox.ymin() );
  float zsize= fabs( bbox.zmax() - bbox.zmin() );

  float biggest= xsize;
  if (ysize>biggest) biggest= ysize;
  if (zsize>biggest) biggest= zsize;

  float view_dist= 5.0 * biggest;

  Camera* result= new Camera( gPoint( 0.0, 0.0, view_dist ), 
			      gPoint( (bbox.xmax()+bbox.xmin())/2.0,
				      (bbox.ymax()+bbox.ymin())/2.0,
				      (bbox.zmax()+bbox.zmin())/2.0),
			      gVector( 0.0, 1.0, 0.0 ), 15.0, 
			      -0.01*view_dist, -1.5*view_dist );
 
  return result;
}

int XCamHandler::int_from_text_widget( Widget w )
{
  char *txtstring= XmTextFieldGetString( w );
  int result= atoi( txtstring );
  XtFree(txtstring);
  return result;
}

int XCamHandler::int_from_text_widget( Widget w, 
					    const int min, const int max )
{
  char *txtstring= XmTextFieldGetString( w );
  int result= atoi( txtstring );
  if ((result<min) || (result>max)) {
    result= (result<min) ? min : ((result>max) ? max : result);
    char fixstring[16];
    sprintf(fixstring, "%-5d", result);
    XmTextFieldSetString( w, fixstring );
  }
  XtFree(txtstring);
  return result;
}

float XCamHandler::float_from_text_widget( Widget w )
{
  char *txtstring= XmTextFieldGetString( w );
  float result= atof(txtstring);
  XtFree(txtstring);
  return result;
}

float XCamHandler::float_from_text_widget( Widget w, const float min, 
						const float max )
{
  char *txtstring= XmTextFieldGetString( w );
  float result= atof(txtstring);
  if ((result<min) || (result>max)) {
    result= (result<min) ? min : ((result>max) ? max : result);
    char fixstring[16];
    sprintf(fixstring, "%-7.3f", result);
    XmTextFieldSetString( w, fixstring );
  }
  XtFree(txtstring);
  return result;
}

Camera XCamHandler::cam_from_dlog()
{
  int fov_scale_val;
  XmScaleGetValue( fovea_scale_widget, &fov_scale_val );

  // Cameras have negative hither and yon, but the dialog shows them
  // as positive.  Check that 0<neg_hither<neg_yon
#ifdef CRAY_ARCH_T3E
  float neg_yon= float_from_text_widget(yon_widget, 0.0, HUGE_VALF);
#else
  float neg_yon= float_from_text_widget(yon_widget, 0.0, HUGE);
#endif
  float neg_hither= float_from_text_widget(hither_widget, 0.0, neg_yon);

  gPoint from_pt( float_from_text_widget( from_x_widget),
		  float_from_text_widget( from_y_widget),
		  float_from_text_widget( from_z_widget) );
  gPoint at_pt( float_from_text_widget( at_x_widget),
		float_from_text_widget( at_y_widget),
		float_from_text_widget( at_z_widget) );
  gVector up_vec( float_from_text_widget( up_x_widget),
		  float_from_text_widget( up_y_widget),
		  float_from_text_widget( up_z_widget) );
  Camera result( from_pt, at_pt, up_vec,
		 (float)fov_scale_val,
		 -neg_hither, -neg_yon,
		 XmToggleButtonGadgetGetState( parallel_proj_tb ) );

  return result;
}

void XCamHandler::cam_to_dlog( const Camera& display_me )
{
  char string[16];

  sprintf(string, "%-7.3f", -display_me.hither_dist());
  XmTextFieldSetString( hither_widget, string );
  sprintf(string, "%-7.3f", -display_me.yon_dist());
  XmTextFieldSetString( yon_widget, string );

  sprintf(string, "%-7.3f", display_me.from().x());
  XmTextFieldSetString( from_x_widget, string );
  sprintf(string, "%-7.3f", display_me.from().y());
  XmTextFieldSetString( from_y_widget, string );
  sprintf(string, "%-7.3f", display_me.from().z());
  XmTextFieldSetString( from_z_widget, string );

  sprintf(string, "%-7.3f", display_me.at().x());
  XmTextFieldSetString( at_x_widget, string );
  sprintf(string, "%-7.3f", display_me.at().y());
  XmTextFieldSetString( at_y_widget, string );
  sprintf(string, "%-7.3f", display_me.at().z());
  XmTextFieldSetString( at_z_widget, string );

  sprintf(string, "%-7.3f", display_me.updir().x());
  XmTextFieldSetString( up_x_widget, string );
  sprintf(string, "%-7.3f", display_me.updir().y());
  XmTextFieldSetString( up_y_widget, string );
  sprintf(string, "%-7.3f", display_me.updir().z());
  XmTextFieldSetString( up_z_widget, string );

  XmScaleSetValue(fovea_scale_widget, (int)display_me.fov());
  XmToggleButtonGadgetSetState( parallel_proj_tb, display_me.parallel_proj(),
				False );
}

void XCamHandler::register_drag_window( Widget drag_window_in )
{
  if (drag_window) {
    // remove old callback
    XtRemoveEventHandler( drag_window, ButtonPressMask | ButtonReleaseMask, 
			  FALSE, (XtEventHandler)window_input_cb, this );
  }

  drag_window= drag_window_in;  

  if (drag_window) 
    XtAddEventHandler( drag_window, ButtonPressMask | ButtonReleaseMask, 
		       FALSE, (XtEventHandler)window_input_cb, this );
}

void XCamHandler::window_input_cb( Widget w, XCamHandler* handler, 
				   XEvent* event )
{
  handler->window_input( w, event );
}

void XCamHandler::window_input( Widget w, XEvent* event )
{
  XWindowAttributes window_attributes;

  if ( ((event->type == ButtonPress) || (event->type == ButtonRelease))
       && (event->xbutton.button == Button2) ) {
    if ( XGetWindowAttributes( XtDisplay(w), XtWindow(w), 
			       &window_attributes ) ) {
      switch (event->type) {
      case ButtonPress:
	{
	  drag_start.maxx = window_attributes.width;
	  drag_start.maxy = window_attributes.height;
	  drag_start.x = event->xbutton.x;
	  drag_start.y = window_attributes.height - event->xbutton.y;
	  drag_start_valid= 1;
	}
	break;
      case ButtonRelease:
	{
	  drag_end.maxx = window_attributes.width;
	  drag_end.maxy = window_attributes.height;
	  drag_end.x = event->xbutton.x;
	  drag_end.y = window_attributes.height - event->xbutton.y;
	  if (drag_start_valid) drag_shift( drag_start, drag_end );
	  drag_start_valid= 0;
	}
	break;
      }
    }
    else fprintf(stderr,
		 "XCamHandler::window_input: unable to get window size");
  }
}

void XCamHandler::save_list()
{
  // Create the file selection dialog if none exists
  if (!save_list_file_dlog) {
    register_object_name();
    MrmType mrm_class;
    if ( !MrmFetchWidget(mrm_id, "cam_list_save_file_dlog", widget,
			 &save_list_file_dlog, &mrm_class) 
	 || !save_list_file_dlog ) {
      fprintf(stderr,
	      "XCamHandler::save: can't get UIL resource!\n");
      return;
    }
  }
  XtManageChild(save_list_file_dlog);
}

void XCamHandler::save_list_file_cb(Widget w, int *id, 
				  XmFileSelectionBoxCallbackStruct *call_data)
{
  get_object(w)->save_list_file( w, id, call_data );
}

void XCamHandler::save_cam_list_to_file( char *fname )
{
  // Save the transfer function
  FILE *ofile= fopen(fname, "w");
  if (!ofile) {
    fprintf(stderr,
  "XCamHandler::save_cam_list_to_file: unexpectedly cannot write file <%s>!\n",
	    fname);
    return;
  }

  dList_iter<Camera*> iter(cam_list);
  Camera **this_cam;
  while (this_cam= iter.next()) {
    fprintf(ofile,"camera set_from %f %f %f\n",
	    (*this_cam)->from().x(),(*this_cam)->from().y(),
	    (*this_cam)->from().z());
    fprintf(ofile,"camera set_at %f %f %f\n",
	    (*this_cam)->at().x(),(*this_cam)->at().y(),
	    (*this_cam)->at().z());
    fprintf(ofile,"camera set_up %f %f %f\n",
	    (*this_cam)->updir().x(),(*this_cam)->updir().y(),
	    (*this_cam)->updir().z());
    fprintf(ofile,"camera set_fovea %f\n", (*this_cam)->fov());
    fprintf(ofile,"camera set_hither_yon %f %f\n",
	    -((*this_cam))->hither_dist(),-((*this_cam)->yon_dist()));
    if ((*this_cam)->parallel_proj()) fprintf(ofile,"camera parallel\n");
    else fprintf(ofile,"camera perspective\n");
    fprintf(ofile,"camera add_cam_to_list\n");
  }

  if (fclose(ofile)==EOF) {
    fprintf(stderr,
      "XCamHandler::save_cam_list_to_file: unexpectedly cannot close <%s>!\n",
	    fname);
    return;
  }
}

static void save_tfun_anyway_cb( Widget w, caddr_t data_in, 
				  caddr_t call_data )
{
  xcamhandler_file_write_warning_data *data= 
    (xcamhandler_file_write_warning_data*)data_in;

  data->handler->save_cam_list_to_file( data->fname );

  XtUnmanageChild(data->to_be_closed);
  delete data;
}

static void cancel_save_tfun_cb( Widget w, 
				  caddr_t data_in, 
				  caddr_t call_data )
{
  xcamhandler_file_write_warning_data *data= 
    (xcamhandler_file_write_warning_data*)data_in;
  // Just pop down the warning dialog
  XtUnmanageChild(data->to_be_closed);
  delete data;
}

void XCamHandler::save_list_file( Widget w, int *id, 
				  XmFileSelectionBoxCallbackStruct *call_data )
{
  char *the_name, *label_name, *s;
  
  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  for (s = the_name; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  if (! *label_name) {
    pop_error_dialog("invalid_cam_list_file_msg");
    return;
  }
  
  if (access(the_name,F_OK)) { // file does not already exist
    save_cam_list_to_file( the_name );
    XtUnmanageChild(w);
  }
  else {
    xcamhandler_file_write_warning_data *data= 
      new xcamhandler_file_write_warning_data;
    data->fname= the_name;
    data->to_be_closed= w;
    data->handler= this;
    pop_warning_dialog( "save_anyway_msg", 
			save_tfun_anyway_cb, 
			cancel_save_tfun_cb,
			(void *)data );
  }
}

void XCamHandler::add_displayed_cam_to_list()
{
  cam_list.append( new Camera(cam_from_dlog()) );
  cams_in_list++;
  char string[16];
  if (list_count_widget) {
    sprintf(string,"%5d",cams_in_list);
    XmTextFieldSetString( list_count_widget, string );
  }
  // Regenerate iterator, pointing to last element
  delete cam_list_iter;
  cam_list_iter= new dList_iter<Camera*>(cam_list);
  cam_list_current= NULL;
  // Set button sensitivity
  if (cam_list_next_pb) XtSetSensitive( cam_list_next_pb, False );
  if (cam_list_prev_pb) {
    if (cams_in_list) XtSetSensitive( cam_list_prev_pb, True);
    else XtSetSensitive( cam_list_prev_pb, False );
  }
  if (cams_in_list && cam_list_save_pb) 
    XtSetSensitive( cam_list_save_pb, True);
}
