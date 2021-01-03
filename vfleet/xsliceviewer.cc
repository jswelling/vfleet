/****************************************************************************
 * xsliceviewer.cc
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <string.h>
#ifdef ATTCC
#include <osfcn.h>
#define F_OK 0
#else
#include <unistd.h>
#endif

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Scale.h>
#include <Xm/ToggleBG.h>
#include <Mrm/MrmAppl.h>

#include "vfleet.h"

int XSliceViewer::initialized= 0;
int XSliceViewer::min_image_form_width= 0;

XSliceViewer::XSliceViewer( Widget parent_widget_in, MrmHierarchy mrm_id_in, 
			    const GridInfo& grid_in,
			    baseTransferFunction *tfun_in, 
			    const int ndata_in, DataVolume** dtbl_in,
			    const int which_dir_in,
			    void (*deletion_cb_in)(XSliceViewer* delete_me),
			    const int manage_dvols_in )
: baseSliceViewer( grid_in, tfun_in, ndata_in, dtbl_in, 
		   which_dir_in, manage_dvols_in )
{
  mrm_id= mrm_id_in;
  parent_widget= parent_widget_in;
  deletion_cb= deletion_cb_in;

  if (!initialized) {
    MRMRegisterArg mrm_names[5];
    mrm_names[0].name= "xsliceviewer_delete_cb";
    mrm_names[0].value= (caddr_t)XSliceViewer::delete_cb;
    mrm_names[1].name= "xsliceviewer_create_cb";
    mrm_names[1].value= (caddr_t)XSliceViewer::create_cb;
    mrm_names[2].name= "xsliceviewer_expose_cb";
    mrm_names[2].value= (caddr_t)XSliceViewer::expose_cb;
    mrm_names[3].name= "xsliceviewer_slider_update_cb";
    mrm_names[3].value= (caddr_t)XSliceViewer::slider_update_cb;
    mrm_names[4].name= "xsliceviewer_update_image_cb";
    mrm_names[4].value= (caddr_t)XSliceViewer::update_image_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    XtPointer widthp;
    MrmCode type;
    MrmFetchLiteral(mrm_id, "xsliceviewer_min_img_width", 
		    XtDisplay(parent_widget), &widthp, &type);
    min_image_form_width= *((int *)widthp);
    initialized= 1;
  }

  widget= NULL;
  image_widget= NULL;
  alpha_button_widget= NULL;
  ihandler= NULL;
  image= NULL;

  register_object_name();
  MrmType mrm_class;

  MrmRegisterArg names[4];
  names[0].name= "xsliceviewer_image_xsize";
  names[0].value= (caddr_t)image_xsize;
  names[1].name= "xsliceviewer_image_ysize";
  names[1].value= (caddr_t)image_ysize;
  names[2].name= "xsliceviewer_nslices";
  names[2].value= (caddr_t)nslices;
  names[3].name= "xsliceviewer_current_slice";
  names[3].value= (caddr_t)current_plane;
  MrmRegisterNamesInHierarchy(mrm_id, names, 4);

  switch (which_dir) {
  case 0: // x constant
    {
      if (!MrmFetchWidget(mrm_id, "sliceviewer_x_dlog", app_shell,
			  &widget, &mrm_class) || !widget) {
	fprintf(stderr,
		"XSliceViewer::XSliceViewer: can't get UIL resource!\n");
	return;
      }
    }
    break;
  case 1: // y constant
    {
      if (!MrmFetchWidget(mrm_id, "sliceviewer_y_dlog", app_shell,
			  &widget, &mrm_class) || !widget) {
	fprintf(stderr,
		"XSliceViewer::XSliceViewer: can't get UIL resource!\n");
	return;
      }
    }
    break;
  case 2: // z constant
    {
      if (!MrmFetchWidget(mrm_id, "sliceviewer_z_dlog", app_shell,
			  &widget, &mrm_class) || !widget) {
	fprintf(stderr,
		"XSliceViewer::XSliceViewer: can't get UIL resource!\n");
	return;
      }
    }
    break;
  }
  XtManageChild(widget);
}

XSliceViewer::~XSliceViewer()
{
  if (widget) XtDestroyWidget(widget);
  delete ihandler;
  delete image;
}

void XSliceViewer::register_object_name()
{
  // This sticks a pointer to this XTfunHandler into the mrm hierarchy
  MrmRegisterArg names;
  names.name= "owner_object";
  names.value= (caddr_t)this;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
}

XSliceViewer* XSliceViewer::get_object( Widget w )
{
  // This recovers the object pointer registered with register_object_name
  // from the widget
  Arg arg;
  void *object;
  XtSetArg( arg, XmNuserData, &object );
  XtGetValues( w, &arg, 1 );
  if (!object) 
    fprintf(stderr,
	    "XSliceViewer::get_object: couldn't get obj pointer!\n");
  return (XSliceViewer*)object;
}

void XSliceViewer::delete_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->issue_delete_request( w, id, reason );
}

void XSliceViewer::issue_delete_request( Widget w, int *id, 
					 unsigned long *reason )
{
  (*deletion_cb)( this );
}

void XSliceViewer::create_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->create( w, id, reason );
}

void XSliceViewer::create( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 0:
    image_widget= w;
    center_image();
    break;
  case 1:
    alpha_button_widget= w;
    break;
  }
}

void XSliceViewer::expose_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->expose( w, id, reason );
}

void XSliceViewer::expose( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 0:
    {
      if (!ihandler) { // just created
	ihandler= AttachImageHandler(w);
	image= new rgbImage( image_xsize, image_ysize );
	update_image();
      }
      else ihandler->redraw();      
    }
    break;
  }
}

void XSliceViewer::slider_update_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->slider_update( w, id, reason );
}

void XSliceViewer::slider_update( Widget w, int *id, unsigned long *reason )
{
  int new_plane;
  XmScaleGetValue(w, &new_plane);
  new_plane--; // since UI counts from 0 and we count from 1
  set_plane( new_plane );
  update_image();
}

void XSliceViewer::update_image_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->update_image();
}

void XSliceViewer::update_image()
{
  prepare_plane( current_plane );

  if (XmToggleButtonGadgetGetState(alpha_button_widget)) {
    // Premultiply by alpha
    switch (which_dir) {
    case 0:
      {
	for (int i=0; i<image->xsize(); i++)
	  for (int j=0; j<image->ysize(); j++) {
	    gBColor clr= calc( current_plane, i, j );
	    image->setpix( i, j, clr.alpha_weighted() );
	  }
      }
      break;
    case 1:
      {
	for (int i=0; i<image->xsize(); i++)
	  for (int j=0; j<image->ysize(); j++) {
	    gBColor clr= calc( j, current_plane, i );
	    image->setpix( i, j, clr.alpha_weighted() );
	  }
      }
      break;
    case 2:
      {
	for (int i=0; i<image->xsize(); i++)
	  for (int j=0; j<image->ysize(); j++) {
	    gBColor clr= calc( i, j, current_plane );
	    image->setpix( i, j, clr.alpha_weighted() );
	  }
      }
      break;
    }
  }
  else {
    // Do not premultiply by alpha
    switch (which_dir) {
    case 0:
      {
	for (int i=0; i<image->xsize(); i++)
	  for (int j=0; j<image->ysize(); j++) {
	    gBColor clr= calc( current_plane, i, j );
	    image->setpix( i, j, clr );
	  }
      }
      break;
    case 1:
      {
	for (int i=0; i<image->xsize(); i++)
	  for (int j=0; j<image->ysize(); j++) {
	    gBColor clr= calc( j, current_plane, i );
	    image->setpix( i, j, clr );
	  }
      }
      break;
    case 2:
      {
	for (int i=0; i<image->xsize(); i++)
	  for (int j=0; j<image->ysize(); j++) {
	    gBColor clr= calc( i, j, current_plane );
	    image->setpix( i, j, clr );
	  }
      }
      break;
    }
  }

  ihandler->display(image);
}

void XSliceViewer::center_image()
{
  // If the image is narrower than the window, we want to
  // shift it right to a central position.
  if (min_image_form_width > image_xsize) {
    int shift= (min_image_form_width - image_xsize)/2;
    Arg setting_arg;
    XtSetArg( setting_arg, XmNx, shift );
    XtSetValues( image_widget, &setting_arg, 1 );
  }
}
