/****************************************************************************
 * xphotocopy.cc
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

#include <Xm/Xm.h>
#include <Mrm/MrmAppl.h>

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "xphotocopy.h"
#include "canvases.h"

int XPhotocopy::initialized= 0;

XPhotocopy::XPhotocopy( Widget parent_widget_in, MrmHierarchy mrm_id_in,
			rgbImage *image_in,
			void (*deletion_cb_in)(XPhotocopy*) )
{
  mrm_id= mrm_id_in;
  parent_widget= parent_widget_in;
  deletion_cb= deletion_cb_in;
  ihandler= NULL; // created in expose
  image_widget= NULL; // set in create 
  image= image_in; // reset to NULL when construction complete

  if (!initialized) {
    MRMRegisterArg mrm_names[3];
    mrm_names[0].name= "xphotocopy_delete_cb";
    mrm_names[0].value= (caddr_t)XPhotocopy::delete_cb;
    mrm_names[1].name= "xphotocopy_create_cb";
    mrm_names[1].value= (caddr_t)XPhotocopy::create_cb;
    mrm_names[2].name= "xphotocopy_expose_cb";
    mrm_names[2].value= (caddr_t)XPhotocopy::expose_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    initialized= 1;
  }

  register_object_name();
#ifdef never
  MrmRegisterArg names[2];
  names[0].name= "photocopy_image_xsize";
  names[0].value= (caddr_t)(image->xsize());
  names[1].name= "photocopy_image_ysize";
  names[1].value= (caddr_t)(image->ysize());
  MrmRegisterNamesInHierarchy(mrm_id, names, 4);
#endif
  MrmType mrm_class;
  if (!MrmFetchWidget(mrm_id, "photocopy_dlog", parent_widget,
		      &widget, &mrm_class) || !widget) {
    fprintf(stderr,
	    "XPhotocopy::XPhotocopy: can't get UIL resource!\n");
    return;
  }
  XtManageChild(widget);
}

XPhotocopy::~XPhotocopy()
{
  delete ihandler;
  if (widget) XtDestroyWidget(widget);
}

void XPhotocopy::register_object_name()
{
  // This sticks a pointer to this XTfunHandler into the mrm hierarchy
  MrmRegisterArg names;
  names.name= "owner_object";
  names.value= (caddr_t)this;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
}

XPhotocopy* XPhotocopy::get_object( Widget w )
{
  // This recovers the object pointer registered with register_object_name
  // from the widget
  Arg arg;
  void *object;
  XtSetArg( arg, XmNuserData, &object );
  XtGetValues( w, &arg, 1 );
  if (!object) 
    fprintf(stderr,
	    "XPhotocopy::get_object: couldn't get obj pointer!\n");
  return (XPhotocopy*)object;
}

void XPhotocopy::delete_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->issue_delete_request( w, id, reason );
}

void XPhotocopy::create_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->create( w, id, reason );
}

void XPhotocopy::create( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 0: 
    {
      image_widget= w;
      Arg args[20];
      int n= 0;
      XtSetArg(args[n], XmNwidth, image->xsize()); n++;
      XtSetArg(args[n], XmNheight, image->ysize()); n++;
      XtSetValues(image_widget, args, n);
    }
    break;
  }
}

void XPhotocopy::expose_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->expose( w, id, reason );
}

void XPhotocopy::expose( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 0:
    {
      if (!ihandler) { // just created
	ihandler= AttachImageHandler(w);
	ihandler->display(image);
	// There is a danger that the main app will deallocate the image,
	// so we do not want to keep the possibly invalid pointer around.
	image= NULL;
      }
      else {
	ihandler->redraw();
      }
    }
    break;
  }
}

