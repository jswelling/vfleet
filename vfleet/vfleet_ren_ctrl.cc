/****************************************************************************
 * vfleet_ren_ctrl.cc
 * Author Joel Welling
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

// This module provides renderer control callbacks

#include <stdlib.h>
#include <stdio.h>

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
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
#include <Mrm/MrmAppl.h>

#include "vfleet.h"

static Widget renderer_controls_id= NULL;
static Widget opac_length_id= NULL;
static Widget use_lighting_id= NULL;
static Widget fast_lighting_id= NULL;
static Widget fast_distances_id= NULL;
static Widget trilinear_interp_id= NULL;
static Widget specular_lighting_id= NULL;
static Widget threed_mipmap_id= NULL;
static Widget quality_settings_id = NULL;
static Widget opac_limit_id= NULL;
static Widget color_comp_error_id= NULL;
static Widget opac_min_id= NULL;
static Widget grad_comp_error_id= NULL;
static Widget grad_mag_error_id= NULL;

void vfleet_ren_ctrl_create(Widget w,
			    int *id,
			    unsigned long *reason) 
{
  switch (*id) {
    case k_quality_settings_id:
	quality_settings_id = w;
	break;
    case k_renderer_controls_id:
	renderer_controls_id= w;
	break;
    case k_opac_length_id:
	opac_length_id= w;
	break;
    case k_use_lighting_id:
	use_lighting_id= w;
	break;
    case k_fast_lighting_id:
	fast_lighting_id= w;
	break;
    case k_fast_distances_id:
	fast_distances_id= w;
	break;
    case k_threed_mipmap_id:
	threed_mipmap_id= w;
	break;
    case k_trilinear_interp_id:
	trilinear_interp_id= w;
	break;
    case k_specular_lighting_id:
	specular_lighting_id= w;
	break;
    case k_opac_limit_id:
	opac_limit_id= w;
	break;
    case k_color_comp_error_id:
	color_comp_error_id= w;
	break;
    case k_opac_min_id:
        opac_min_id= w;
        break;
  }
}

void set_quality_settings_cb( Widget w, XtPointer foo, XtPointer bar )
{
  Arg args[2];
  int max_value, value;
  float opac_limit, color_comp_error;
  int opac_min;

  XtSetArg (args[0], XmNmaximum, &max_value);
  XtSetArg (args[1], XmNvalue, &value);

  XtGetValues(opac_limit_id, args, 2);
  opac_limit= ((float)value)/((float)max_value);
  
  XtGetValues(color_comp_error_id, args, 2);
  color_comp_error= ((float)value)/((float)max_value);
  
  XtGetValues(opac_min_id, args, 2);
  opac_min= value;

  main_vren->setQualityMeasure( QualityMeasure(opac_limit, color_comp_error,
					       opac_min) );
}

static void reset_quality_settings_cb( Widget w, XtPointer foo, XtPointer bar )
{
  float opac_limit= main_vren->quality()->get_opacity_limit();
  float color_comp_error= main_vren->quality()->get_color_comp_error();
  int opac_min= main_vren->quality()->get_opacity_min();

  Arg args[1];
  int max_value, value;

  XtSetArg( args[0], XmNmaximum, &max_value );

  XtGetValues( opac_limit_id, args, 1 );
  XmScaleSetValue( opac_limit_id, (int)(opac_limit*max_value) );

  XtGetValues( color_comp_error_id, args, 1 );
  XmScaleSetValue( color_comp_error_id, (int)(color_comp_error*max_value) );

  XmScaleSetValue( opac_min_id, opac_min );
}

void set_renderer_controls_cb( Widget w, XtPointer foo, XtPointer bar )
{
  Arg args[2];
  int value;
  short decimals;
  XtSetArg (args[0], XmNdecimalPoints, &decimals);
  XtSetArg (args[1], XmNvalue, &value);
  XtGetValues(opac_length_id, args, 2);
  current_opac_scale= ((float)value/pow(10.0,decimals));
  if (main_svol) main_svol->set_size_scale( current_opac_scale );
  
  VRenOptions new_opts= 0;
  if ( XmToggleButtonGadgetGetState(use_lighting_id) ) 
    new_opts |= OPT_USE_LIGHTING;
  if ( XmToggleButtonGadgetGetState(fast_lighting_id) ) 
    new_opts |= OPT_FAST_LIGHTING;
#ifdef never
  if ( XmToggleButtonGadgetGetState(fast_distances_id) ) 
    new_opts |= OPT_FAST_DISTANCES;
#endif
  if ( XmToggleButtonGadgetGetState(trilinear_interp_id) ) 
    new_opts |= OPT_TRILINEAR_INTERP;
  if ( XmToggleButtonGadgetGetState(specular_lighting_id) )
    new_opts |= OPT_SPECULAR_LIGHTING;
  if ( XmToggleButtonGadgetGetState(threed_mipmap_id) )
    new_opts |= OPT_THREED_MIPMAP;
  main_vren->setOptionFlags( new_opts );
}

void vfleet_update_ren_ctrl_widgets()
{
  if (main_svol) {
    float old_size_scale= main_svol->get_size_scale();
    Arg args[1];
    short decimals;
    XtSetArg (args[0], XmNdecimalPoints, &decimals);
    XtGetValues(opac_length_id, args, 1);
    XmScaleSetValue( opac_length_id, 
		   (int)(old_size_scale*pow(10.0,decimals)) );
  }

  VRenOptions opts= main_vren->getOptionFlags();
  XmToggleButtonGadgetSetState( use_lighting_id, 
				opts & OPT_USE_LIGHTING, False );
  XmToggleButtonGadgetSetState( fast_lighting_id, 
				opts & OPT_FAST_LIGHTING, False );
#ifdef never
  XmToggleButtonGadgetSetState( fast_distances_id, 
				opts & OPT_FAST_DISTANCES, False );
#endif
  XmToggleButtonGadgetSetState( trilinear_interp_id, 
				opts & OPT_TRILINEAR_INTERP, False );
  XmToggleButtonGadgetSetState( threed_mipmap_id, 
				opts & OPT_THREED_MIPMAP, False );
  XmToggleButtonGadgetSetState( specular_lighting_id,
			       opts & OPT_SPECULAR_LIGHTING, False );
}

static void reset_renderer_controls_cb( Widget w, XtPointer foo, 
					XtPointer bar )
{
  vfleet_update_ren_ctrl_widgets();
}

void vfleet_ren_ctrl_open( Widget w, int *tag, caddr_t cb )
{
  switch (*tag) {
  case k_quality_settings_id:
    XtManageChild( quality_settings_id );
    break;
  case k_renderer_controls_id:
    XtManageChild( renderer_controls_id );
    break;
  }
}

static MRMRegisterArg mrm_names[] = {
  {"set_quality_settings_cb", (caddr_t)set_quality_settings_cb},
  {"reset_quality_settings_cb", (caddr_t)reset_quality_settings_cb},
  {"set_renderer_controls_cb", (caddr_t)set_renderer_controls_cb},
  {"reset_renderer_controls_cb", (caddr_t)reset_renderer_controls_cb}
};

void vfleet_ren_ctrl_reg()
// Register appropriate callback names
{
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));

}
