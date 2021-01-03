/*test
  a test module for the crystal ball widget.*/

#include <stdio.h>
#include <Mrm/MrmAppl.h>
#include <Xm/MessageB.h>
#include "crystal.h"

XtAppContext  app_context;

#define k_help_id 1
#define k_crystal_id 2

static void create_cb();
static void help_cb();
static void exit_cb();

static void arm_cb();
static void disarm_cb();
static void drag_cb();
static void value_changed_cb();
static void redraw_cb();
static void resize_cb();

static MrmHierarchy mrm_id;
static char *mrm_vec[]={"test.uid"};
static MrmCode mrm_class;

static MRMRegisterArg mrm_names[] = {
    {"create_cb",  (caddr_t) create_cb},
    {"help_cb",  (caddr_t) help_cb},
    {"exit_cb",  (caddr_t) exit_cb},
    {"arm_cb",  (caddr_t) arm_cb},
    {"disarm_cb",  (caddr_t) disarm_cb},
    {"drag_cb",  (caddr_t) drag_cb},
    {"value_changed_cb",  (caddr_t) value_changed_cb},
    {"redraw_cb",  (caddr_t) redraw_cb},
    {"resize_cb",  (caddr_t) resize_cb}
};

static Widget help_id;

static Widget crystal_id;

int main(argc, argv)
int argc;
char **argv;
{
    Widget shell;
    Display *display;
    Widget app_main = NULL;
    Arg args[3];
 
    MrmInitialize ();
    CrystalMrmInitialize();
 
    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();
    display = XtOpenDisplay(app_context, NULL, argv[0], "Test",
                        NULL, 0, &argc, argv);
    
    if (display == NULL) {
	fprintf(stderr, "%s:  Can't open display\n", argv[0]);
	exit(1);
    }
 
    XtSetArg (args[0], XtNallowShellResize, True);
    shell = XtAppCreateShell(argv[0], NULL, applicationShellWidgetClass,
                          display, args, 1);
 
    if (MrmOpenHierarchy (1, mrm_vec, NULL, &mrm_id) != MrmSUCCESS) exit(0);
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    MrmFetchWidget (mrm_id, "app_main", shell, &app_main, &mrm_class);
    XtManageChild(app_main);
    XtRealizeWidget(shell);
    XtAppMainLoop(app_context);
    return(0);
}
 
static void create_cb(w, id, reason)
    Widget w;
    int *id;
    unsigned long *reason;
{
    switch (*id) {
    case k_crystal_id: crystal_id = w; break;
        case k_help_id:
            help_id = w;
	XtUnmanageChild(XmMessageBoxGetChild(help_id,
                                XmDIALOG_CANCEL_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(help_id,
                                XmDIALOG_HELP_BUTTON));
            break;
    }
}

static void help_cb (w, name, cb)
    Widget w;
    XmString name;
    XtPointer cb;
{
    Arg args[1];
 
    if (name == NULL) return;
    XtSetArg (args[0], XmNmessageString, name);
    XtSetValues(help_id, args, 1);
    XtManageChild(help_id);
}
 
static void exit_cb (w, data, cb)
    Widget w;
    XtPointer data;
    XtPointer cb;
{
    exit(0);
}

static void arm_cb (w, data, event)
    Widget w;
    XtPointer data;
    XEvent *event;
{
    printf("Armed at %d, %d.\n", event->xbutton.x, event->xbutton.y);
}

static void disarm_cb (w, data, event)
    Widget w;
    XtPointer data;
    XEvent *event;
{
    printf("Disarmed at %d, %d.\n", event->xbutton.x, event->xbutton.y);
}

void ptrans(float *trans) {
    register short i,j;
    for (i=0; i<=3; i++, printf("\n"))
	for (j=0; j<=3; j++)
	    printf("%f ", *(trans+j+(4*i)));
}

static void drag_cb (w, data, trans)
    Widget w;
    XtPointer data;
    float *trans;
{
    printf("Dragged object matrix to\n");
    ptrans(trans);
}

static void value_changed_cb (w, data, trans)
    Widget w;
    XtPointer data;
    float *trans;
{
    printf("Transformed object matrix by\n");
    ptrans(trans);
}

static void redraw_cb (w, data, toss)
Widget w;
XtPointer data;
XtPointer toss;
{
    printf("Received a redraw event.\n");
}

static void resize_cb (w, data, toss)
Widget w;
XtPointer data;
XtPointer toss;
{
    printf("Received a resize event.\n");
}

