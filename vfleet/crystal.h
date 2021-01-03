externalref WidgetClass crystalWidgetClass;

#ifdef __cplusplus
extern "C" {
#endif

#define CrysNViewMatrix "ViewMatrix"
#define CrysCViewMatrix "ViewMatrix"
#define CrysRMatrix     "Matrix"

#define CrysNAllowThrowing "AllowThrowing"

typedef struct _CrystalClassRec *CrystalWidgetClass;
typedef struct _CrystalRec *CrystalWidget;

#define IsCrystal(w) XtIsSubclass((w), crystalWidgetClass)

extern Widget CrystalCreate();
extern int CrystalMrmInitialize();

#ifdef __cplusplus
}
#endif
