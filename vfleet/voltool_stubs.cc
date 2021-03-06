//
// voltool_stubs.cc - Notify and event callback function stubs.
// This file was generated by `gxv++' from `voltool.G'.
//

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include "voltool_ui.h"
#include <gfm.h>
#include "basenet.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "voltool.h"

#ifdef MAIN

//
// Global object definitions.
//
voltool_mainwindow_objects	Voltool_mainwindow;
voltool_ImageWindow_objects	Voltool_ImageWindow;

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
	Voltool_mainwindow.objects_initialize(NULL);
	Voltool_ImageWindow.objects_initialize(Voltool_mainwindow.mainwindow);
	
	
	//
	// Turn control over to XView.
	//
	xv_main_loop(Voltool_mainwindow.mainwindow);
	exit(0);
}

#endif


//
// Menu handler for `FileMenu (Load)'.
//
Menu_item
load_callback(Menu_item item, Menu_generate op)
{
	voltool_mainwindow_objects * ip = (voltool_mainwindow_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		gfm_activate(file_dialog, NULL, "^.*\*.rle$", NULL,
			     file_select_callback, NULL, GFM_LOAD);
		
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
	voltool_mainwindow_objects * ip = (voltool_mainwindow_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		
		// gxv_start_connections DO NOT EDIT THIS SECTION

		// gxv_end_connections

		voltool_quit();

		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

//
// Repaint callback function for `canvas1'.
//
void
imagecanvas_repaint_callback(Canvas canvas, Xv_window paint_window, Rectlist *rects)
{
	XImageHandler *ihandler= 
	  (XImageHandler *)xv_get(canvas, XV_KEY_DATA, WIN_IHANDLER_KEY);

	ihandler->redraw();

	// gxv_start_connections DO NOT EDIT THIS SECTION

	// gxv_end_connections

}

//
// Resize callback function for `canvas1'.
//
void
imagecanvas_resize_callback(Canvas canvas, int width, int height)
{
	XImageHandler *ihandler= 
	  (XImageHandler *)xv_get(canvas, XV_KEY_DATA, WIN_IHANDLER_KEY);

	fprintf(stderr,"imagecanvas_resize_callback called\n");
	ihandler->resize();
	
	// gxv_start_connections DO NOT EDIT THIS SECTION

	// gxv_end_connections

}

//
// Menu handler for `RenderMenu (Go)'.
//
Menu_item
vren_go_callback(Menu_item item, Menu_generate op)
{
	voltool_mainwindow_objects * ip = (voltool_mainwindow_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		fputs("voltool: vren_go_callback: MENU_NOTIFY\n", stderr);
		
		voltool_render_start( ip );

		// gxv_start_connections DO NOT EDIT THIS SECTION

		// gxv_end_connections

		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

//
// Repaint callback function for `rendercanvas'.
//
void
rendercanvas_repaint_callback(Canvas canvas, Xv_window paint_window, Rectlist *rects)
{
	XImageHandler *ihandler= 
	  (XImageHandler *)xv_get(canvas, XV_KEY_DATA, WIN_IHANDLER_KEY);

	fputs("voltool: rendercanvas_repaint_callback\n", stderr);
	
	ihandler->redraw();

	// gxv_start_connections DO NOT EDIT THIS SECTION

	// gxv_end_connections

}

//
// Resize callback function for `rendercanvas'.
//
void
rendercanvas_resize_callback(Canvas canvas, int width, int height)
{
	XImageHandler *ihandler= 
	  (XImageHandler *)xv_get(canvas, XV_KEY_DATA, WIN_IHANDLER_KEY);

	fputs("voltool: rendercanvas_resize_callback\n", stderr);
	
	ihandler->resize();
	// gxv_start_connections DO NOT EDIT THIS SECTION

	// gxv_end_connections

}
