/****************************************************************************
 * xlogger.cc
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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include "logger.h"

#ifdef MOTIF
#include <Xm/Xm.h>
#include <X11/Intrinsic.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>

#include "xlogger.h"

void motifLogger::write_string(char *string) {
    Arg args[1];
    XmString xmstring;
    XtSetArg(args[0], res_string, &xmstring);
    XtGetValues((Widget)widget, args, 1);
    XmStringFree(xmstring);
    xmstring = XmStringCreateSimple(string);
    XtSetArg(args[0], res_string, xmstring);
    XtSetValues((Widget)widget, args, 1);
    XmStringFree(xmstring);
    XmUpdateDisplay((Widget)widget);
}

Widget mlistLogger::list_widget= NULL;
int mlistLogger::list_id= 1;

mlistLogger::mlistLogger( const char *cmd, Widget parent_widget ) 
: baseLogger( cmd )
{
  if (!list_widget) {
    Arg args[10];
    int n= 0;

    // Starting up;  create dialog and logger
    Widget dialog_shell= XmCreateDialogShell( parent_widget, "ListDialogSH",
					      NULL, 0 );
    n= 0;
    XtSetArg(args[n], XtNheight, 300); n++;
    XtSetArg(args[n], XtNwidth, 300); n++;
    list_widget= XmCreateScrolledList( dialog_shell, "LoggerList",
				       args, n );
    XtManageChild( list_widget );
  }

  int cmd_length= 3 + 2
    + strlen(cmd) + 1
    + strlen(user) + 2
    + strlen( procname ) + 4
    + 1;
  char *tmp_buf= new char[cmd_length];
#ifdef never
  sprintf(tmp_buf,"%3d: %s <%s><%s>: ", list_id++, cmd, user, procname );
#endif
  sprintf(tmp_buf,"%3d: ", list_id++ );
  cmd_string= XmStringCreateSimple( tmp_buf );
  delete [] tmp_buf;

  current_string= XmStringCopy( cmd_string );
  XmListAddItemUnselected( list_widget, 
			   current_string, 0 ); // 0 puts it at end
  open_logger();
}

mlistLogger::~mlistLogger()
{
  close_logger();
  XmListDeleteItem( list_widget, current_string );
  XmStringFree( current_string );
  XmStringFree( cmd_string );
}

void mlistLogger::write_string( char *string )
{
  XmString new_string= XmStringConcat( cmd_string,
				       XmStringCreateSimple( string ) );
  XmListReplaceItems( list_widget, &current_string, 1, &new_string );
  XmStringFree( current_string );
  current_string= new_string;
}

#endif // MOTIF
