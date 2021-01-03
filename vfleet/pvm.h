/****************************************************************************
 * pvm.h for pvm 2.4
 * Author Joel Welling, Adam Beguelin
 * Copyright 1992, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/*
 * This file provides definitions for PVM functions.
 */

/*********/
/* types */
/*********/
typedef int nint;
typedef float nfloat;
typedef double ndfloat;
typedef struct ncplx_strct {
  float r;
  float i;
} ncplx;
typedef struct ndcplx_strct {
  double r;
  double i;
} ndcplx;

/***************/
/* Definitions */
/***************/

#ifdef __cplusplus
//	Error return codes from libpvm calls

const int	PvmOk		=	0;	/* okay */
const int	PvmSyserr	=	-1;	/* some system error */
const int	PvmBadParam	=	-2;	/* bad parameter (neg msg id, etc) */
const int	PvmMismatch	=	-3;	/* barrier count mismatch */
const int	PvmTooLong	=	-4;	/* name too long (32) */
const int	PvmNoData	=	-5;	/* read past end of buffer */
const int	PvmNoHost	=	-6;	/* no such host */
const int	PvmNoFile	=	-7;	/* no such executable */
const int	PvmNoComp	=	-8;	/* no such process component */
const int	PvmNoMsg	=	-9;	/* no message available */
const int	PvmNoMem	=	-10;	/* can't get memory */

// Misc assumptions
const int 	PvmMaxUserProcNameLen =  32;

#else /* ifdef __cplusplus */

#define	PvmOk			0	/* okay */
#define	PvmSyserr		-1	/* some system error */
#define	PvmBadParam		-2	/* bad parameter (neg msg id, etc) */
#define	PvmMismatch		-3	/* barrier count mismatch */
#define	PvmTooLong		-4	/* name too long (32) */
#define	PvmNoData		-5	/* read past end of buffer */
#define	PvmNoHost		-6	/* no such host */
#define	PvmNoFile		-7	/* no such executable */
#define	PvmNoComp		-8	/* no such process component */
#define	PvmNoMsg		-9	/* no message available */
#define	PvmNoMem		-10	/* can't get memory */

/* Misc assumptions */
#define PvmMaxUserProcNameLen   32

#endif /* ifdef __cplusplus */

/*************/
/* Functions */
/*************/
#ifdef __cplusplus

extern "C" int barrier( char *barrier_name, int num );

extern "C" int enroll( const char *component_name );

extern "C" int getnint( nint *x, int num );
extern "C" int getnfloat( nfloat *x, int num );
extern "C" int getndfloat( ndfloat *x, int num );
extern "C" int getncplx( ncplx *x, int num );
extern "C" int getndcplx( ndcplx *x, int num );
extern "C" int getstring( char *x );
extern "C" int getbytes( char *x, int num );
extern "C" int getnshort( short *x, int num );
extern "C" int getnlong( long *x, int num );

extern "C" int initiate( const char *object_file, const char *arch );

extern "C" int initiatem( char *object_file, char *machine );

extern "C" int initsend();

extern "C" int leave();

extern "C" int probe( int msgtype );

extern "C" int probemulti( int num, int *msgtypes );

extern "C" int pstatus( int *ncpu, int *nformats );

extern "C" int putnint( const nint *ptr, int num );
extern "C" int putnfloat( const nfloat *ptr, int num );
extern "C" int putndfloat( const ndfloat *ptr, int num );
extern "C" int putncplx( const ncplx *ptr, int num );
extern "C" int putndcplx( const ndcplx *ptr, int num );
extern "C" int putstring( const char *ptr );
extern "C" int putbytes( const char *ptr, int num );
extern "C" int putnshort( const short *ptr, int num );
extern "C" int putnlong( const long *ptr, int num );

extern "C" int rcv( int msgtype );

extern "C" int rcvinfo( int *bytes, int *msgtype, char *component, 
		       int *instance );

extern "C" int rcvmulti( int num, int *msgtypes );

extern "C" int ready( const char *event_name );

extern "C" int snd( const char *component, int instance, int msgtype );

extern "C" int status( const char *component, int instance );

extern "C" int terminate( const char *component, int instance );

extern "C" int waituntil( const char *event_name );

extern "C" int whoami( char *component, int *instance );

#else /* ifdef __cplusplus */

extern int barrier( char *barrier_name, int num );

extern int enroll( char *component_name );

extern int getnint( nint *x, int num );
extern int getnfloat( nfloat *x, int num );
extern int getndfloat( ndfloat *x, int num );
extern int getncplx( ncplx *x, int num );
extern int getndcplx( ndcplx *x, int num );
extern int getstring( char *x );
extern int getbytes( char *x, int num );
extern int getnshort( short *x, int num );
extern int getnlong( long *x, int num );

extern int initiate( char *object_file, char *arch );

extern int initiatem( char *object_file, char *machine );

extern int initsend();

extern int leave();

extern int probe( int msgtype );

extern int probemulti( int num, int *msgtypes );

extern int pstatus( int *ncpu, int *nformats );

extern int putnint( nint *ptr, int num );
extern int putnfloat( nfloat *ptr, int num );
extern int putndfloat( ndfloat *ptr, int num );
extern int putncplx( ncplx *ptr, int num );
extern int putndcplx( ndcplx *ptr, int num );
extern int putstring( char *ptr );
extern int putbytes( char *ptr, int num );
extern int putnshort( short *ptr, int num );
extern int putnlong( long *ptr, int num );

extern int rcv( int msgtype );

extern int rcvinfo( int *bytes, int *msgtype, char *component, 
		   int *instance );

extern int rcvmulti( int num, int *msgtypes );

extern int ready( char *event_name );

extern int snd( char *component, int instance, int msgtype );

extern int status( char *component, int instance, int msgtype );

extern int terminate( char *component, int instance );

extern int waituntil( char *event_name );

extern int whoami( char *component, int *instance );

#endif /* ifdef __cplusplus */
