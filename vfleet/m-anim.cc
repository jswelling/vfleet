/****************************************************************************
 * m-anim.cc for pvm 2.4
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

#include <stdio.h>
#include <stdlib.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <Xm/FileSB.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xext.h>
#endif /* SHM */
#endif /* LOCAL */

#include "playb.xbm"
#include "stop.xbm"
#include "playf.xbm"

#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"

static void manage_meCB(Widget w, Widget client_data, caddr_t call_data)
{
    XtManageChild(client_data);
}
 
static void unmanageCB(Widget w, Widget me, caddr_t call_data)
{
    XtUnmanageChild(me);
}


static Widget works_init(Widget w)
{
    Arg args[20];
    register int n;
#ifdef SHM
    char *works_text = "\
  M-Anim accepts a list of files, and loads them into your X server\n\
sequentially. It then allows you to scroll through them, or play\n\
them back in a movie like fashion.\n\
\n\
  M-Anim was compiled with Shared Memory turned on, and thus will attempt\n\
to use shared memory XImages whenever possible. This reduces the load on\n\
the X server, and seems to make things faster. Note that if a request by\n\
the program for a shared memory segment fails, it will still default back\n\
to the normal, non-shared memory behavior.\n\
\n\
  If you wish to turn off use of shared memory for a particular session\n\
(for instance, if you suspect that the use of shared memory is degrading\n\
 your system performance), set your DISPLAY environment variable to\n\
`hostname:0`. In the csh shell (the usual default for Unix systems), this\n\
is done by entering the command \"setenv DISPLAY `hostname:0`\" before\n\
running the program.\n\
\n\
  Also, if the program should happen to crash when using shared memory, the\n\
shared memory segments it had allocated will not be deallocated, and thus\n\
it is necessary to remover them manually. See your man pages on the \"ipcs\"\n\
and \"ipcrm\" commands for more details.\n\
\n\
  Enjoy! Questions/Comments should be sent to the email address\n\
\"welling@psc.edu\".\n\
";
#else
char *works_text = "\
  M-Anim accepts a list of files, and loads them into your X server\n\
sequentially. It then allows you to scroll through them, or play\n\
them back in a movie like fashion.\n\
\n\
  M-Anim was not compiled with Shared Memory turned on, and thus will\n\
never use shared memory, even if running locally with an XServer that\n\
supports it.\n\
\n\
  Enjoy! Questions/Comments should be sent to the email address\n\
\"welling@psc.edu\".\n\
";
#endif /* SHM */
    
    XmString okText=XmStringCreateSimple("Close");
    XmString messageText=XmStringCreateLtoR(works_text,
              XmSTRING_DEFAULT_CHARSET);

    n=0;
    XtSetArg(args[n], XmNtitle, "How it Works"); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNbottomOffset, 2); n++;
    XtSetArg(args[n], XmNokLabelString, okText); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
    XtSetArg(args[n], XmNmessageString, messageText); n++;
    Widget message_box=XmCreateMessageDialog(w, "works", args, n);

    XmStringFree(okText);
    XmStringFree(messageText);

    Widget it;
    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_HELP_BUTTON))
      XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_CANCEL_BUTTON))
      XtUnmanageChild(it);

    if (it=XmMessageBoxGetChild(message_box, XmDIALOG_OK_BUTTON)) {
	XtAddCallback(it, XmNactivateCallback,
		      (XtCallbackProc)unmanageCB, (XtPointer)message_box);

	n=0;
	XtSetArg(args[n], XmNdefaultButton, it); n++;
	XtSetValues(message_box, args, n);
    }
    return(message_box);
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
		    (XtCallbackProc)unmanageCB, (XtPointer)message_box);

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
M-anim version 1.0\n\
\n\
By Robert Earhart\n\
\n\
This is a Motif based tool designed\n\
for viewing a sequence of images (generally, an animation).\n\
\n\
Copyright 1993, Pittsburgh Supercomputing Center";

    n=0;
    XmString helpText=XmStringCreateLtoR(text, XmSTRING_DEFAULT_CHARSET);
    XmString okText=XmStringCreateSimple("Close");
    XmString worksText=XmStringCreateSimple("How it Works");
    XmString formatsText=XmStringCreateSimple("List Formats");
    XtSetArg(args[n], XmNmessageString, helpText); n++;
    XtSetArg(args[n], XmNokLabelString, okText); n++;
    XtSetArg(args[n], XmNcancelLabelString, worksText); n++;
    XtSetArg(args[n], XmNhelpLabelString, formatsText); n++;
    XtSetArg(args[n], XmNmessageAlignment, XmALIGNMENT_CENTER); n++;
    XtSetArg(args[n], XmNtitle, "Help"); n++;
    XtSetArg(args[n], XmNautoUnmanage, False); n++;
    Widget message_box=XmCreateMessageDialog(w, "help", args, n);
 
    Widget button=XmMessageBoxGetChild(message_box, XmDIALOG_OK_BUTTON);
    XtAddCallback(button, XmNactivateCallback,
		  (XtCallbackProc)unmanageCB, (XtPointer)message_box);
    
    button=XmMessageBoxGetChild(message_box, XmDIALOG_CANCEL_BUTTON);
    XtAddCallback(button, XmNactivateCallback,
		  (XtCallbackProc)manage_meCB,
		  (XtPointer)works_init(message_box));
    
    button=XmMessageBoxGetChild(message_box, XmDIALOG_HELP_BUTTON);
    XtAddCallback(button, XmNactivateCallback,
		  (XtCallbackProc)manage_meCB,
		  (XtPointer)formats_init(message_box));
    XmStringFree(okText);
    XmStringFree(helpText);
    XmStringFree(formatsText);
    XmStringFree(worksText);
    return(message_box);
}

static void quitCB(Widget w, Boolean *done, caddr_t call_data)
{
    *done=True;
}

typedef struct redraw_struct {
    XtAppContext app_context;
    int number, current;
    XImageHandler **the_images;
    int argc;
    char **argv;
    Widget top;
    Widget the_picture, sb, message_box;
    XmString readyString, renderingString, forwardString, backwordString;
    Widget playb, stop, playf, active, once, repeat, bounce;
    XtIntervalId timer;
    unsigned long timeout;
    int stepsize;
    Boolean time_installed;
} redraw_data;

static void animate_proc(redraw_data *cdat) {

    Boolean re_add = True; /*Toggle for whether to add work proc again or not*/

    cdat->time_installed = False;

    if (cdat->stepsize > 0)
	if ((cdat->current + cdat->stepsize) > cdat->number)
	    if (XmToggleButtonGadgetGetState(cdat->once)) {
		cdat->current = cdat->number;
		re_add = False;
		XmToggleButtonGadgetSetState(cdat->stop,True, True);
	    } else
		if (XmToggleButtonGadgetGetState(cdat->repeat))
		    cdat->current = 1;
		else {			/* Must be bouncing */
		    cdat->current=cdat->number;
		    XmToggleButtonGadgetSetState(cdat->playb,True, True);
		}
	else cdat->current+=cdat->stepsize;
    else if (cdat->stepsize < 0)
	if ((cdat->current + cdat->stepsize) < 1)
	    if (XmToggleButtonGadgetGetState(cdat->once)) {
		cdat->current = 1;
		re_add = False;
		XmToggleButtonGadgetSetState(cdat->stop,True, True);
	    } else if (XmToggleButtonGadgetGetState(cdat->repeat))
		cdat->current = cdat->number;
	    else {   			/* Must be boucing */
		cdat->current= 1;
		XmToggleButtonGadgetSetState(cdat->playf, True, True);
	    }
	else
	    cdat->current += cdat->stepsize;
    else
	re_add = False;
    
    int value, slider, increment, page;
    XmScrollBarGetValues(cdat->sb, &value, &slider, &increment, &page);
    XmScrollBarSetValues(cdat->sb, cdat->current, slider, increment, page,
			 False);
    if (re_add && (! cdat->time_installed)) {
	cdat->timer=XtAppAddTimeOut(cdat->app_context, cdat->timeout,
				    (XtTimerCallbackProc)&animate_proc,
				    (XtPointer) cdat);
	cdat->time_installed = True;
    }
    
    cdat->the_images[cdat->current]->redraw(); /* This should definitly happen
						AFTER we reinstall this proc,
						so the timeouts look better.*/
}

static void movestyleCB(Widget w, redraw_data *cdat,
			XmToggleButtonCallbackStruct *call_data)
{
    if (! (XmToggleButtonGadgetGetState(w))) {
	XmToggleButtonGadgetSetState(w, True, False);
	return;
    }

    if (w != cdat->once)
	XmToggleButtonGadgetSetState(cdat->once, False, False);
    if (w != cdat->repeat)
	XmToggleButtonGadgetSetState(cdat->repeat, False, False);
    if (w != cdat->bounce)
	XmToggleButtonGadgetSetState(cdat->bounce, False, False);
}

static void toggleplayCB(Widget w, redraw_data *cdat,
			 XmToggleButtonCallbackStruct *call_data)
{
    Arg args[15];
    register int n;
    
    if (! (XmToggleButtonGadgetGetState(w))) {
	XmToggleButtonGadgetSetState(w, True, False);
	return;
    }

    XmToggleButtonGadgetSetState(cdat->active, False, False);
    
    n=0;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
    
    if (w == cdat->stop) {
	XmProcessTraversal(cdat->active, XmTRAVERSE_CURRENT);
	XmProcessTraversal(cdat->active, XmTRAVERSE_CURRENT);
	if (cdat->time_installed) {
	    XtRemoveTimeOut(cdat->timer);
	    cdat->time_installed = False;
	}
	cdat->stepsize = 0;
	XtSetArg(args[n], XmNmessageString, cdat->readyString); n++;
    } else {
	XmProcessTraversal(cdat->stop, XmTRAVERSE_CURRENT);
	XmProcessTraversal(cdat->stop, XmTRAVERSE_CURRENT);
	if (! cdat->time_installed) {
	    cdat->timer=XtAppAddTimeOut(cdat->app_context, cdat->timeout,
					(XtTimerCallbackProc)&animate_proc,
					(XtPointer) cdat);
	    cdat->time_installed = True;
	}
	if (w == cdat->playf) {
	    cdat->stepsize = 1;
	    XtSetArg(args[n], XmNmessageString, cdat->forwardString); n++;
	}
	else {
	    cdat->stepsize = -1;
	    XtSetArg(args[n], XmNmessageString, cdat->backwordString); n++;
	}
    }
    XtSetValues(cdat->message_box, args, n);
    cdat->active = w;
}

static void scrollCB(Widget w, redraw_data *cdat,
		     XmScrollBarCallbackStruct *call_data);

static void redrawCB(Widget w, redraw_data *cdat,
		     XmDrawingAreaCallbackStruct *call_data)
{
    if (! XtIsRealized(w))
	return;
    if (! cdat->the_images) {			/* groan... read them in... */
	Arg args[15];
	register int n;	

	cdat->the_images = (XImageHandler **)calloc((cdat->number) + 1,
						sizeof(XImageHandler *));
	if (! cdat->the_images) {
	    perror(cdat->argv[0]);
	    exit(1);
	}

	Cursor watch=XCreateFontCursor(XtDisplay(w), XC_watch);
	n=0;
	XtSetArg(args[n], XmNdialogType, XmDIALOG_WORKING); n++;
	XtSetArg(args[n], XmNmessageString, cdat->renderingString); n++;
	XtSetValues(cdat->message_box, args, n);
    
	XDefineCursor(XtDisplay(w), XtWindow(cdat->top), watch);
	rgbImage *the_image;
	FILE *the_file;

	XtRemoveCallback(w, XmNexposeCallback,
			 (XtCallbackProc) redrawCB,
			 (XtPointer) cdat);
	    
	/*I wish I knew why this call needs to be made twice... but it does
	  so I will.*/
	XmProcessTraversal(cdat->sb, XmTRAVERSE_CURRENT);
	XmProcessTraversal(cdat->sb, XmTRAVERSE_CURRENT);
	
	for (int count=1; count<(cdat->argc); count++) {
	    n=0;
	    XtSetArg(args[n], XmNvalue, count); n++;
	    XtSetValues(cdat->sb, args, n);

	    XmUpdateDisplay(w);

	    if (! (the_file=fopen(cdat->argv[count], "r"))) {
		perror(cdat->argv[0]);
		exit(1);
	    }
	    
	    the_image=new rgbImage(the_file, cdat->argv[count]);
	    if (! the_image->valid()) {
		fprintf(stderr, "%s: Invalid image file %s\n",
			cdat->argv[0], cdat->argv[count]);
		exit(1);
	    }
	    cdat->the_images[count]= new XImageHandler(XtDisplay(w),
						       0, XtWindow(w));
	    
	    if (! cdat->the_images[count]) {
		perror(cdat->argv[0]);
		exit(1);
	    }
	    
	    cdat->the_images[count]->display(the_image, True);
	    
	    fclose(the_file);
	    delete(the_image); 
	}
	XUndefineCursor(XtDisplay(w), XtWindow(cdat->top));
	n=0;
	XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
	XtSetArg(args[n], XmNmessageString, cdat->readyString); n++;
 	XtSetValues(cdat->message_box, args, n);

	n=0;
	XtSetArg(args[n], XmNvalue, cdat->current); n++;
	XtSetValues(cdat->sb, args, n);

	XmUpdateDisplay(w);

	XtAddCallback(cdat->sb, XmNvalueChangedCallback,
		      (XtCallbackProc) scrollCB,
		      (XtPointer) cdat);

/*
  XtAddCallback(cdat->sb, XmNdragCallback,
  (XtCallbackProc) scrollCB,
  (XtPointer) cdat);
*/
	XtAddCallback(w, XmNexposeCallback,
			 (XtCallbackProc) redrawCB,
			 (XtPointer) cdat);

	/*I wish I knew why this call needs to be made twice... but it does
	  so I will.*/
	XmProcessTraversal(cdat->playf, XmTRAVERSE_CURRENT);
	XmProcessTraversal(cdat->playf, XmTRAVERSE_CURRENT);
    }
    cdat->the_images[cdat->current]->redraw();
    cdat->time_installed = False;
}

static void set_speedCB(Widget w, redraw_data *cdat,
		     XmToggleButtonCallbackStruct *call_data)
{
    Arg args[1];
    if (call_data->set) {
	XtSetArg(args[0], XmNuserData, &cdat->timeout);
	XtGetValues(w, args, 1);
    }
}

static void scrollCB(Widget w, redraw_data *cdat,
		     XmScrollBarCallbackStruct *call_data)
{
    cdat->current=call_data->value;
    cdat->the_images[cdat->current]->redraw();
    if (! cdat->time_installed)		/* do NOT reinstall. :) */
	cdat->timer=XtAppAddTimeOut(cdat->app_context, cdat->timeout,
				    (XtTimerCallbackProc)&animate_proc,
				    (XtPointer) cdat);
}

#ifdef SHM
int do_nothing(Display *dpy, XErrorEvent *err) {
    /* This exists just to keep MIT-SHM from whining
       too loudly.*/
}
#endif /* SHM */

int main(int argc, char *argv[]) {
    Arg args[15];
    register int n;
    Display *display;
    
    redraw_data *the_data = new redraw_data;

    XtToolkitInitialize();

#ifdef SHM
    /*Keep Dec's from whining if the server doesn't have MIT-SHM*/
    XSetExtensionErrorHandler(&do_nothing);
#endif /* SHM */
    
    the_data->app_context = XtCreateApplicationContext();

    if ((display = XtOpenDisplay(the_data->app_context, NULL, argv[0],
				 "M-Anim", NULL, 0, &argc,
				 argv)) == NULL) {
	fprintf(stderr, "\n%s: Can't open display\n", argv[0]);
	exit(1);
    }

    n=0;
    XtSetArg(args[n], XmNargc, argc); n++;
    XtSetArg(args[n], XmNargv, argv); n++;
    XtSetArg(args[n], XmNmwmFunctions, -MWM_FUNC_RESIZE); n++;
    XtSetArg(args[n], XmNmwmDecorations, -MWM_DECOR_RESIZEH
             -MWM_DECOR_BORDER); n++;
   Widget toplevel=XtAppCreateShell(argv[0], "M-Anim",
				     applicationShellWidgetClass,
				     display, args, n);

    if (argc < 2) {
	fprintf(stderr, "%s: Usage: %s files\n", argv[0], argv[0]);
	exit(1);
    }

    the_data->number = argc-1;
    the_data->current = 1;
    the_data->the_images = NULL;
    the_data->argc = argc;
    the_data->argv = argv;
    
    /*Main window*/
    n=0;
    the_data->top=XmCreateMainWindow(toplevel, "main", args, n);
    XtManageChild(the_data->top);
 
    /*Menu bar*/
    n=0;
    Widget menu_bar = XmCreateMenuBar(the_data->top, "menubar", args, n);
    XtManageChild(menu_bar);
    
    /*File Menu*/
    n=0;
    Widget menu_pane = XmCreatePulldownMenu(menu_bar, "file_menu", args, n);

    Boolean done;
    
    n=0;
    XmString labelString=XmStringCreateSimple("Quit");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_Q); n++;
    Widget button = XmCreatePushButton(menu_pane, "quit", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) quitCB, (XtPointer)&done);
    XmStringFree(labelString);
 
    n=0;
    labelString=XmStringCreateSimple("File");
    XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_F); n++;
    Widget cascade = XmCreateCascadeButton (menu_bar, "file", args, n);
    XtManageChild(cascade);
    XmStringFree(labelString);

    /*Control Menu*/
    n=0;
    menu_pane = XmCreatePulldownMenu(menu_bar, "control_menu", args, n);

    n=0;
    labelString=XmStringCreateSimple("Once Only");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_O); n++;
    XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
    XtSetArg(args[n], XmNset, True); n++;
    the_data->once = XmCreateToggleButtonGadget(menu_pane, "once", args, n);
    XtManageChild(the_data->once);
    XtAddCallback(the_data->once, XmNvalueChangedCallback,
                  (XtCallbackProc) movestyleCB, (XtPointer)the_data);
    XmStringFree(labelString);

    n=0;
    labelString=XmStringCreateSimple("Repeat");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_R); n++;
    XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
    the_data->repeat = XmCreateToggleButtonGadget(menu_pane, "repeat", args,
						  n);
    XtManageChild(the_data->repeat);
    XtAddCallback(the_data->repeat, XmNvalueChangedCallback,
                  (XtCallbackProc) movestyleCB, (XtPointer)the_data);
    XmStringFree(labelString);

    n=0;
    labelString=XmStringCreateSimple("Bounce");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_B); n++;
    XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
    the_data->bounce = XmCreateToggleButtonGadget(menu_pane, "bounce", args,
						  n);
    XtManageChild(the_data->bounce);
    XtAddCallback(the_data->bounce, XmNvalueChangedCallback,
                  (XtCallbackProc) movestyleCB, (XtPointer)the_data);
    XmStringFree(labelString);

    n=0;
    Widget sep = XmCreateSeparatorGadget(menu_pane, "sep", args, n);
    XtManageChild(sep);

    n=0;
    XtSetArg(args[n], XmNradioBehavior, True); n++;
    XtSetArg(args[n], XmNradioAlwaysOne, True); n++;
    Widget delay_menu_pane = XmCreatePulldownMenu(menu_pane, "delay_menu",
						  args, n);

    n=0;
    labelString=XmStringCreateSimple("0 ms.");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_0); n++;
    XtSetArg(args[n], XmNuserData, 1); n++;
    XtSetArg(args[n], XmNset, True); n++;
    button = XmCreateToggleButtonGadget(delay_menu_pane, "0", args, n); n++;
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
                  (XtCallbackProc) set_speedCB, (XtPointer)the_data);

    the_data->timeout = 1;
    
    n=0;
    labelString=XmStringCreateSimple("30 ms.");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_3); n++;
    XtSetArg(args[n], XmNuserData, 30); n++;
    button = XmCreateToggleButtonGadget(delay_menu_pane, "30", args, n); n++;
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
                  (XtCallbackProc) set_speedCB, (XtPointer)the_data);
    
    n=0;
    labelString=XmStringCreateSimple("60 ms.");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_6); n++;
    XtSetArg(args[n], XmNuserData, 60); n++;
    button = XmCreateToggleButtonGadget(delay_menu_pane, "60", args, n); n++;
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
                  (XtCallbackProc) set_speedCB, (XtPointer)the_data);
    
    n=0;
    labelString=XmStringCreateSimple("100 ms.");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_1); n++;
    XtSetArg(args[n], XmNuserData, 100); n++;
    button = XmCreateToggleButtonGadget(delay_menu_pane, "100", args, n); n++;
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
                  (XtCallbackProc) set_speedCB, (XtPointer)the_data);
    
    n=0;
    labelString=XmStringCreateSimple("150 ms.");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_5); n++;
    XtSetArg(args[n], XmNuserData, 150); n++;
    button = XmCreateToggleButtonGadget(delay_menu_pane, "150", args, n); n++;
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
                  (XtCallbackProc) set_speedCB, (XtPointer)the_data);
    
    n=0;
    labelString=XmStringCreateSimple("200 ms.");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_2); n++;
    XtSetArg(args[n], XmNuserData, 200); n++;
    button = XmCreateToggleButtonGadget(delay_menu_pane, "200", args, n); n++;
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
                  (XtCallbackProc) set_speedCB, (XtPointer)the_data);
    
    n=0;
    labelString=XmStringCreateSimple("Delay");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNmnemonic, XK_D); n++;
    XtSetArg(args[n], XmNsubMenuId, delay_menu_pane); n++;
    button  = XmCreateCascadeButtonGadget(menu_pane, "delay", args, n);
    XtManageChild(button);
    XmStringFree(labelString);

    n=0;
    labelString=XmStringCreateSimple("Control");
    XtSetArg(args[n], XmNlabelString, labelString); n++;
    XtSetArg(args[n], XmNsubMenuId, menu_pane); n++;
    XtSetArg(args[n], XmNmnemonic, XK_C); n++;
    cascade=XmCreateCascadeButton(menu_bar, "control", args, n);
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
    cascade=XmCreateCascadeButton(menu_bar, "help", args, n);
    XtManageChild(cascade);
    XmStringFree(labelString);
 
    n=0;
    XtSetArg(args[n], XmNmenuHelpWidget, cascade); n++;
    XtSetValues(menu_bar, args, n);

    n=0;
    Widget mainform=XmCreateForm(the_data->top, "mainform", args, n);
    XtManageChild(mainform);
    
    XmMainWindowSetAreas(the_data->top, menu_bar, NULL,
			 NULL, NULL, mainform);

    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    Widget frame=XmCreateFrame(mainform, "frame", args, n);
    XtManageChild(frame);

    n=0;
    XtSetArg(args[n], XmNfractionBase, 2); n++;
    Widget form=XmCreateForm(frame, "form", args, n);
    XtManageChild(form);
    
    the_data->readyString=XmStringCreateSimple("Ready.");
    the_data->renderingString=XmStringCreateSimple("Loading Images...");
    the_data->forwardString=XmStringCreateSimple("Animating Forwards...");
    the_data->backwordString=XmStringCreateSimple("Animating Backwords...");
    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(args[n], XmNrightPosition, 1); n++;
    XtSetArg(args[n], XmNmessageString, the_data->readyString); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
    the_data->message_box=XmCreateMessageBox(form, "message_box",
					     args, n);
    XtManageChild(the_data->message_box);

    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, the_data->message_box); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, the_data->message_box); n++;
    XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
    sep=XmCreateSeparatorGadget(form, "sep1", args, n);
    XtManageChild(sep);

    Widget it;
    if (it=XmMessageBoxGetChild(the_data->message_box, XmDIALOG_CANCEL_BUTTON))
	XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(the_data->message_box, XmDIALOG_HELP_BUTTON))
	XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(the_data->message_box, XmDIALOG_OK_BUTTON))
	XtUnmanageChild(it);
    if (it=XmMessageBoxGetChild(the_data->message_box, XmDIALOG_SEPARATOR))
	XtUnmanageChild(it);


    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, the_data->message_box); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, sep); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNfractionBase, 3); n++;
    Widget rc=XmCreateForm(form, "rc", args, n);
    XtManageChild(rc);

    Cardinal dep;
    Pixel fg, bg;
    
    n=0;
    XtSetArg(args[n], XmNdepth, &dep); n++;
    XtSetArg(args[n], XmNforeground, &fg); n++;
    XtSetArg(args[n], XmNbackground, &bg); n++;
    XtGetValues(rc, args, n);
    
    n=0;
    XtSetArg(args[n], XmNlabelPixmap,
	     XCreatePixmapFromBitmapData(display,
					 DefaultRootWindow(display),
					 playb_bits, playb_width,
					 playb_height, fg, bg, dep));n++;
    XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
    XtSetArg(args[n], XmNindicatorOn, False); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(args[n], XmNrightPosition, 1); n++;
    the_data->playb=XmCreateToggleButtonGadget(rc, "playb", args, n);
    XtManageChild(the_data->playb);
    XtAddCallback(the_data->playb, XmNvalueChangedCallback,
                  (XtCallbackProc) toggleplayCB,
                  (XtPointer) the_data);

    n=0;
    XtSetArg(args[n], XmNlabelPixmap,
	     XCreatePixmapFromBitmapData(display,
					 DefaultRootWindow(display),
					 stop_bits, stop_width,
					 stop_height, fg, bg, dep));n++;
    XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
    XtSetArg(args[n], XmNindicatorOn, False); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(args[n], XmNleftPosition, 1); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(args[n], XmNrightPosition, 2); n++;
    XtSetArg(args[n], XmNset, True); n++;
    the_data->stop=XmCreateToggleButtonGadget(rc, "stop", args, n);
    XtManageChild(the_data->stop);
    XtAddCallback(the_data->stop, XmNvalueChangedCallback,
                  (XtCallbackProc) toggleplayCB,
                  (XtPointer) the_data);

    n=0;
    XtSetArg(args[n], XmNlabelPixmap,
	     XCreatePixmapFromBitmapData(display,
					 DefaultRootWindow(display),
					 playf_bits, playf_width,
					 playf_height, fg, bg, dep));n++;
    XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
    XtSetArg(args[n], XmNindicatorOn, False); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(args[n], XmNleftPosition, 2); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    the_data->playf=XmCreateToggleButtonGadget(rc, "playf", args, n);
    XtManageChild(the_data->playf);
    XtAddCallback(the_data->playf, XmNvalueChangedCallback,
                  (XtCallbackProc) toggleplayCB,
                  (XtPointer) the_data);

    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, sep); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    Widget sep2=XmCreateSeparatorGadget(form, "sep2", args, n);
    XtManageChild(sep2);

    n=0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++; 
    XtSetArg(args[n], XmNbottomWidget, sep2); n++; 
    XtSetArg(args[n], XmNsliderSize, 1); n++;
    XtSetArg(args[n], XmNminimum, 1); n++;
    XtSetArg(args[n], XmNmaximum, argc); n++;
    XtSetArg(args[n], XmNvalue, 1); n++;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
    the_data->sb=XmCreateScrollBar(form, "sb", args, n);
    XtManageChild(the_data->sb);

    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, the_data->sb); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    sep2=XmCreateSeparatorGadget(form, "sep3", args, n);
    XtManageChild(sep2);

    FILE *the_file;			/* HACK hack hack... shh... :) */
    if (! (the_file=fopen(argv[1], "r"))) {
	perror(argv[0]);
	exit(1);
    }

    rgbImage *the_image=new rgbImage(the_file, argv[1]);
    if (! the_image->valid()) {
	fprintf(stderr, "%s: Invalid image file %s\n", argv[0], argv[1]);
	exit(1);
    }
    
    n=0;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, sep2); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNwidth, the_image->xsize()); n++;
    XtSetArg(args[n], XmNheight, the_image->ysize()); n++;
    Widget the_area=XmCreateDrawingArea(form, "draw", args, n);
    XtManageChild(the_area);

    XtAddCallback(the_area, XmNexposeCallback,
                  (XtCallbackProc) redrawCB,
                  (XtPointer) the_data);
    
    fclose(the_file);
    delete the_image;

    the_data->active = the_data->stop;
    
    XEvent the_event;
    
    XtRealizeWidget(toplevel);

    for (done = False; ! done ;) {
        XtAppNextEvent(the_data->app_context, &the_event);
        XtDispatchEvent(&the_event);
    };

    /*When we're done, we *MUST* delete our XImageHandlers, because they
      *COULD* be using shared memory images, and we don't want to leave
      those lying around the machine...*/

    for (n=1; n <= the_data->number; n++)
	delete(the_data->the_images[n]);
}
