/****************************************************************************
 * vfleet_error.cc
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

// Dialog makers
void pop_error_dialog( char *text )
{
  Arg args[MAX_ARGS];
  int n;
  Widget error_widget= NULL;
  MrmType widget_class;
  MrmCode lit_type;
  XmString text_string= NULL;
  
  if (text == NULL) return;

  // Try to use the string as a name to fetch from the UID
  void* lit_value;
  if (MrmFetchLiteral(mrm_id, text, XtDisplay(app_shell),
		      &lit_value, &lit_type) == MrmSUCCESS) {
    // Put this literal in the dialog
    MRMRegisterArg names;
    names.name= "error_text_id";
    names.value= lit_value;
    MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  }
  else {
    // Use the string directly
    XmString text_string= XmStringCreate(text, XmSTRING_DEFAULT_CHARSET);
    MRMRegisterArg names;
    names.name= "error_text_id";
    names.value= text_string;
    MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  }
    

  MrmFetchWidget(mrm_id, "error_dialog", app_shell, 
		 &error_widget, &widget_class);
  if (error_widget) {
    Widget button= XmMessageBoxGetChild(error_widget, XmDIALOG_CANCEL_BUTTON);
    if (button) XtUnmanageChild(button);
    button= XmMessageBoxGetChild(error_widget, XmDIALOG_HELP_BUTTON);
    if (button) XtUnmanageChild(button);
    XtManageChild(error_widget);
  }
  else 
  fprintf(stderr,
       "pop_error_dialog: error dialog not found; is uid file out of date?\n");

  if (text_string) XmStringFree(text_string);

}

void pop_warning_dialog( char *text, 
			 void (*ok_cb)(Widget, caddr_t, caddr_t),
			 void (*cancel_cb)(Widget, caddr_t, caddr_t),
			 void *client_data )
{
  Arg args[MAX_ARGS];
  Widget warning_widget= NULL;
  MrmType widget_class;
  MrmCode lit_type;
  XmString text_string= NULL;
  
  if (text == NULL) return;

  // Try to use the string as a name to fetch from the UID
  void* lit_value;
  if (MrmFetchLiteral(mrm_id, text, XtDisplay(app_shell),
		      &lit_value, &lit_type) == MrmSUCCESS) {
    // Put this literal in the dialog
    MRMRegisterArg names;
    names.name= "warning_text_id";
    names.value= lit_value;
    MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  }
  else {
    // Use the string directly
    text_string= XmStringCreate(text, XmSTRING_DEFAULT_CHARSET);
    MRMRegisterArg names;
    names.name= "warning_text_id";
    names.value= text_string;
    MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  }

  MrmFetchWidget(mrm_id, "warning_dialog", app_shell, 
		 &warning_widget, &widget_class);
  if (warning_widget) {
    Widget button= XmMessageBoxGetChild(warning_widget, 
					XmDIALOG_HELP_BUTTON);
    if (button) XtUnmanageChild(button);
    button= XmMessageBoxGetChild(warning_widget,
				 XmDIALOG_OK_BUTTON);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)ok_cb, 
		  client_data);
    button= XmMessageBoxGetChild(warning_widget,
				 XmDIALOG_CANCEL_BUTTON);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)cancel_cb, 
		  client_data);
    XtManageChild(warning_widget);
  }
  else 
  fprintf(stderr,
   "pop_warning_dialog: warning dialog not found; is uid file out of date?\n");

  if (text_string) XmStringFree(text_string);

}

void pop_info_dialog( char *text )
{
  Arg args[MAX_ARGS];
  int n;
  Widget info_widget= NULL;
  MrmType widget_class;
  MrmCode lit_type;
  XmString text_string= NULL;
  
  if (text == NULL) return;

  // Try to use the string as a name to fetch from the UID
  void* lit_value;
  if (MrmFetchLiteral(mrm_id, text, XtDisplay(app_shell),
		      &lit_value, &lit_type) == MrmSUCCESS) {
    // Put this literal in the dialog
    MRMRegisterArg names;
    names.name= "info_text_id";
    names.value= lit_value;
    MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  }
  else {
    // Use the string directly
    text_string= XmStringCreate(text, XmSTRING_DEFAULT_CHARSET);
    MRMRegisterArg names;
    names.name= "info_text_id";
    names.value= text_string;
    MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  }
    

  MrmFetchWidget(mrm_id, "info_dialog", app_shell, 
		 &info_widget, &widget_class);
  if (info_widget) {
    Widget button= XmMessageBoxGetChild(info_widget, XmDIALOG_CANCEL_BUTTON);
    if (button) XtUnmanageChild(button);
    button= XmMessageBoxGetChild(info_widget, XmDIALOG_HELP_BUTTON);
    if (button) XtUnmanageChild(button);
    XtManageChild(info_widget);
  }
  else 
  fprintf(stderr,
       "pop_info_dialog: info dialog not found; is uid file out of date?\n");

  if (text_string) XmStringFree(text_string);

}

static void destroy_dialog_cb( Widget w, int *type, caddr_t call_data )
{
  XtDestroyWidget( XtParent( w ) );
}

static void help_cb (Widget w,
		     char *text,
		     XtPointer cb) {
  Arg args[MAX_ARGS];
  int n;
  Widget help_widget= NULL;
  MrmType widget_class;
  
  if (text == NULL) return;

  MRMRegisterArg names;
  names.name= "help_text_id";
  names.value= text;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
  MrmFetchWidget(mrm_id, "help_dialog", app_shell, 
		 &help_widget, &widget_class);
  XtManageChild(help_widget);
}

static void close_help_cb( Widget w, int *type, caddr_t call_data )
{
  XtDestroyWidget( XtParent( XtParent( XtParent( w ) ) ) );
}

static void error_cb (Widget w, char *text, XtPointer cb) 
{
  Arg args[MAX_ARGS];
  int n;
  Widget error_widget= NULL;
  MrmType widget_class;
  MrmCode lit_type;
  
  if (text == NULL) return;

  // Use the string directly
  XmString text_string= XmStringCreate(text, XmSTRING_DEFAULT_CHARSET);
  MRMRegisterArg names;
  names.name= "error_text_id";
  names.value= text_string;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
    
  MrmFetchWidget(mrm_id, "error_dialog", app_shell, 
		 &error_widget, &widget_class);
  if (error_widget) {
    Widget button= XmMessageBoxGetChild(error_widget, XmDIALOG_CANCEL_BUTTON);
    if (button) XtUnmanageChild(button);
    button= XmMessageBoxGetChild(error_widget, XmDIALOG_HELP_BUTTON);
    if (button) XtUnmanageChild(button);
    XtManageChild(error_widget);
  }
  else 
  fprintf(stderr,
       "error_cb: error dialog not found; is uid file out of date?\n");

  if (text_string) XmStringFree(text_string);
}

static MRMRegisterArg mrm_names[] = {
    {"destroy_dialog_cb", (caddr_t)destroy_dialog_cb},
    {"help_cb",  (caddr_t) help_cb},
    {"close_help_cb", (caddr_t)close_help_cb},
    {"error_cb", (caddr_t)error_cb}
};

void vfleet_error_reg()
// Register appropriate callback names
{
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));
}
