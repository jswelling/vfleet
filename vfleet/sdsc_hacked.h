/**
 **	$Header: /home/welling/cvsroot/vfleet/sdsc_hacked.h,v 1.2 2003/11/22 06:35:07 welling Exp $
 **	Copyright (c) 1989-1992  San Diego Supercomputer Center (SDSC)
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
 **	sdsc.h	-  SDSC projects standard include file
 **
 **  PROJECT
 **	All SDSC C projects
 **
 **  PUBLIC CONTENTS
 **			d =defined constant
 **			f =function
 **			m =defined macro
 **			t =typedef/struct/union
 **			v =variable
 **			? =other
 **
 **	__SDSCH__	d  File inclusion flag
 **
 **	copyright	v  Copyright message
 **
 **	void		t  returns nothing
 **	int8		t  8-bit integer
 **	uint8		t  8-bit unsigned integer
 **	int16		t  16-bit integer
 **	uint16		t  16-bit unsigned integer
 **	int32		t  32-bit integer
 **	uint32		t  32-bit unsigned integer
 **	int64		t  64-bit integer
 **	uint64		t  64-bit unsigned integer
 **	boolean		t  logical
 **	pointer		t  generic pointer
 **	uchar		t  unsigned char
 **	ushort		t  unsigned short
 **	uint		t  unsigned int
 **	ulong		t  unsigned long
 **
 **	private		?  local to file
 **	public		?  global to project
 **
 **	TRUE		d  boolean true
 **	FALSE		d  boolean false
 **	YES		d  boolean true
 **	NO		d  boolean false
 **
 **  PRIVATE CONTENTS
 **	none
 **
 **  HISTORY
 **	$Log: sdsc_hacked.h,v $
 **	Revision 1.2  2003/11/22 06:35:07  welling
 **	Many mods to get things to compile under the new Make system.
 **	
 * Revision 1.1  1992/12/09  21:39:36  welling
 * Initial revision
 *
 **	Revision 1.3  92/09/02  15:15:03  vle
 **	Updated copyright notice.
 **	
 **	Revision 1.2  91/01/09  16:54:21  nadeau
 **	Updated copyright.
 **	
 **	Revision 1.1  90/06/22  12:16:21  nadeau
 **	Initial revision
 **	
 */

#ifndef __SDSCH__
#define __SDSCH__

#include "sdscconfig.h"				/* Machine/OS config flags */


/*
 *  Standard Copyright Notice:
 *	A copyright notice must appear as clear text in all binaries.
 *
 *	HEADER is #defined by the standard C file comment header to be the RCS
 *	header string for the file.  Placement of this string in the copyright
 *	message is sufficient to associate the copyright with the file.
 *
 *	Because lint doesn't like variables that are declared but not
 *	used in the file, we surround the lot with #ifndef lint.
 */

#ifndef lint
static char *copyright[] =
{
"------------------------------------------------------------------------",
#ifdef HEADER
HEADER,
#endif /* HEADER */
"    Copyright (c) 1989-1992  San Diego Supercomputer Center (SDSC), CA, USA",
"------------------------------------------------------------------------"
};
#endif /* lint */



/*
 *  Storage Classes:
 *	Public and private are used to mark functions and globals in a
 *	file that are to be or not to be accessible from other files.
 *
 *	Public should be #defined to 'auto', but some C compilers don't
 *	recognize the keyword.  The default is 'auto' anyway.
 */

#define sdsc_private		static
#define sdsc_public



/*
 *  Standard Constants:
 */

#ifndef	FALSE
#define	FALSE		(0)
#endif /* FALSE */

#ifndef	TRUE
#define	TRUE		(!FALSE)
#endif /* TRUE */

#ifndef NO
#define NO		(0)
#endif /* NO */

#ifndef YES
#define YES		(!NO)
#endif /* YES */



/*
 *  Standard types:
 */

#if CHAR_SIZE >= 8
#define __8	char
#else
#if SHORT_SIZE >= 8
#define __8	short
#else
#if INT_SIZE >= 8
#define __8	int
#else
#if LONG_SIZE >= 8
#define __8	long
#endif
#endif
#endif
#endif

#if CHAR_SIZE >= 16
#define __16	char
#else
#if SHORT_SIZE >= 16
#define __16	short
#else
#if INT_SIZE >= 16
#define __16	int
#else
#if LONG_SIZE >= 16
#define __16	long
#endif
#endif
#endif
#endif

#if CHAR_SIZE >= 32
#define __32	char
#else
#if SHORT_SIZE >= 32
#define __32	short
#else
#if INT_SIZE >= 32
#define __32	int
#else
#if LONG_SIZE >= 32
#define __32	long
#endif
#endif
#endif
#endif

#if CHAR_SIZE >= 64
#define __64	char
#else
#if SHORT_SIZE >= 64
#define __64	short
#else
#if INT_SIZE >= 64
#define __64	int
#else
#if LONG_SIZE >= 64
#define __64	long
#endif
#endif
#endif
#endif

#ifdef __8
typedef __8		int8;
typedef unsigned __8	uint8;
#undef __8
#endif /* __8 */
#ifdef __16
typedef __16		int16;
typedef unsigned __16	uint16;
#undef __16
#endif /* __16 */
#ifdef __32
typedef __32		int32;
typedef unsigned __32	uint32;
#undef __32
#endif /* __32 */
#ifdef __64
typedef __64		int64;
typedef unsigned __64	uint64;
#undef __64
#endif /* __64 */

#ifndef VOID
typedef int		void;
#endif /* VOID */

typedef int *		pointer;

/*
 *  We'd like to use typedef's here, but other include files, such as
 *  <sys/types.h> may have been included first and have already made
 *  typedef's for these.  We can't check for the inclusion of <sys/types.h>
 *  reliably because different OS variants use different inclusion flags,
 *  if any.  Instead we use #define's and avoid the collision problems
 *  alltogether.
 */
#define boolean		int

#define uchar		unsigned char
#define ushort		unsigned short
#define uint		unsigned int
#define ulong		unsigned long


#include "bin.h"
#include "arg.h"
#include "tag.h"

#endif /* __SDSCH__ */
