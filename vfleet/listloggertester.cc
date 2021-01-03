/****************************************************************************
 * listloggertester.cc
 * Authors Robert Earhart, Joel Welling
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

/* This module tests the Motif list logger. */

#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include "logger.h"
#include "xlogger.h"

static char *exename;
static baseLogger *most_recent_logger;
static int current_msg_id= 0;
static Widget toplevel;

static void exit_cb (Widget w,
		     XtPointer data,
		     XtPointer cb) {
  exit(0);
}

static void add_cb (Widget w,
		    XtPointer data,
		    XtPointer cb) {
  most_recent_logger= new mlistLogger( exename, toplevel );
}

static void message_cb (Widget w,
			XtPointer data,
			XtPointer cb) {
  char msg_buf[40];
  sprintf(msg_buf,"Trivial message %d",current_msg_id++);
  if (most_recent_logger) most_recent_logger->comment(msg_buf);
}

static void delete_cb (Widget w,
		     XtPointer data,
		     XtPointer cb) {
  delete most_recent_logger;
  most_recent_logger= 0;
}

main( int argc, char *argv[] )
{
  const int MAX_ARGS= 20;

  Widget main_window;
  Widget menu_bar;
  Widget menu_pane;
  Widget button;
  Arg    args[MAX_ARGS];
  int n;
  
  fprintf(stderr,"Starting...\n");
  
  exename= argv[0];
  
  toplevel = XtInitialize(argv[0], "MP3d", NULL, 0, &argc, argv);
  n= 0;
  main_window = XmCreateMainWindow(toplevel, "main", args, n);
  XtManageChild(main_window);
  n= 0;
  menu_bar = XmCreateMenuBar(main_window, "menu_bar", args, n);
  XtManageChild(menu_bar);
  n = 0;
  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, n);
  
  /* Create File Menu */
  XmString title_string= XmStringCreate("File", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'F'); n++;
  button = XmCreateCascadeButton(menu_bar, "file", args, n);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);
  
  /* Create Add button */
  title_string= XmStringCreate("Add", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'A'); n++;
  button = XmCreatePushButton(menu_pane, "add", args, n);
  XtAddCallback(button, XmNactivateCallback, add_cb, NULL);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);
  
  /* Create Message button */
  title_string= XmStringCreate("Message", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'M'); n++;
  button = XmCreatePushButton(menu_pane, "message", args, n);
  XtAddCallback(button, XmNactivateCallback, message_cb, NULL);
  if (title_string) XmStringFree(title_string);
  XtManageChild(button);
  
  /* Create Delete button */
  title_string= XmStringCreate("Delete", XmSTRING_DEFAULT_CHARSET);
  n = 0;
  XtSetArg(args[n], XmNlabelString, title_string); n++;
  XtSetArg(args[n], XmNmnemonic, 'D'); n++;
  button = XmCreatePushButton(menu_pane, "delete", args, n);
  XtAddCallback(button, XmNactivateCallback, delete_cb, NULL);
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
  
  XtRealizeWidget(toplevel);
  XtMainLoop();
  
  fprintf(stderr,"done.\n");
}
