/****************************************************************************
 * xlogserver.cc
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

/* This module provides a server for the Motif list logger. */

#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include "logger.h"
#include "xlogger.h"
#include "basenet.h"
#include "netlogger.h"
#include "servman.h"

// This is the time between calls to the baseNet::service routine.
const int service_wait_time= 100;

static char *exename;
static Widget toplevel;

static void exit_cb (Widget w,
		     XtPointer data,
		     XtPointer cb) {
  netLogger::shutdown();
  exit(0);
}

static baseLogger *outlogger_creation_cb()
{
  return( new mlistLogger( exename, toplevel ) );
}

static void service_callback(Widget w)
{
  baseNet::service();
  XtAddTimeOut( service_wait_time, 
		(XtTimerCallbackProc)service_callback, NULL);
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
  
  exename= argv[0];

  toplevel = XtInitialize(argv[0], "Xlogserver", NULL, 0, &argc, argv);

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

  netLogger::initialize( LogServerExeName );
  logServer::initialize( outlogger_creation_cb, argv[0] );
  
  XtAddTimeOut( service_wait_time, 
		(XtTimerCallbackProc)service_callback, NULL );
  
  XtMainLoop();
  
}
