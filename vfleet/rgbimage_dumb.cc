/****************************************************************************
 * rgbimage_dumb.cc
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
#include "rgbimage.h"

// This module provides much of the functionality of rgbImage, but without
// the use of the SDSC ImageTools library libim.a .  Tiff file structure
// is based on the TIFF 6.0 specification, 1992, Aldus.  For a copy of
// this spec, call Aldus Corp. at \(206\)628-6593.

float* rgbImage::alpha_dif_table= NULL;

rgbImage::rgbImage( int xin, int yin, ImVfb *initial_buf )
{
    if (!alpha_dif_table) {
      alpha_dif_table= new float[256];
      for (int i=0; i<256; i++)
	alpha_dif_table[i]= 1.0 - ((float)i/255.0);
    }

    xdim= xin;
    ydim= yin;
    if (!initial_buf) {
      buf= new ImVfb;
      buf->vfb_width= xdim;
      buf->vfb_height= ydim;
      buf->vfb_fields= IMVFBRGB | IMVFBALPHA;
      buf->vfb_nbytes= 4;
      buf->vfb_clt= IMCLTNULL;
      buf->vfb_roff= 0;
      buf->vfb_goff= 1;
      buf->vfb_boff= 2;
      buf->vfb_aoff= 3;
      buf->vfb_i8off= 0;
      buf->vfb_wpoff= 0;
      buf->vfb_i16off= 0;
      buf->vfb_zoff= 0;
      buf->vfb_moff= 0;
      buf->vfb_fpoff= 0;
      buf->vfb_ioff= 0;
      buf->vfb_pfirst= 
	new unsigned char[buf->vfb_nbytes * buf->vfb_width * buf->vfb_height];
      buf->vfb_plast= buf->vfb_pfirst 
	+ buf->vfb_nbytes * buf->vfb_width * buf->vfb_height;
      owns_buffer= 1;
    }
    else {
      buf= initial_buf;
      owns_buffer= 0;
    }
    image_valid= 1;
    compressed_flag= 0;
    n_comp_pixels= n_comp_runs= 0;
    comp_pixels= NULL;
    comp_runs= NULL;
}

rgbImage::rgbImage( FILE *fp, char *fname )
{
  if (!alpha_dif_table) {
    alpha_dif_table= new float[256];
    for (int i=0; i<256; i++)
      alpha_dif_table[i]= 1.0 - ((float)i/255.0);
  }

  fprintf(stderr,"rgbImage::rgbImage: linking error;  this is the dumb version of rgbImage;  it can't read the file <%s>!\n",
	  fname);
  exit(-1);
}

rgbImage::rgbImage( const int xin, const int yin,
		   const int n_comp_pixels_in, gBColor* comp_pixels_in,
		   const int n_comp_runs_in, int* comp_runs_in, 
		    const int owns_compbuf_in)
{
    if (!alpha_dif_table) {
      alpha_dif_table= new float[256];
      for (int i=0; i<256; i++)
	alpha_dif_table[i]= 1.0 - ((float)i/255.0);
    }

    xdim= xin;
    ydim= yin;
    buf= NULL;
    current_pix= NULL;
    compressed_flag= 1;
    image_valid= 1;
    owns_buffer= 0;

    n_comp_pixels= n_comp_pixels_in;
    n_comp_runs= n_comp_runs_in;
    comp_pixels= comp_pixels_in;  // *not* a copy of the buffer!
    comp_runs= comp_runs_in;      // *not* a copy of the buffer!
}

rgbImage::~rgbImage()
{
  if (valid() && owns_buffer) {
    delete buf->vfb_pfirst;
    delete buf;
  }
  delete comp_pixels;
  delete comp_runs;
}

void rgbImage::clear( gBColor pix )
{
  if (compressed()) {
    for (gBColor* thispix= comp_pixels; thispix < comp_pixels + n_comp_pixels;
	 thispix++) *thispix= pix;
  }
  else {
    ImVfbPtr p, p1, p2;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      ImVfbSRed( buf, p, pix.ir() );
      ImVfbSGreen( buf, p, pix.ig() );
      ImVfbSBlue( buf, p, pix.ib() );
      ImVfbSAlpha( buf, p, pix.ia() );
    }
  }
}

void rgbImage::add_over( rgbImage *other )
// Note that this method assumes premultiplication by alpha!
{
  float alphadif;

  if (compressed()) uncompress();

  if (other->compressed()) {
    register ImVfbPtr p= ImVfbQPtr( buf, 0, 0 );
    gBColor transparent_black; // initializes properly
    register gBColor* src_pixel= other->comp_pixels;
    register int i;
    for (int* src_runs= other->comp_runs; 
	 src_runs < other->comp_runs + other->n_comp_runs; 
	 src_runs++) {
      if (*src_runs<0) {
	p= (ImVfbPtr)((char*)p + -4 * (*src_runs));
      }
      else {
	for (i=0; i<*src_runs; i++) {
	  if ((src_pixel->ia() == 255) || (ImVfbQAlpha(buf,p) == 0)) { 
	    ImVfbSRed( buf, p, src_pixel->ir() );
	    ImVfbSGreen( buf, p, src_pixel->ig() );
	    ImVfbSBlue( buf, p, src_pixel->ib() );
	    ImVfbSAlpha( buf, p, src_pixel->ia() );
	  }
	  // No black source pixels in compressed images
	  else {
	    alphadif= alpha_dif_table[src_pixel->ia()];
	    ImVfbSRed( buf, p, 
		      (int)(src_pixel->ir()+alphadif*ImVfbQRed(buf,p)) );
	    ImVfbSGreen( buf, p, 
			(int)(src_pixel->ig()+alphadif*ImVfbQGreen(buf,p)) );
	    ImVfbSBlue( buf, p, 
		       (int)(src_pixel->ib()+alphadif*ImVfbQBlue(buf,p)) );
	    ImVfbSAlpha( buf, p, 
			(int)(src_pixel->ia()+alphadif*ImVfbQAlpha(buf,p)) );
	  }
	  ImVfbSInc( buf, p );
	  src_pixel++;
	}
      }
    }
  }
  else {
    register ImVfbPtr p;
    ImVfbPtr p1, p2;
    register ImVfbPtr p_other;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    p_other= ImVfbQPtr( other->buf, 0, 0 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      if ((ImVfbQAlpha(other->buf,p_other) == 255) 
	  || (ImVfbQAlpha(buf,p) == 0)) {
	ImVfbSRed( buf, p, ImVfbQRed( other->buf, p_other ) );
	ImVfbSGreen( buf, p, ImVfbQGreen( other->buf, p_other ) );
	ImVfbSBlue( buf, p, ImVfbQBlue( other->buf, p_other ) );
	ImVfbSAlpha( buf, p, ImVfbQAlpha( other->buf, p_other ) );
      }
      else if ( ImVfbQAlpha(other->buf,p_other) == 0 ) {
	// Do nothing;  pixel is correct
      }
      else {
	alphadif= alpha_dif_table[ ImVfbQAlpha( other->buf, p ) ];
	ImVfbSRed( buf, p, (int)(ImVfbQRed(other->buf,p_other)
				 + alphadif*ImVfbQRed(buf,p)) );
	ImVfbSGreen( buf, p, (int)(ImVfbQGreen(other->buf,p_other)
				   + alphadif*ImVfbQGreen(buf,p)) );
	ImVfbSBlue( buf, p, (int)(ImVfbQBlue(other->buf,p_other)
				  + alphadif*ImVfbQBlue(buf,p)) );
	ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(other->buf,p_other)
				   + alphadif*ImVfbQAlpha(buf,p)) );
      }
      ImVfbSInc( other->buf, p_other );
    }
  }
}

void rgbImage::add_under( rgbImage *other )
// Note that this method assumes premultiplication by alpha!
{
  float alphadif;

  if (compressed()) uncompress();

  if (other->compressed()) {
    register ImVfbPtr p= ImVfbQPtr( buf, 0, 0 );
    gBColor transparent_black; // initializes properly
    register gBColor* src_pixel= other->comp_pixels;
    register int i;
    for (int* src_runs= other->comp_runs; 
	 src_runs < other->comp_runs + other->n_comp_runs; 
	 src_runs++) {
      if (*src_runs<0) {
	p= (ImVfbPtr)((char*)p + -4 * (*src_runs));
      }
      else {
	for (i=0; i<*src_runs; i++) {
	  // No black source pixels in compressed images
	  if ( ImVfbQAlpha(buf,p) == 255 ) {
	    // Pixel is correct; do nothing
	  }
	  else if ( ImVfbQAlpha(buf,p) == 0 ) {
	    ImVfbSRed( buf, p, src_pixel->ir() );
	    ImVfbSGreen( buf, p, src_pixel->ig() );
	    ImVfbSBlue( buf, p, src_pixel->ib() );
	    ImVfbSAlpha( buf, p, src_pixel->ia() );
	  }
	  else {
	    alphadif= alpha_dif_table[ ImVfbQAlpha( buf, p ) ];
	    ImVfbSRed( buf, p, (int)(ImVfbQRed(buf,p)
				     + alphadif*src_pixel->ir()) );
	    ImVfbSGreen( buf, p, (int)(ImVfbQGreen(buf,p)
				       + alphadif*src_pixel->ig()) );
	    ImVfbSBlue( buf, p, (int)(ImVfbQBlue(buf,p)
				      + alphadif*src_pixel->ib()) );
	    ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(buf,p)
				       + alphadif*src_pixel->ia()) );
	  }
	  ImVfbSInc( buf, p );
	  src_pixel++;
	}
      }
    }
  }
  else {
    register ImVfbPtr p;
    ImVfbPtr p1, p2;
    register ImVfbPtr p_other;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    p_other= ImVfbQPtr( other->buf, 0, 0 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      if ((ImVfbQAlpha(buf,p) == 255) 
	  || (ImVfbQAlpha(other->buf, p_other) == 0)) {
	// Pixel is correct;  do nothing
      }
      else if ( ImVfbQAlpha(buf,p) == 0 ) {
	ImVfbSRed( buf, p, ImVfbQRed(other->buf, p_other) );
	ImVfbSGreen( buf, p, ImVfbQGreen(other->buf, p_other) );
	ImVfbSBlue( buf, p, ImVfbQBlue(other->buf, p_other) );
	ImVfbSAlpha( buf, p, ImVfbQAlpha(other->buf, p_other) );
      }
      else {
	alphadif= alpha_dif_table[ ImVfbQAlpha( buf, p ) ];
	ImVfbSRed( buf, p, (int)(ImVfbQRed(buf,p)
				 + alphadif*ImVfbQRed(other->buf,p_other)));
	ImVfbSGreen( buf, p, (int)(ImVfbQGreen(buf,p)
				 + alphadif*ImVfbQGreen(other->buf,p_other)));
	ImVfbSBlue( buf, p, (int)(ImVfbQBlue(buf,p)
				 + alphadif*ImVfbQBlue(other->buf,p_other)));
	ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(buf,p)
				 + alphadif*ImVfbQAlpha(other->buf,p_other)));
      }
      ImVfbSInc( other->buf, p_other );
    }
  }
}

void rgbImage::rescale_by_alpha()
// Note that this method assumes premultiplication by alpha!  The purpose
// of this routine is to undo that premultiplication.
{
  float inv_alpha;

  if (compressed()) {
    for (gBColor* thispix= comp_pixels; thispix<comp_pixels+n_comp_pixels;
	 thispix++) {
      if ((thispix->ia() != 255) && (thispix->ia() != 0)) {
	inv_alpha= 255.0/((float)thispix->ia());
	*thispix= gBColor( bounds_check((int)(inv_alpha * thispix->ir())),
			  bounds_check((int)(inv_alpha * thispix->ig())),
			  bounds_check((int)(inv_alpha * thispix->ib())),
			  255 );
      }
    }
  }
  else {
    ImVfbPtr p, p1, p2;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      if ((ImVfbQAlpha( buf, p ) != 255) && (ImVfbQAlpha( buf, p ) != 0)) {
	inv_alpha= 255.0/((float)ImVfbQAlpha( buf, p ));
	ImVfbSRed( buf, p, bounds_check((int)(inv_alpha*ImVfbQRed(buf,p))) );
	ImVfbSGreen( buf, p, 
		    bounds_check((int)(inv_alpha*ImVfbQGreen(buf,p))) );
	ImVfbSBlue( buf, p, bounds_check((int)(inv_alpha*ImVfbQBlue(buf,p))) );
	ImVfbSAlpha( buf, p, 255 );
      }
    }
  }
}

static int write_int16(FILE *ofile, const unsigned int val)
{
  if (putc(val/256, ofile) == EOF) return 0;
  if (putc(val % 256, ofile) == EOF) return 0;
  return 1;
}

static int write_int32(FILE *ofile, const unsigned int val)
{
  unsigned int tval= val;
  if (putc(tval/(256*256*256), ofile) == EOF) return 0;
  tval= tval % (256*256*256);
  if (putc(val/(256*256), ofile) == EOF) return 0;
  tval= tval % (256*256);
  if (putc(tval/256, ofile) == EOF) return 0;
  if (putc(tval%256, ofile) == EOF) return 0;
  return 1;
}

static int write_rational(FILE *ofile,
			  const unsigned int numerator, 
			  const unsigned int denominator)
{
  if (!write_int32(ofile,numerator)) return 0;
  if (!write_int32(ofile,denominator)) return 0;
  return 1;
}

static int write_field(FILE *ofile, const int tag, const int type, 
		       const unsigned int value)
{
  if (!write_int16(ofile,tag)) return 0;
  if (!write_int16(ofile,type)) return 0;
  if (!write_int32(ofile, 1)) return 0;
  switch (type) {
  case 1: 
    { // byte
      if (putc(value,ofile) == EOF) return 0;
      for (int i=0; i<3; i++) if (putc(0,ofile) == EOF) return 0;
    }
    break;
  case 3: // short
    if (!write_int16(ofile,value)) return 0;
    if (!write_int16(ofile,0)) return 0;
    break;
  case 4: // long
    if (!write_int32(ofile,value)) return 0;
    break;
  default:
    fprintf(stderr,"TIFF writing error: write_field can't write type %d!\n",
	    type);
    return 0;
    break;
  }

  return 1;
}

static int write_field_offset(FILE *ofile, const int tag, const int type,
			     const int count, const int table_offset)
{
  if (!write_int16(ofile,tag)) return 0;
  if (!write_int16(ofile,type)) return 0;
  if (!write_int32(ofile, count)) return 0;
  if (!write_int32(ofile, table_offset)) return 0;
  return 1;
}

static int write_to_tiff(const char *fname, const int xdim, const int ydim,
			 ImVfb* image)
{
  FILE *ofile= fopen(fname,"w");
  if (ofile == NULL) {
    fprintf(stderr,"rgbImage::save: cannot open <%s> for writing!\n",fname);
    return(0);
  }

  int num_ifd_entries= 13;

  int bps_offset=
    2+2+4   // header
    +2      // number of directory entries
    +num_ifd_entries*12  // number of IFD entries * size of an entry
    +4;     // four bytes of zero after last IFD

  int xres_offset= bps_offset + 2*4;  // shift by bits per sample size
  int yres_offset= xres_offset + 2*4; // shift by xres_offset size

  int image_offset= yres_offset + 2*4; // shift by yres_offset size

  // Write the header

  if (!write_int16(ofile,0x4d4d)) return 0;     // byte order MM
  if (!write_int16(ofile,0x2a)) return 0;       // TIFF magic number
  if (!write_int32(ofile,0x8)) return 0;        // Offset of first IFD
  if (!write_int16(ofile,num_ifd_entries)) return 0; // IFD begins; num entries
  
  if (!write_field(ofile,0x100,4,xdim)) return 0; // width in long format
  if (!write_field(ofile,0x101,4,ydim)) return 0; // height in long format

  // bits per sample
  if (!write_field_offset(ofile, 0x102, 3, 4, bps_offset)) return 0;

  if (!write_field(ofile,0x103,3,1)) return 0;  // compression
  if (!write_field(ofile,0x106,3,2)) return 0;  // photometric interpretation

  // strip offsets
  if (!write_field(ofile,0x111,4,image_offset)) return 0; // One strip offset

  if (!write_field(ofile,0x115,3,4)) return 0;  // samples per pixel
  if (!write_field(ofile,0x116,3,ydim)) return 0;  // rows per strip

  // strip byte counts
  int strip_byte_count= 4*xdim*ydim;
  if (!write_field(ofile,0x117,4,strip_byte_count)) return 0; // one byte count

  // X and Y resolution
  if (!write_field_offset(ofile,0x11a,5,1,xres_offset)) return 0;
  if (!write_field_offset(ofile,0x11b,5,1,yres_offset)) return 0;
  if (!write_field(ofile,0x128, 3, 1)) return 0;        // ResolutionUnit
  if (!write_field(ofile,0x152, 3, 1)) return 0;        // ExtraSamples info

  if (!write_int32(ofile,0)) return 0; // close IFD

  // Write the needed array data

  // Bits per sample table
  for (int i=0; i<4; i++) {
    if (!write_int16(ofile,8)) return 0;
  }

  // X and Y resolution, in that order
  if (!write_rational(ofile,1,1)) return 0;
  if (!write_rational(ofile,1,1)) return 0;

  // Write the image
  ImVfbPtr p, p1, p2;
  p1= ImVfbQPtr( image, 0, 0 );
  p2= ImVfbQPtr( image, xdim-1, ydim-1 );
  for ( p=p1; p<=p2; ImVfbSInc(image,p) ) {
    if (putc(ImVfbQRed(image,p),ofile) == EOF) return 0;
    if (putc(ImVfbQGreen(image,p),ofile) == EOF) return 0;
    if (putc(ImVfbQBlue(image,p),ofile) == EOF) return 0;
    if (putc(ImVfbQAlpha(image,p),ofile) == EOF) return 0;
  }

  // Close up
  if (fclose(ofile) == EOF) {
    fprintf(stderr,"rgbImage::save: error closing file <%s>!\n",fname);
    return(0);
  }
    
  return(1);
}

static int write_to_ps( const char *fname, const int xdim, const int ydim,
			ImVfb* image )
{
  FILE *ofile= fopen(fname,"w");
  if (ofile == NULL) {
    fprintf(stderr,"rgbImage::save: cannot open <%s> for writing!\n",fname);
    return(0);
  }

  // Calculate Postscript transformation coefficients to place image
  // in center of a 7.5 inch by 10 inch region on page center.  Postscript
  // uses a coordinate system which is 72 points per inch.
  int xshift, yshift, xscale, yscale;
  float fxshift, fyshift, fxscale, fyscale;
  if ((xdim * 10.0) > (ydim * 7.5)) {
    // Touches X edges
    fxscale= 7.5*72;
    fxshift= 0.5*72;
    fyscale= (fxscale*ydim)/(float)xdim;
    fyshift= 5.5*72 - 0.5*fyscale;
  }
  else {
    // Touches Y edges
    fyscale= 10.0*72;
    fyshift= 0.5*72;
    fxscale= (fyscale*xdim)/(float)ydim;
    fxshift= 4.25*72 - 0.5*fxscale;
  }
  xscale= (int)fxscale;
  yscale= (int)fyscale;
  xshift= (int)fxshift;
  yshift= (int)fyshift;

  static char header_string[]=
    "%%!PS-Adobe-1.0 \n\
%%%%Creator: VFleet \n\
%%%%BoundingBox 0. 0. 432. 576. \n\
%%%%EndComments \n\
save gsave \n\
/str %d string def \n\
%d %d translate \n\
%d %d scale \n\
%d %d 8 [%d 0 0 %d 0 %d ] \n\
{currentfile str readhexstring pop} false 3 colorimage \n";

  static char trailer_string[]= "grestore restore showpage\n";

  fprintf(ofile,header_string,
	  xdim*3,
          xshift, yshift,
          xscale, yscale,
	  xdim, ydim, 
          xdim, -ydim, ydim);

  // Write the image
  ImVfbPtr p, p1, p2;
  p1= ImVfbQPtr( image, 0, 0 );
  p2= ImVfbQPtr( image, xdim-1, ydim-1 );
  int counter= 0;
  for ( p=p1; p<=p2; ImVfbSInc(image,p) ) {
    fprintf(ofile,"%02x%02x%02x",
	    ImVfbQRed(image,p),ImVfbQGreen(image,p),
	    ImVfbQBlue(image,p));
    counter += 6;
    if (counter>=72) {
      putc('\n',ofile);
      counter= 0;
    }
  }  

  fputs(trailer_string,ofile);

  // Close up
  if (fclose(ofile) == EOF) {
    fprintf(stderr,"rgbImage::save: error closing file <%s>!\n",fname);
    return(0);
  }
    
  return(1);
}

int rgbImage::save( char *fname, char *format )
{
  if (compressed()) uncompress();

  if (!strcmp(format,"tiff"))
    return write_to_tiff(fname, xsize(), ysize(), buf);
  else if (!strcmp(format,"ps"))
    return write_to_ps(fname, xsize(), ysize(), buf);
  else {
    fprintf(stderr,
 "rgbImage::save: only TIFF and PS formats supported; cannot handle %s!\n",
	    format);
    return(0);
  }
}

void rgbImage::uncompress()
{
  // Compressed images are stored as an array of pixels, and an array
  // of runs.  A positive number in the runs array means that that
  // number of pixels from the pixels array should be used.  A negative
  // number means that the absolute value of that number of black 
  // transparent pixels should be used.

  buf= new ImVfb;
  buf->vfb_width= xdim;
  buf->vfb_height= ydim;
  buf->vfb_fields= IMVFBRGB | IMVFBALPHA;
  buf->vfb_nbytes= 4;
  buf->vfb_clt= IMCLTNULL;
  buf->vfb_roff= 0;
  buf->vfb_goff= 1;
  buf->vfb_boff= 2;
  buf->vfb_aoff= 3;
  buf->vfb_i8off= 0;
  buf->vfb_wpoff= 0;
  buf->vfb_i16off= 0;
  buf->vfb_zoff= 0;
  buf->vfb_moff= 0;
  buf->vfb_fpoff= 0;
  buf->vfb_ioff= 0;
  buf->vfb_pfirst= 
    new unsigned char[buf->vfb_nbytes * buf->vfb_width * buf->vfb_height];
  buf->vfb_plast= buf->vfb_pfirst 
    + buf->vfb_nbytes * buf->vfb_width * buf->vfb_height;


  owns_buffer= 1;

  register ImVfbPtr p= ImVfbQPtr( buf, 0, 0 );
  register gBColor* thispix= comp_pixels;
  gBColor transparent_black; // initializes properly
  register int i;

  for (int* thisrun= comp_runs; thisrun<comp_runs+n_comp_runs; thisrun++) {
    if (*thisrun >= 0) {
      for (i=0; i<*thisrun; i++) {
	ImVfbSRed( buf, p, thispix->ir() );
	ImVfbSGreen( buf, p, thispix->ig() );
	ImVfbSBlue( buf, p, thispix->ib() );
	ImVfbSAlpha( buf, p, thispix->ia() );
	ImVfbSInc( buf, p );      
	thispix++;
      }
    }
    else {
      for (i=0; i<(-1*(*thisrun)); i++) {
	ImVfbSRed( buf, p, transparent_black.ir() );
	ImVfbSGreen( buf, p, transparent_black.ig() );
	ImVfbSBlue( buf, p, transparent_black.ib() );
	ImVfbSAlpha( buf, p, transparent_black.ia() );
	ImVfbSInc( buf, p );      
      }
    }
  }

  delete comp_pixels;
  comp_pixels= NULL;
  n_comp_pixels= 0;
  delete comp_runs;
  comp_runs= NULL;
  n_comp_runs= 0;
  compressed_flag= 0;
}

