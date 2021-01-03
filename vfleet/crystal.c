#include <stdio.h>
#include <Xm/XmP.h>
#include <Mrm/MrmAppl.h>

#include "crystal_ball.h"

#include "crystal.h"
#include "crystalP.h"

#define HighlightThickness(w) \
                XmField(w,offsets,XmPrimitive,highlight_thickness,Dimension)
#define ShadowThickness(w) \
                XmField(w,offsets,XmPrimitive,shadow_thickness,Dimension)
#define Highlighted(w) XmField(w,offsets,XmPrimitive,highlighted,Boolean)
#define TopShadowGC(w) XmField(w,offsets,XmPrimitive,top_shadow_GC,GC)
#define BottomShadowGC(w) XmField(w,offsets,XmPrimitive,bottom_shadow_GC,GC)
#define Width(w) XmField(w,offsets,Core,width,Dimension)
#define Height(w) XmField(w,offsets,Core,height,Dimension)
#define ViewMatrix(w) XmField(w,offsets,Crystal,view_matrix,float *)

static void ClassInitialize();
static void Initialize();
static void Redisplay();
static void Resize();
static void Destroy();
static Boolean SetValues();
static XtGeometryResult QueryGeometry();
static void rotate_up();
static void rotate_down();
static void rotate_left();
static void rotate_right();
static void arm();
static void disarm();

static char defaultTranslations[] =
    "<Btn2Down>:         Arm()\n\
     <Btn2Up>:           Disarm()\n\
     <Key>osfUp:         RotateUp(5)\n\
     <Key>osfDown:       RotateDown(5)\n\
     <Key>osfRight:      RotateRight(5)\n\
     <Key>osfLeft:       RotateLeft(5)\n\
     <Key>osfHelp:       PrimitiveHelp()";

static XtActionsRec actionsList[] = {
    { "Arm", (XtActionProc) arm},
    { "Disarm", (XtActionProc) disarm},
    { "RotateUp", (XtActionProc) rotate_up},
    { "RotateDown", (XtActionProc) rotate_down},
    { "RotateLeft", (XtActionProc) rotate_left},
    { "RotateRight", (XtActionProc) rotate_right},
};

static XmPartResource resources[] = {
    {
	CrysNViewMatrix,
	CrysCViewMatrix,
	CrysRMatrix,
	sizeof(float *),
	XtOffsetOf( struct _CrystalRec, crystal.view_matrix),
	XmRPointer, (XtPointer) NULL},
    {
	XmNarmCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffsetOf( struct _CrystalRec, crystal.arm_callback),
	XmRPointer, (XtPointer) NULL},
    {
	XmNdisarmCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffsetOf( struct _CrystalRec, crystal.disarm_callback),
	XmRPointer, (XtPointer) NULL},
    {
	XmNdragCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffsetOf( struct _CrystalRec, crystal.drag_callback),
	XmRPointer, (XtPointer) NULL},
    {
	XmNrepeatDelay,
	XmCRepeatDelay,
	XmRInt,
	sizeof(int),
	XtOffsetOf( struct _CrystalRec, crystal.repeat_delay),
	XmRImmediate, (XtPointer) 300},
    {
	CrysNAllowThrowing,
	XmCBoolean,
	XmRBoolean,
	sizeof(Boolean),
	XtOffsetOf( struct _CrystalRec, crystal.allow_throw),
	XmRImmediate, (XtPointer) True},
    {
	XmNvalueChangedCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffsetOf( struct _CrystalRec, crystal.value_changed_callback),
	XmRPointer, (XtPointer) NULL},
    {
	XmNexposeCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffsetOf( struct _CrystalRec, crystal.expose_callback),
	XmRPointer, (XtPointer) NULL},
    {
	XmNresizeCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffsetOf( struct _CrystalRec, crystal.resize_callback),
	XmRPointer, (XtPointer) NULL}
};
 
CrystalClassRec crystalClassRec = {
    {                                   /* core_class fields    */
    (WidgetClass) &xmPrimitiveClassRec, /* superclass           */
    "Crystal",                          /* class_name           */
    sizeof(CrystalPart),                /* widget_size          */
    ClassInitialize,                    /* class_initialize     */
    NULL,                               /* class_part_initialize*/
    False,                              /* class_inited         */
    Initialize,                         /* initialize           */
    NULL,                               /* initialize_notify    */
    XtInheritRealize,                   /* realize              */
    actionsList,                        /* actions              */
    XtNumber(actionsList),              /* num_actions          */
    (XtResourceList)resources,          /* resources            */
    XtNumber(resources),                /* num_resources        */
    NULLQUARK,                          /* xrm_class            */
    True,                               /* compress_motion      */
    True,                               /* compress_exposure    */
    True,                               /* compress_enterleave  */
    False,                              /* visible_interest     */
    Destroy,                            /* destroy              */
    Resize,                             /* resize               */
    Redisplay,                          /* expose               */
    SetValues,                          /* set_values           */
    NULL,                               /* set_values_hook      */
    XtInheritSetValuesAlmost,           /* set_values_almost    */
    NULL,                               /* get_values_hook      */
    NULL,                               /* accept_focus         */
    XtVersionDontCheck,                 /* version              */
    NULL,                               /* callback_private     */
    defaultTranslations,                /* tm_table             */
    QueryGeometry,                      /* query_geometry       */
    NULL,                               /* disp accelerator     */
    NULL                                /* extension            */
    },
    {                                   /* primitive_class record */
    _XtInherit,                         /* border_highlight     */
    _XtInherit,                         /* border_unhighlight   */
    XtInheritTranslations,              /* translations         */
    NULL,                               /* arm_and_activate     */
    NULL,                               /* syn resources        */
    0,                                  /* num syn_resources    */
    NULL,                               /* extension            */
    },
    {                                   /* crystal_class record */
    NULL,                               /* */
    }
};

externaldef(crystalwidgetclass) WidgetClass crystalWidgetClass =
    (WidgetClass) &crystalClassRec;
 
static XmOffsetPtr offsets; /* Part Offset table for XmResolvePartOffsets */

int CrystalMrmInitialize()
{
    return(MrmRegisterClass (MrmwcUnknown, "Crystal" , "CrystalCreate",
			     CrystalCreate,
			     (WidgetClass)&crystalClassRec));
}

Widget CrystalCreate(parent, name, arglist, nargs)
    Widget parent;
    char *name;
    Arg *arglist;
    int nargs;
{
    return(XtCreateWidget (name, crystalWidgetClass, parent, arglist, nargs));
}

static void ClassInitialize()
{
    XmResolvePartOffsets(crystalWidgetClass, &offsets);
}

static void Initialize(request, new)
    CrystalWidget request;
    CrystalWidget new;
{
    Resize(new);
}

static void Resize(w)
    CrystalWidget w;
{
    XtCallCallbacks((Widget)w, XmNresizeCallback, NULL);
}

static XtGeometryResult QueryGeometry (w, intended, reply)
    CrystalWidget w;
    XtWidgetGeometry *intended;
    XtWidgetGeometry *reply;
{
    reply->request_mode = 0;
 
    if (intended->request_mode & (~(CWWidth | CWHeight)) != 0)
        return (XtGeometryNo);
 
    reply->request_mode = 0;
    return (XtGeometryYes);
}

static void Destroy(w)
    CrystalWidget w;
{
}
 
static void Redisplay(w, event, region)
    CrystalWidget w;
    XEvent *event;
    Region region;
{
    if (XtIsRealized(w)) {
        if (ShadowThickness(w) > 0)
            _XmDrawShadow (XtDisplay (w), XtWindow (w),
			   TopShadowGC(w), BottomShadowGC(w),
			   ShadowThickness(w),
			   HighlightThickness(w), HighlightThickness(w),
			   Width(w) - 2 * HighlightThickness(w),
			   Height(w) - 2 * HighlightThickness(w));
        if (Highlighted(w))
            _XmHighlightBorder ((Widget)w);
        else if (_XmDifferentBackground ((Widget)w, XtParent ((Widget)w)))
            _XmUnhighlightBorder ((Widget)w);
    }
    XtCallCallbacks((Widget)w, XmNexposeCallback, NULL);
}

static Boolean SetValues(current, request, new)
    CrystalWidget current;
    CrystalWidget request;
    CrystalWidget new;
 
{
    Boolean redraw = False;
    return(redraw);
}

/* Actions */

static void rotate_up(w, event, params, num_params)
    CrystalWidget w;
    XEvent *event;
    char **params;
    Cardinal *num_params;
{
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
    printf("up\n");
}

static void rotate_down(w, event, params, num_params)
    CrystalWidget w;
    XEvent *event;
    char **params;
    Cardinal *num_params;
{
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
    printf("down\n");
}

static void rotate_left(w, event, params, num_params)
    CrystalWidget w;
    XEvent *event;
    char **params;
    Cardinal *num_params;
{
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
    printf("left\n");
}

static void rotate_right(w, event, params, num_params)
    CrystalWidget w;
    XEvent *event;
    char **params;
    Cardinal *num_params;
{
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
    printf("right\n");
}

static void arm(w, event, params, num_params)
    CrystalWidget w;
    XEvent *event;
    char **params;
    Cardinal *num_params;
{
    w->crystal.mousedown.x = event->xbutton.x;
    w->crystal.mousedown.y = event->xbutton.y;
    w->crystal.mousedown.maxx = Width(w);
    w->crystal.mousedown.maxy = Height(w);
    
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
    XtCallCallbacks((Widget)w, XmNarmCallback, event);
}

static void disarm(w, event, params, num_params)
    CrystalWidget w;
    XEvent *event;
    char **params;
    Cardinal *num_params;
{
    float *trans, *view, default_view[16] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0};
    MousePosition mouseup;
    mouseup.x = event->xbutton.x;
    mouseup.y = event->xbutton.y;
    mouseup.maxx = Width(w);
    mouseup.maxy = Height(w);

    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
    XtCallCallbacks((Widget)w, XmNdisarmCallback, event);
    if (! (view = ViewMatrix(w)))
	view = default_view;
    trans = crystal_ball(view, w->crystal.mousedown, mouseup);
    XtCallCallbacks((Widget)w, XmNvalueChangedCallback, trans);
    free(trans);
}
