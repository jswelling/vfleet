/****************************************************************************
 * imview.cc
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
#include <sdsc.h>
#include "basenet.h"
#include "imview_ui.h"
#include "image.h"
#include "imagehandler.h"
#include "ximagehandler.h"

//
// Global object definitions.
//
imview_mainwindow_objects	Imview_mainwindow;
gfm_popup_objects *file_dialog;

//
// Instance XV_KEY_DATA key.  An instance is a set of related
// user interface objects.  A pointer to an object's instance
// is stored under this key in every object.  This must be a
// global variable.
//
Attr_attribute	INSTANCE;

main(int argc, char **argv)
{
  //
  // Initialize XView.
  //
  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
  INSTANCE = xv_unique_key();
  
  //
  // Initialize user interface components.
  // Do NOT edit the object initializations by hand.
  //
  Imview_mainwindow.objects_initialize((Xv_opaque)NULL);
  file_dialog= gfm_initialize(NULL, Imview_mainwindow.mainwindow,
			      "Load Image File");
    
  //
  // Turn control over to XView.
  //
    xv_main_loop(Imview_mainwindow.mainwindow);
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

  if ((fp = fopen(fullname, "r")) == NULL) {
    popup_notice(ip->popup,"Input data file not found!");
    delete fullname;
    return( GFM_ERROR );
  }

  image= new rgbImage( fp, fullname );
  if (!image->valid()) {
    popup_notice(ip->popup,"Unable to read file; it may be corrupted!");
    delete fullname;
    return( GFM_ERROR );
  }

  // Create and pop up the image window
  XvautoImageHandler *ihandler= new XvautoImageHandler( image, file );

  // Clean up
  delete fullname;
  
  return(GFM_OK);
}

//
// Menu handler for `FileMenu (Load)'.
//
Menu_item
load_callback(Menu_item item, Menu_generate op)
{
	imview_mainwindow_objects * ip = (imview_mainwindow_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		gfm_activate(file_dialog, NULL, "^.*\*$", NULL,
			     file_select_callback, (Xv_opaque)NULL, GFM_LOAD);
		
		// gxv_start_connections DO NOT EDIT THIS SECTION

		// gxv_end_connections

		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

//
// Menu handler for `FileMenu (Quit)'.
//
Menu_item
quit_callback(Menu_item item, Menu_generate op)
{
	imview_mainwindow_objects * ip = (imview_mainwindow_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		
		// gxv_start_connections DO NOT EDIT THIS SECTION

		// gxv_end_connections

		exit(0);

		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

