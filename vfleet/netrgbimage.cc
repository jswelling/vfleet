/****************************************************************************
 * netrgbimage.cc
 * Authors Joel Welling, Rob Earhart
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
#include "basenet.h"
#include "rgbimage.h"
#include "netrgbimage.h"

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include <mpp/shmem.h>
#endif

int netRgbImage::run_table_length= 0;
int* netRgbImage::run_table= NULL;
int netRgbImage::pix_table_length= 0;
unsigned char* netRgbImage::pix_table= NULL;

void netRgbImage::netputself()
{
  netputparts( this );
}

netRgbImage *netRgbImage::netget()
{
  netRgbImage *result= NULL;
  netgetparts( &result );

  return( result );
}

void netRgbImage::netputparts( netRgbImage *source )
{
  int nruns= 0;
  int tot_black= 0;
  int npix= 0;
  int black_run= 0;
  int clr_run= 0;
  int i;
  gBColor transparent_black; // initializes properly

  gBColor thispix= source->pix( 0, 0 );
  for (i=0; i<source->xsize()*source->ysize()-1; i++) {
    if (thispix == transparent_black) {
      if (!black_run) {
	black_run= 1;
	clr_run= 0;
	nruns++;
      }
      tot_black++;
    }
    else {
      if (!clr_run) {
	black_run= 0;
	clr_run= 1;
	nruns++;
      }
      npix++;
    }
    thispix= source->nextpix();
  }
  if (thispix == transparent_black) {
    if (!black_run) nruns++;
    tot_black++;
  }
  else {
    if (!clr_run) nruns++;
    npix++;
  }

  netputnint( &(source->xdim), 1 );
  netputnint( &(source->ydim), 1 );
  int im_valid= source->image_valid;
  netputnint( &im_valid, 1 );
  netputnint( &npix, 1 );
  netputnint( &nruns, 1 );

  // Build and pack the run length table.  Positive number n means
  // n non-black pixels;  negative number n means -n black pixels
  if (run_table_length < nruns) {
    delete [] run_table;
    run_table_length= 2*nruns;   // allocate extra to avoid reallocation
    run_table= new int[run_table_length];
  }
  int* table_runner= run_table;
  int black_run_length= 0;
  int clr_run_length= 0;
  thispix= source->pix( 0, 0 );
  for (i=0; i<(source->xsize()*source->ysize())-1; i++) {
    if (thispix == transparent_black) {
      black_run_length++;
      if (clr_run_length) {
	*table_runner++= clr_run_length;
	clr_run_length= 0;
      }
    }
    else {
      clr_run_length++;
      if (black_run_length) {
	*table_runner++= -black_run_length;
	black_run_length= 0;
      }
    }
    thispix= source->nextpix();
  }
  // handle last pixel
  if (thispix == transparent_black) {
    black_run_length++;
    if (clr_run_length) {
      *table_runner++= clr_run_length;
      clr_run_length= 0;
    }
  }
  else {
    clr_run_length++;
    if (black_run_length) {
      *table_runner++= -black_run_length;
      black_run_length= 0;
    }
  }
  if (black_run_length) *table_runner++= -black_run_length;
  if (clr_run_length) *table_runner++= clr_run_length;
  netputnint( run_table, nruns );

  // Put the image data in the buffer
  if (pix_table_length < 4*npix) {
    delete [] pix_table;
    pix_table_length= 8*npix;
    pix_table= new unsigned char[ pix_table_length ];
  }
  unsigned char* pixrunner= pix_table;
  unsigned char* pixptr= (unsigned char*)(ImVfbQPtr(source->buf,0,0));
  table_runner= run_table;
  for (table_runner= run_table; table_runner < run_table+nruns;
       table_runner++) {
    if (*table_runner>0) {
      for (int i=0; i < 4 * *table_runner; i++) *pixrunner++= *pixptr++;
    }
    else pixptr += -4 * *table_runner;
  }
  netputbytes( (char*)pix_table, 4*npix );
}

void netRgbImage::netgetparts( netRgbImage **result )
{
  if (*result) {
    fprintf(stderr,"netRgbImage::netgetparts: called with non-null param!\n");
    exit(-1);
  }

  int im_xdim;
  int im_ydim;
  int im_valid;
  int npix;
  int nruns;
  netgetnint( &im_xdim, 1 );
  netgetnint( &im_ydim, 1 );
  netgetnint( &im_valid, 1 );  
  netgetnint( &npix, 1 );
  netgetnint( &nruns, 1 );

  if (!im_valid) {
    *result= new netRgbImage( 0, 0, 0, (gBColor*)NULL, 0, NULL );
    (*result)->image_valid= 0;
    return;
  }

  int* runs= new int[ nruns ];
  gBColor* pixels= new gBColor[ npix ];

  // Get the image data from the buffer
  netgetnint( runs, nruns );
  netgetbytes( (char*)(pixels->bytes()), 4*npix );
  // Unfortunately *some* pathetic compilers make sizeof(gBColor) > 4
  if (sizeof(gBColor) > 4) {
    // Stretch the image data out; buffer is large enough
    unsigned char* byterunner= (unsigned char*)pixels + 4*(npix-1);
    for (gBColor* crunner= pixels + npix - 1; crunner >= pixels; crunner--) {
      *crunner= gBColor( byterunner );
      byterunner -= 4;
    }
  }

  *result= new netRgbImage( im_xdim, im_ydim, 
			   npix, pixels, nruns, runs );
}

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
// This routine shoves info appropriate to this image down the given
// Cray PVM Channel
void netRgbImage::shmem_send_self_info( long* shmem_landing_addr, 
				       int partner_pe )
{
  gBColor transparent_black; // initializes properly

  int start_offset= 0;
  gBColor thispix= pix(0,0);
  while (thispix==transparent_black) {
    start_offset++;
    thispix= nextpix();
  }
  // Now one might think that the thing to do to find the other end
  // would be to start at the tail of the image and back up, but the
  // cache line prefetch algorithm on the T3D means that this is
  // inefficient.  Better, sadly enough, to run on up through memory
  // until we find the last colored pixel.
  int end_offset= 0;
  int limit= xsize()*ysize();
  for (int i=start_offset; i<limit; i++) {
    if (thispix!=transparent_black) end_offset= i;
    thispix= nextpix();
  }

  // Send the information
  static long shmem_send_buffer[7]; // must match size in netImageHandler
  extern int _MPP_MY_PE;
  shmem_send_buffer[0]= 1; // flag to indicate new message
  shmem_send_buffer[1]= _MPP_MY_PE; // processor
  shmem_send_buffer[2]= xsize();
  shmem_send_buffer[3]= ysize();
  shmem_send_buffer[4]= (long)ImVfbQPtr(buf,0,0);
  shmem_send_buffer[5]= start_offset;
  shmem_send_buffer[6]= end_offset;
  shmem_put(shmem_landing_addr, shmem_send_buffer, 7, partner_pe);
}

netRgbImage* netRgbImage::get_from_shmem( const long* shmem_landing_area )
{
  int partner_pe= (int)shmem_landing_area[1];
  int xdim_in= (int)shmem_landing_area[2];
  int ydim_in= (int)shmem_landing_area[3];
  short* partner_pix_buf= (short*)shmem_landing_area[4];
  int start_offset= (int)shmem_landing_area[5];
  int end_offset= (int)shmem_landing_area[6];

  int n_runs;
  int* comp_runs;
  int n_pixels= end_offset - start_offset + 1;

  // Pull the incoming pixels into a special aligned block, so that the
  // compositing routine will benefit from cache management trickery
  unsigned char* byte_comp_pixels= 
    rgbImage::get_matched_memory(xdim_in,ydim_in) + 4*start_offset;

  if (start_offset==0) {
    n_runs= 2;
    comp_runs= new int[n_runs];
    comp_runs[0]= n_pixels;
    comp_runs[1]= -((xdim_in * ydim_in) - n_pixels);
  }
  else {
    n_runs= 3;
    comp_runs= new int[n_runs];
    comp_runs[0]= -start_offset;
    comp_runs[1]= n_pixels;
    comp_runs[2]= -(((xdim_in * ydim_in) - n_pixels) - start_offset);
  }

  shmem_get32( (short*)byte_comp_pixels, partner_pix_buf + start_offset, 
	      n_pixels, partner_pe);

  // Result image is created with flag unset to prevent deletion of
  // the aligned memory block when result is deleted- that block is
  // managed elsewhere.
  netRgbImage* result= new netRgbImage( xdim_in, ydim_in, 
				       n_pixels, byte_comp_pixels,
				       n_runs, comp_runs, 0 );

  return result;
}
#endif

