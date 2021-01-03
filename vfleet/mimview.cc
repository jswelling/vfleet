/****************************************************************************
 * mimview.cc for pvm 2.4
 * Author Robert Earhart
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

/*mimview.cc
  creates an X window from which one may open image files.
  I view this as basically a test for xmautoimagehandler;
  it's really trivial.*/

#include <stdio.h>

#include <X11/keysym.h>
#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xext.h>
#endif /* SHM */
#endif /* LOCAL */

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"

static Widget message_box;
static XmString readyString;
static XmString renderingString;

static void manage_meCB(Widget w, Widget client_data, caddr_t call_data)
{
    XtManageChild(client_data);
}
 
static void unmanageCB(Widget w, Widget me, caddr_t call_data)
{
    XtUnmanageChild(me);
}

static Widget formats_init(Widget w)
{
    Arg args[20];
    register int n;
    char *top_text= "Supported formats are:";
    char *formats_text = "\
eps\n\
gif\n\
hdf\n\
icon\n\
iff\n\
mpnt\n\
pbm\n\
pcx\n\
pgm\n\
pic\n\
pict\n\
pix\n\
pnm\n\
ppm\n\
ras\n\
rgb\n\
rla\n\
rle\n\
rpbm\n\
rpgm\n\
rpnm\n\
rppm\n\
synu\n\
tiff\n\
x\n\
xbm\n\
xwd";
    char *descrip_text = "\
Encapsulated PostScript file\n\
Compuserve Graphics image file\n\
Hierarchical Data File\n\
Sun Icon and Cursor file\n\
Sun TAAC Image File Format\n\
Apple Macintosh MacPaint file\n\
Portable Bit Map file\n\
ZSoft IBM PC Paintbrush file\n\
Portable Gray Map file\n\
PIXAR picture file\n\
Apple Macintosh QuickDraw/PICT file\n\
Alias image file\n\
Portable any Map file\n\
Portable Pixel Map file\n\
Sun Rasterfile\n\
SGI RGB image file\n\
Wavefront raster image file\n\
Utah Run length encoded image file\n\
Raw Portable Bit Map file\n\
Raw Portable Gray Map file\n\
Raw Portable any Map file\n\
Raw Portable Pixel Map file\n\
Synu image file\n\
Tagged image file\n\
Stardent AVS X image file\n\
X11 bitmap file\n\
X Window System window dump image file";

    n=0;
    XtSetArg(args[n], XmNtitle, "Format List"); n++;
    XtSetArg(args[n], XmNmwmFunctions, -MWM_FUNC_RESIZE); n++;
    XtSetArg(args[n], XmNmwmDecorations, -MWM_DECOR_RESIZEH
	     -MWM_DECOR_BORDER); n++;
    Widget message_box=XmCreateFormDialog(w, "formats", args, n);

    n=0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNvalue, top_text); n++;
    XtSetArg(args[n], XmNshadowThickness, 0); n++;
    XtSetArg(args[n], XmNtraversalOn, False); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    Widget top=XmCreateText(message_box, "top", args, n);
    XtManageChild(top);

    XmString okText=XmStringCreateSimple("Close");
    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNbottomOffset, 2); n++;
    XtSetArg(args[n], XmNokLabelString, okText); n++;
    Widget bboard=XmCreateMessageBox(message_box, "buttons", args, n);
    XtManageChild(bboard);

    XmStringFree(okText);

    Widget it;
    if (it=XmMessageBoxGetChild(bboard, XmDIALOG_HELP_BUTTON))
      XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(bboard, XmDIALOG_CANCEL_BUTTON))
      XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(bboard, XmDIALOG_MESSAGE_LABEL))
      XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(bboard, XmDIALOG_SYMBOL_LABEL))
      XtUnmanageChild(it);

    if (it=XmMessageBoxGetChild(bboard, XmDIALOG_OK_BUTTON))
      XtAddCallback(it, XmNactivateCallback,
		    (XtCallbackProc)unmanageCB, message_box);

    n=0;
    XtSetArg(args[n], XmNdefaultButton, it); n++;
    XtSetValues(bboard, args, n);

    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, bboard); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, top); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNvalue, formats_text); n++;
    XtSetArg(args[n], XmNshadowThickness, 0); n++;
    XtSetArg(args[n], XmNtraversalOn, False); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNtraversalOn, False); n++;
    XtSetArg(args[n], XmNwidth, 50); n++;
    Widget formats=XmCreateText(message_box, "formats", args, n);
    XtManageChild(formats);
    
    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, bboard); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, top); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, formats); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNvalue, descrip_text); n++;
    XtSetArg(args[n], XmNshadowThickness, 0); n++;
    XtSetArg(args[n], XmNtraversalOn, False); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNwidth, 300); n++;
    XtSetArg(args[n], XmNheight, 400); n++;
    Widget descrip=XmCreateText(message_box, "descrip", args, n);
    XtManageChild(descrip);

    return(message_box);
}

static Widget about_init(Widget w)
{
    Arg args[10];
    register int n;
    char *text= "\
Mimview version 1.0\n\
\n\
By Robert Earhart\n\
\n\
This is a Motif based tool designed\n\
for viewing a variety of image formats.\n\
\n\
Copyright 1993, Pittsburgh Supercomputing Center and Carnegie Mellon University.";

    n=0;
    XmString helpText=XmStringCreateLtoR(text, XmSTRING_DEFAULT_CHARSET);
    XmString okText=XmStringCreateSimple("Close");
    XmString formatsText=XmStringCreateSimple("List Formats");
    XtSetArg(args[n], XmNmessageString, helpText); n++;
    XtSetArg(args[n], XmNokLabelString, okText); n++;
    XtSetArg(args[n], XmNhelpLabelString, formatsText); n++;
    XtSetArg(args[n], XmNmessageAlignment, XmALIGNMENT_CENTER); n++;
    XtSetArg(args[n], XmNtitle, "Help"); n++;
    XtSetArg(args[n], XmNmwmDecorations, -MWM_DECOR_RESIZEH
	     -MWM_DECOR_BORDER); n++;
    Widget message_box=XmCreateMessageDialog(w, "help", args, n);
 
    Widget button=XmMessageBoxGetChild(message_box, XmDIALOG_CANCEL_BUTTON);
    XtUnmanageChild(button);
    
    button=XmMessageBoxGetChild(message_box, XmDIALOG_HELP_BUTTON);
    XtAddCallback(button, XmNactivateCallback,
		  (XtCallbackProc)manage_meCB,
		  formats_init(message_box));
    XmStringFree(okText);
    XmStringFree(helpText);
    XmStringFree(formatsText);
    return(message_box);
}

static void destroy_handlerCB(Widget w, XmautoImageHandler *me,
			      caddr_t call_data)

/*What happens when we\'re XtDestroyed...*/
{
    if (me->last_image()) {
	fclose(me->fp);
	delete(me->last_image());
    }
    delete(me);
}

static void unmanage_and_refreshCB(Widget w, Widget me, caddr_t call_data)
{
    XtUnmanageChild(me);
    XmUpdateDisplay(XtParent(me));
}

static Widget bad_image_dlog;

static void open_this(char *the_name, Widget top) {
    Arg args[10];
    register int n;

    FILE *fp;
    rgbImage *image;

    if (! (fp=fopen(the_name, "r")))
	return;

    image=new rgbImage(fp, the_name);
    
    if (!image->valid()) {
      XtManageChild(bad_image_dlog);
      fclose(fp);
      delete(image);
      return;
    }

    n=0;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_WORKING); n++;
    XtSetArg(args[n], XmNmessageString, renderingString); n++;
    XtSetValues(message_box, args, n);
    XmUpdateDisplay(top);
    
    n=0;
    XtSetArg(args[n], XmNheight, image->ysize()); n++;
    XtSetArg(args[n], XmNwidth, image->xsize()); n++;
    XtSetArg(args[n], XmNmaxHeight, image->ysize()); n++;
    XtSetArg(args[n], XmNmaxWidth, image->xsize()); n++;
    XtSetArg(args[n], XmNgeometry, "+0-0"); n++;
    XtSetArg(args[n], XmNx, 0); n++;
    XtSetArg(args[n], XmNy, 0); n++;
    XtSetArg(args[n], XmNtitle, the_name); n++;
    XtSetArg(args[n], XmNmwmDecorations, -MWM_DECOR_RESIZEH
	     - MWM_DECOR_BORDER); n++;
    XmautoImageHandler *the_handler=
	new XmautoImageHandler(top, 256, 256, NULL, args, n,
			       image, fp);
    XtAddCallback(the_handler->widget(), XmNdestroyCallback,
                  (XtCallbackProc)(&destroy_handlerCB), the_handler);
    XmUpdateDisplay(top);
    n=0;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
    XtSetArg(args[n], XmNmessageString, readyString); n++;
    XtSetValues(message_box, args, n);
}

static void open_a_fileCB(Widget w, Widget top,
			  XmFileSelectionBoxCallbackStruct *call_data)
{
    char *the_name;
    if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
	return;
    open_this(the_name, top);
}

static Widget create_error_dlog(Widget w)
{
    Arg args[10];
    register int n;
    Widget error_dlog;
    
    char *warn_text="Error: Invalid file type for reading.";
    XmString okText=XmStringCreateSimple("Close");
    XmString dtitle=XmStringCreateSimple("Bad File");
    XmString warnText=XmStringCreateSimple(warn_text);

    n=0;
    XtSetArg(args[n], XmNfileTypeMask, XmFILE_REGULAR); n++;
    XtSetArg(args[n], XmNokLabelString, okText); n++;
    XtSetArg(args[n], XmNmessageString, warnText); n++;
    XtSetArg(args[n], XmNdialogTitle, dtitle); n++;
    error_dlog = XmCreateErrorDialog(w, "bad_file", args, n);

    Widget button;

    if (button=XmMessageBoxGetChild(error_dlog, XmDIALOG_HELP_BUTTON))
      XtUnmanageChild(button);
    if (button=XmMessageBoxGetChild(error_dlog, XmDIALOG_CANCEL_BUTTON))
      XtUnmanageChild(button);

    XmStringFree(okText);
    XmStringFree(warnText);
    XmStringFree(dtitle);
    return(error_dlog);
}

static Widget file_init(Widget w)
{
    Arg args[10];
    register int n;

    n=0;
    XtSetArg(args[n], XmNfileTypeMask, XmFILE_REGULAR); n++;
    XtSetArg(args[n], XmNtitle, "Open Image"); n++;
    Widget file_box=XmCreateFileSelectionDialog(w, "file", args, n);

    /*we need to get RID of the file box before opening the file,
      because in all likelyhood the file box obscures some part
      of the application, and we'd get two redraw events.

      BAD bad abd baad bad...*/
    
    XtAddCallback(file_box, XmNokCallback,
                  (XtCallbackProc)(&unmanage_and_refreshCB), file_box);
    XtAddCallback(file_box, XmNokCallback,
                  (XtCallbackProc)(&open_a_fileCB), w);

    XtAddCallback(file_box, XmNcancelCallback,
                  (XtCallbackProc)(&unmanageCB), file_box);

    Widget button;
    
    if (button=XmMessageBoxGetChild(file_box, XmDIALOG_HELP_BUTTON))
	XtUnmanageChild(button);

    bad_image_dlog=create_error_dlog(w);

    return(file_box);
}

static void quitCB(Widget w, Boolean *done, caddr_t call_data)
{
    *done=True;
}

int main(int argc, char *argv[]) {
    Arg args[10];
    register int n;
    XtAppContext app_context;
    
    n=0;
    XtSetArg(args[n], XmNgeometry, "175x75"); n++;
    XtSetArg(args[n], XmNmwmDecorations, -MWM_DECOR_RESIZEH
	     -MWM_DECOR_BORDER); n++;
    Widget toplevel=XtAppInitialize(&app_context, "Mimview", NULL,
                                    0, &argc, argv, NULL, args, n);

    /*Main window*/
    n=0;
    Widget main_window=XmCreateMainWindow(toplevel, "main", args, n);
    XtManageChild(main_window);
 
    /*Menu bar*/
    n=0;
    Widget menu_bar = XmCreateMenuBar(main_window, "menubar", args, n);
    XtManageChild(menu_bar);
    
    /*File Menu*/
    n=0;
    Widget menu_pane = XmCreatePulldownMenu(menu_bar, "file_menu", args, n);

    n=0;
    XmString labelString=XmStringCreateSimple("Open");
    XmString accelString=XmStringCreateSimple("Ctrl+O");
    XtSetArg(args[n], XmNaccelerator, "Ctrl<Key>O"); n++;
    XtSetArg(args[n], XmNacceleratorText, accelString); n++;
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_O); n++;
    Widget button = XmCreatePushButton(menu_pane, "open_datafile", args, n);
    XtManageChild(button);
    XmStringFree(labelString);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) manage_meCB,
                  file_init(toplevel));

    n=0;
    Widget separator = XmCreateSeparatorGadget(menu_pane, "", args, n);
    XtManageChild(separator);

    Boolean done;
    
    n=0;
    labelString=XmStringCreateSimple("Quit");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_Q); n++;
    button = XmCreatePushButton(menu_pane, "quit", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) quitCB, &done);
    XmStringFree(labelString);
 
    n=0;
    labelString=XmStringCreateSimple("File");
    XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_F); n++;
    Widget cascade = XmCreateCascadeButtonGadget(menu_bar, "file", args, n);
    XtManageChild(cascade);
    XmStringFree(labelString);

    /*Help Menu*/
    n=0;
    menu_pane = XmCreatePulldownMenu(menu_bar, "help_menu", args, n);
 
    n=0;
    labelString=XmStringCreateSimple("About");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_A); n++;
    button = XmCreatePushButton(menu_pane, "about", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) manage_meCB,
                  (XtPointer) about_init(toplevel));
    XmStringFree(labelString);
    
    n=0;
    labelString=XmStringCreateSimple("Help");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
    XtSetArg(args[n], XmNmnemonic, XK_H); n++;
    cascade=XmCreateCascadeButtonGadget(menu_bar, "help", args, n);
    XtManageChild(cascade);
    XmStringFree(labelString);
 
    n=0;
    XtSetArg(args[n], XmNmenuHelpWidget, cascade); n++;
    XtSetValues(menu_bar, args, n);

    readyString=XmStringCreateSimple("Ready.");
    renderingString=XmStringCreateSimple("Loading Image...");
    n=0;
    XtSetArg(args[n], XmNmessageString, readyString); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
    message_box=XmCreateMessageBox(main_window, "message_box", args, n);
    XtManageChild(message_box);

    Widget it;
    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_CANCEL_BUTTON))
	XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_HELP_BUTTON))
	XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_OK_BUTTON))
	XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_SEPARATOR))
	XtUnmanageChild(it);
    
    XmMainWindowSetAreas(main_window, menu_bar, message_box, NULL, NULL, NULL);

    XEvent the_event;
    
    XtRealizeWidget(toplevel);

    /*Now, open all the images we've been passed on the command line...*/

    for (n=1; n<argc; n++)
	open_this(argv[n], toplevel);

    /* Go! */
    for (done = False; ! done ;) {
        XtAppNextEvent(app_context, &the_event);
        XtDispatchEvent(&the_event);
    };

    /* And exit. */

    /* We do this in order to hit all of the destroy callbacks for the entire
       widget tree. */
    
    XtDestroyWidget(toplevel);
}
