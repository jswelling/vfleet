/**
 **	$Header: /home/welling/cvsroot/vfleet/netv.h,v 1.2 2003/11/22 06:35:07 welling Exp $
 **	Copyright (c) 1992  San Diego Supercomputer Center (SDSC)
 **		San Diego, California, USA
 **
 **	Users and possessors of this source code are hereby granted a
 **	nonexclusive, royalty-free copyright and design patent license to
 **	use this code in individual software.  License is not granted for
 **	commercial resale, in whole or in part, without prior written
 **	permission from SDSC.  This source is provided "AS IS" without express
 **	or implied warranty of any kind.
 **
 **	For further information contact:
 **		E-Mail:		info@sds.sdsc.edu
 **
 **		Surface Mail:	Information Center
 **				San Diego Supercomputer Center
 **				P.O. Box 85608
 **				San Diego, CA  92138-5608
 **				(619) 534-5000
 **/

/**
 **  FILE
 **	netv.h		-  Network volume visualization header file
 **
 **  PROJECT
 **	NETV		-  Network volume visualization
 **
 **  DESCRIPTION
 **	Macros and shared stuff
 **
 **  PUBLIC CONTENTS
 **			d =defined constant
 **			f =function
 **			m =defined macro
 **			t =typedef/struct/union
 **			v =variable
 **			? =other
 **
 **	__NETVH__		d  File inclusion flag
 **
 **	NetVErrNo		v  error number
 **	NETVE...		d  error codes
 **
 **	NetV*			f  function types
 **
 **  PRIVATE CONTENTS
 **	none
 **
 **  HISTORY
 **	$Log: netv.h,v $
 **	Revision 1.2  2003/11/22 06:35:07  welling
 **	Many mods to get things to compile under the new Make system.
 **	
 * Revision 1.1  1992/12/09  21:39:36  welling
 * Initial revision
 *
 **	
 **/

#ifndef __NETVH__
#define __NETVH__

#include "sdsc.h"

/*
 *  GLOBAL VARIABLE
 *	NetVErrNo	-  error number
 *	NetVNErr	-  number of error messages
 *	NetVErrList	-  error messages
 *
 *  DESCRIPTION
 *	On an error, the NETV package routines return -1 and sets
 *	NetVErrno to an error code.  The programmer may call NetVPError
 *	to print the associated error message to stderr, or may do the
 *	message lookup in NetVErrList themselves.
 */
extern int NetVErrNo;			/* Current error number		*/
extern int NetVNErr;			/* Number of error messages	*/
extern char *NetVErrList[];		/* List of error messages	*/


/*
 *  CONSTANTS
 *	NETVE...		-  error codes
 *	NETVNULL	Null string
 *	NETV*		Var types for netv calls
 *
 *  DESCRIPTION
 */

#define NETVNULL	 	(0)	/* NULL pointer			*/

#define NETVENOERROR	  	0	/* Status okay 			*/
#define	NETVESYS		1	/* System call error; see errno */
#define NETVECLOSE		2	/* Connection closed		*/
#define NETVEEOL		3	/* End of line			*/
#define NETVEMALLOC		4	/* Cannot allocate memory	*/
#define NETVEUNKHOST		5	/* Unknown host			*/
#define NETVECOMM		6	/* Communications error		*/
#define NETVEWRITEFILE		7	/* Cannot write file		*/
/* 8 ... 99 reserved */

#define NETVNERROR		8

#define NETVSTRING		STRING	/* String argument type		*/
#define NETVINT			INT	/* Int argument type		*/
#define NETVSHORT		SHORT	/* Int argument type		*/
#define NETVFLOAT		FLOAT	/* Float argument type		*/
#define NETVBYTE		SHORT+FLOAT  /* Int argument type	*/

#define NETVERR			1000 	/* For printing routines	*/
#define NETVDBG			1001 	/* For printing routines	*/
/* 1002 ... reserved */

#define NETVGREY  		(2)	/* Insist on an 8 bit image file*/
#define NETVINDEX8  		(1)	/* Insist on an 8 bit image file*/
#define NETVANY	 		(0)	/* Any depth of image file okay */

#define NETVPARALLEL	0		/* Avolve specific		*/
#define NETVPERSPECTIVE	1		/* Avolve specific		*/

#define NETVSTRSIZE		(256)	/* Maximum string size this pkg	*/

					/* Use this mail program	*/
#define NETVMAILPROG		"/usr/ucb/mail"

					/* Nil string			*/
#define NETVNILSTR		"nil\0"

					/* Default e-mail host		*/
#define NETVDEFMAILHOST		"sds.sdsc.edu\0"


/*
 *  TYPEDEF & STRUCT
 *
 *  DESCRIPTION
 *	Vec3d		A three component vector
 *	Point3d		A point in 3-space
 *	Extent		A sub-volume of the dataset
 *	Rgb		An rgb triple
 *	Window		A screen space portion of the view
 *	View		Viewing parameters
 *	InfLight	An light source at infinity
 *	PointLight	A positioned light source
 *	LightSource	An array of light sources
 *	Animate		Information needed for a cine loop
 *	NetVParms	All of the above in one neat structure
 *
 */

typedef struct _Vec3d {
    	float 	xdir;
    	float 	ydir;
    	float 	zdir;
} Vec3d;

typedef struct _Vec4d {
    	float 	xdir;
    	float 	ydir;
    	float 	zdir;
    	float 	mag;
} Vec4d;

typedef struct _Point3d {
    	float 	x;
    	float 	y;
    	float 	z;
} Point3d;

typedef struct _Point4d {
    	float 	x;
    	float 	y;
    	float 	z;
    	float 	w;
} Point4d;

typedef struct _Node3d {
    	int 	i;
    	int 	j;
    	int 	k;
} Node3d;


typedef float Mat4x4[4][4];


typedef struct _Extent {
	int	minx;
	int	miny;
	int 	minz;
	int	maxx;
	int	maxy;
	int	maxz;
} Extent;

typedef struct _Color
{
	uchar	red;
	uchar	grn;
	uchar	blu;
} Color;

#define NETVRANGES	(7)
#define NETVCLASSES	(NETVRANGES+1)

typedef struct NetVParms
{
        Node3d  nv_dims;                /* Dimensionality of the data   */
        Vec3d   nv_scaleDims;           /* Make the volume isotropic    */
	Extent  nv_extents;		/* Extents			*/

        float 	nv_gradMagThresh;	/* Opacity threshold 0-1        */
        int	nv_imageRes;		/* 100x100 200x200 or 400x400   */
        boolean	nv_progRef;		/* Progressive refinement ?     */

        Vec3d 	nv_volRot;         	/* Volume viewing rotation  	*/

        Point3d	nv_posPtLt;             /* Position of the point light  */
        float	nv_brtPtLt;             /* Brightness of the point light*/
        float	nv_brtAmbLt;            /* Brightness of ambinent light */

	int	nv_frames;		/* Animation count. 1 frame is 1*/
	int	nv_axis;		/* Axis of rotation, X, Y, or Z	*/

        uchar  	nv_range[NETVCLASSES];  /* Ranges, eg. 0-23  23-255 	*/
        float  	nv_opacity[NETVRANGES]; /* Actual opacity values        */
        Color  	nv_color[NETVRANGES];   /* Actual data colors	        */
        Color 	nv_backColor;         	/* Background color        	*/

	char	*nv_user;		/* Name of user       		*/
	char	*nv_clientHost;		/* Name of client host		*/
	int	*nv_display;		/* Number of display device     */
	char	*nv_qftDest;		/* Destination for QFT transfer */
	char	*nv_viewType;		/* Parallel or perspective      */
	char	*nv_cameraAnimate;	/* Parallel or perspective      */
} NetVParms;


typedef struct _View
{
	int	type;			/* 0:parallel, 1:perspective	*/
	Point3d vRef;			/* Center of view		*/
	Vec3d	vNorm;			/* Viewing plane normal		*/
	Vec3d	vUp;			/* View up			*/
	float	wind[4];		/* Size of the window		*/
	float	eyeOff;			/* Eye offset			*/
} View;


typedef struct _NetVRSM
{
	char	  *nv_host;
	char	  *nv_serviceID;
	boolean	   nv_interactive;
	int	   nv_maxVolSize;
} NetVRSM;
	

/*
 *  MACRO 
 *	NETV*	Dirty bits
 *
 *  DESCRIPTION
 *	These are used by the NetV UCM and the RSM to indicate that the user
 *	has changed the value of a field.  If the field has been touched, the
 *	dirty bit is set so that the UCM will transmit the new value of the
 *	field to the RSM when the user pushes the GO render button.  The
 *	RSM uses these bits to keep track of what calculations must be 
 *	performed before rendering.
 */
#define NETVDBDIMS		(0x00000001)
#define NETVDBSCALEDIMS		(0x00000002)
#define NETVDBEXTENTS		(0x00000004)
#define NETVDBGRADMAGTHRESH	(0x00000008)
#define NETVDBIMAGERES		(0x00000010)
#define NETVDBPROGREF		(0x00000020)
#define NETVDBVOLROT		(0x00000040)
#define NETVDBPOSPTLT		(0x00000080)
#define NETVDBBRTPTLT		(0x00000100)
#define NETVDBBRTAMBLT		(0x00000200)
#define NETVDBFRAMES		(0x00000400)
#define NETVDBAXIS		(0x00000800)
#define NETVDBCLASSIFICATION	(0x00001000)
#define NETVDBBACKCOLOR		(0x00002000)
#define NETVDBUSER		(0x00004000)
#define NETVDBCLIENTHOST	(0x00008000)
#define NETVDBDISPLAY		(0x00010000)
#define NETVDBQFTDEST		(0x00020000)
#define NETVDBVIEWTYPE		(0x00040000)
#define NETVDBCAMERAANIMATE	(0x00080000)

/*
 *  MACRO 
 *	NETV*	Renderer actions as requested by client.
 *
 *  DESCRIPTION
 *	These are the actions that the RSMs take when they receive a message
 *	from UCMs.  These correspond to RNP message classes.
 *
 */
#define NETVCLASSSTARTRENDER    (1)
#define NETVCLASSABORTRENDER	(2)
#define NETVCLASSSETPARAMS	(3)
#define NETVCLASSSSETVOLUME	(4)
/* 5..99 reserved */


/*
 *  MACRO 
 *	NETV*	Type of RSM to use
 *
 *  DESCRIPTION
 *	In the future users will have a choice of serveral different types
 *	of volume rendering.
 *
 */
#define NETVSPLAT       	(0x0001)
#define NETVRAYCAST		(0x0002)
#define NETVINTERACTIVE		(0x0004)
#define NETVBATCH		(0x0008)
#define NETVUSEPVM		(0x0010)

/*
 *  MACRO 
 *	NETV*	Image resolution
 *
 *  DESCRIPTION
 * 	The field nv_imageRes can take on one of these 3 values.
 *
 */
#define NETVONEHUNDRED		(0)	/* 100x100 resolution image	*/
#define NETVTWOHUNDRED		(1)	/* 200x200 resolution image	*/
#define NETVFOURHUNDRED		(2)	/* 400x400 resolution image	*/
/* 3..99 reserved */

/*
 *  MACRO 
 *	NETV*	Rotation axis
 *
 *  DESCRIPTION
 * 	The field nv_axis can take on one of these 3 values.
 *
 */
#define NETVXAXIS		(0)
#define NETVYAXIS		(1)
#define NETVZAXIS		(2)
/* 3..99 reserved */

/*
 *  MACRO 
 *	NETV*	Alternative displays
 *
 *  DESCRIPTION
 * 	Users will have the choice of different displays.  Usually the images
 *	will be displayed on the client host.  Alternatively the images
 *	could be displayed on a high-speed frame buffer such as the SDSC
 *	HIPPI frame buffer, assuming the RSM host is on the HIPPI network.
 *
 */
#define NETVDISPLAYONCLIENT	(0)
#define NETVDISPLAYONHIPPI	(1)
/* 2..99 reserved */

/*
 *  FUNCTION
 *	NetV*	-  function types
 */

extern char *	NetVQError( );
extern char *	NetVQErrorByNum( );
extern void 	NetVPrint();

#endif /* __NETVH__ */
