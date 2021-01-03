/*CrystalP.h : private header file for the Crystal class.*/

#define CrystalIndex (XmPrimitiveIndex + 1)

typedef struct _CrystalClassPart {
    int reserved;
} CrystalClassPart;

typedef struct _CrystalClassRec {
    CoreClassPart core_class;
    XmPrimitiveClassPart primitive_class;
    CrystalClassPart crystal_class;
} CrystalClassRec;

extern CrystalClassRec crystalClassRec;

typedef struct _CrystalPart {
    XtCallbackList arm_callback;
    XtCallbackList disarm_callback;
    XtCallbackList drag_callback;
    XtCallbackList value_changed_callback;
    XtCallbackList expose_callback;
    XtCallbackList resize_callback;
    MousePosition mousedown;		/* where the current drag began */
    float *view_matrix;			/* The current viewing matrix */
    int repeat_delay;			/* minimum time between redraws */
    Boolean allow_throw;		/* Do we let the user throw the ball?*/
} CrystalPart;

typedef struct _CrystalRec {
    CorePart core;
    XmPrimitivePart primitive;
    CrystalPart crystal;
} CrystalRec;

