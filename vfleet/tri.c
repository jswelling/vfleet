/****************************************************************************
 * tri.c
 * Author Joe Demers
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>

#include "xdrawih.h"

static Widget toplevel;
static XmautodrawImageHandler *ihandler1;
static XdrawImageHandler *ihandler2;

static void show_cb (Widget w,
		     XtPointer data,
		     XtPointer cb) 
{
  long i;
  XPoint a[] = { 50, 220,
		   140, 40,
		   220, 200,
		   200, 200,
		   140, 80,
		   60, 240};
  XPoint b[] = { 240, 240,
		   60, 240,
		   140, 80,
		   150, 100,
		   90, 220,
		   250, 220};
  XPoint c[] = { 160, 40,
		   250, 220,
		   90, 220,
		   100, 200,
		   220, 200,
		   140, 40};
  
  XPoint d[] = { 50, 220,
		 140, 40,
		 220, 200,
		 200, 200,
		 140, 80,
		 60, 240,
		 50, 220};
  XPoint e[] = { 240, 240,
		 60, 240,
		 140, 80,
		 150, 100,
		 90, 220,
		 250, 220,
		 240, 240};
  XPoint f[] = { 160, 40,
		 250, 220,
		 90, 220,
		 100, 200,
		 220, 200,
		 140, 40,
		 160, 40};

  xdrawih_clear(ihandler2, 0, 0, 0);
  xmadih_clear(ihandler1, 0, 0, 0);

  fprintf(stderr, "blue polygons:\n");
  xdrawih_pgon(ihandler2, 0, 0, 255, 6, a);
  xmadih_pgon(ihandler1, 0, 0, 255, 6, a);
  fprintf(stderr, "purple polygons:\n");
  xdrawih_pgon(ihandler2, 127, 0, 127, 6, b);
  xmadih_pgon(ihandler1, 127, 0, 127, 6, b);
  fprintf(stderr, "green polygons:\n");
  xdrawih_pgon(ihandler2, 0, 255, 0, 6, c);
  xmadih_pgon(ihandler1, 0, 255, 0, 6, c);

  xdrawih_redraw(ihandler2);
  xmadih_redraw(ihandler1);

  sleep(5);

  xdrawih_clear(ihandler2, 0, 0, 0);
  xmadih_clear(ihandler1, 0, 0, 0);

  fprintf(stderr, "red lines:\n");
  xdrawih_pline(ihandler2, 255, 0, 0, 7, d);
  xmadih_pline(ihandler1, 255, 0, 0, 7, d);
  fprintf(stderr, "red lines:\n");
  xdrawih_pline(ihandler2, 255, 0, 0, 7, e);
  xmadih_pline(ihandler1, 255, 0, 0, 7, e);
  fprintf(stderr, "red lines:\n");
  xdrawih_pline(ihandler2, 255, 0, 0, 7, f);
  xmadih_pline(ihandler1, 255, 0, 0, 7, f);

  xdrawih_redraw(ihandler2);
  xmadih_redraw(ihandler1);

  sleep(5);

  xdrawih_clear(ihandler2, 0, 0, 0);
  xmadih_clear(ihandler1, 0, 0, 0);

  fprintf(stderr, "white points:\n");
  xdrawih_ppoint(ihandler2, 255, 255, 255, 6, a);
  xmadih_ppoint(ihandler1, 255, 255, 255, 6, a);
  fprintf(stderr, "white points:\n");
  xdrawih_ppoint(ihandler2, 255, 255, 255, 6, b);
  xmadih_ppoint(ihandler1, 255, 255, 255, 6, b);
  fprintf(stderr, "white points:\n");
  xdrawih_ppoint(ihandler2, 255, 255, 255, 6, c);
  xmadih_ppoint(ihandler1, 255, 255, 255, 6, c);

  xdrawih_redraw(ihandler2);
  xmadih_redraw(ihandler1);

  fprintf(stderr, "All in this box\n");
  fprintf(stderr, "of me Lucky Charms!\n");
}

static void redraw_cb (Widget w, XtPointer data, XtPointer cb) 
{
  xdrawih_redraw(ihandler2);
}

static void exit_cb (Widget w,		     
		     XtPointer data,
		     XtPointer cb) {
  xdrawih_delete(ihandler2);
  xmadih_delete(ihandler1);
  exit(0);
}

main( int argc, char *argv[] )
{
  const int MAX_ARGS= 20;

  XmString title_string;
  Atom atomcolormapwindows;
  Window canvas_win;
  Widget main_window;
  Widget menu_bar;
  Widget menu_pane;
  Widget button;
  Widget drawing_form;
  Widget drawing_area;
  Arg    args[MAX_ARGS];
  int n;
  
  toplevel = XtInitialize(argv[0], "escher", NULL, 0, &argc, argv);
  n= 0;
  main_window = XmCreateMainWindow(toplevel, "main", args, n);
  XtManageChild(main_window);
  n= 0;
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
  
  /* Create Show button */
  title_string= XmStringCreate("Show", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'S'); n++;
  button = XmCreatePushButton(menu_pane, "show", args, n);
  XtAddCallback(button, XmNactivateCallback, show_cb, NULL);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);
  
  /* Create Exit button */
  title_string= XmStringCreate("Exit", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'E'); n++;
  button = XmCreatePushButton(menu_pane, "exit", args, n);
  XtAddCallback(button, XmNactivateCallback, exit_cb, NULL);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);

  /* Create drawing form */
  n= 0;
  drawing_form= XmCreateForm(main_window,"drawing_form",args,n);

  /* Create drawing area */
  n= 0;
  XtSetArg(args[n], XmNheight, 300); n++;
  XtSetArg(args[n], XmNwidth, 300); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  drawing_area= XmCreateDrawingArea(drawing_form,"drawing_area",args,n);
  XtAddCallback(drawing_area,XmNexposeCallback, redraw_cb,NULL);
  XtManageChild(drawing_area);
  XtManageChild(drawing_form);
  
  XtRealizeWidget(toplevel);

  atomcolormapwindows= 
    XInternAtom(XtDisplay(toplevel), "WM_COLORMAP_WINDOWS", False);
  
  canvas_win = XtWindow(drawing_area);
  XChangeProperty( XtDisplay(toplevel), XtWindow(toplevel), 
		   atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		   (unsigned char *)&canvas_win, 1 );

  fprintf(stderr, "Create ihandler1 (Xmautodraw...)\n");
  ihandler1 = xmadih_create(toplevel, 300, 300);
  fprintf(stderr, "ihandler1 created\n");
  fprintf(stderr, "Create ihandler2 (Xdraw...)\n");
  ihandler2 = xdrawih_create(XtDisplay(drawing_area), 0,
			    XtWindow(drawing_area), 300, 300);
  fprintf(stderr, "ihandler2 created\n");

  XtMainLoop();
}
