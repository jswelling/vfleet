/****************************************************************************
 * rgbaimage.cc
 * Author Joel Welling
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
#include "image.h"
#include "im.h"

rgbaImage::rgbaImage( int xdim, int ydim )
: baseImage(xdim, ydim)
{
  buf= ImVfbAlloc( xdim, ydim, (IMVFBRGB | IMVFBALPHA) );
}

rgbaImage::rgbaImage( FILE *fp, char *fname )
: baseImage( 1, 1 ) // Set correct dimensions later
{
  // Allocate tag tables and add error stream
  TagTable *dataTable= TagTableAlloc();
  if (!dataTable) {
    image_valid= 0;
    return;
  }
  TagTable *flagsTable= TagTableAlloc();
  if (!dataTable || !flagsTable) {
    image_valid= 0;
    return;
  }
  FILE *tmp= stderr;
  if ( TagTableAppend( flagsTable,
		       TagEntryAlloc("error stream", POINTER, &tmp))
       == -1 ) {
    image_valid= 0;
    return;
  }

  // Get the input file format
  char *type= ImFileQFFormat( fp, fname );
  fprintf(stderr,"Got type <%s>\n",type);
  if (!type) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }

  // Read the file
  if ( ImFileFRead( fp, type, flagsTable, dataTable ) == -1 ) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }

  // Get the image
  TagEntry *imageEntry= TagTableQDirect( dataTable, "image vfb", 0 );
  if (!imageEntry) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }
  if (TagEntryQValue( imageEntry, &buf ) == -1) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }

  // Convert to RGB if necessary
  if ( !(ImVfbQFields(buf)) & IMVFBRGB ) {
    if ( !ImVfbToRgb(buf, buf) ) {
      image_valid= 0;
      TagTableFree( dataTable );
      TagTableFree( flagsTable );
      return;
    }
  }

  xdim= ImVfbQWidth( buf );
  ydim= ImVfbQHeight( buf );
  fprintf(stderr,"got %d by %d\n",xdim,ydim);

  // Clean up
  TagTableFree( dataTable );
  TagTableFree( flagsTable );
}

rgbaImage::~rgbaImage()
{
  ImVfbFree( buf );
}

void rgbaImage::clear( rgbaPixel pix )
{
  ImVfbPtr p, p1, p2;

  p1= ImVfbQPtr( buf, 0, 0 );
  p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
  for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
    ImVfbSRed( buf, p, pix.r() );
    ImVfbSGreen( buf, p, pix.g() );
    ImVfbSBlue( buf, p, pix.b() );
    ImVfbSAlpha( buf, p, pix.a() );
  }
}

rgbaPixel rgbaImage::pix( int i, int j )
{
  ImVfbPtr p= ImVfbQPtr( buf, i, j );
  return( rgbaPixel pxl( ImVfbQRed( buf, p ),
			 ImVfbQGreen( buf, p ),
			 ImVfbQBlue( buf, p ),
			 ImVfbQAlpha( buf, p ) ) );
}
