/****************************************************************************
 * image.h
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
#include <im.h>

class rgbPixel {
// range= 0 - 255 for all int values, 0.0 - 1.0 for floats
public:
  rgbPixel( int r, int g, int b )
    { rval= r; gval= g; bval= b; }
  void set_r( int value ) { rval= value; }
  void set_g( int value ) { gval= value; }
  void set_b( int value ) { bval= value; }
  int r() const { return( rval ); } 
  int g() const { return( gval ); }
  int b() const { return( bval ); }
  float fr() const { return( (float)rval/255.0 ); }
  float fg() const { return( (float)gval/255.0 ); }
  float fb() const { return( (float)bval/255.0 ); }
private:
  unsigned char rval;
  unsigned char gval;
  unsigned char bval;
};

class rgbaPixel {
// range= 0 - 255 for all int values, 0.0 - 1.0 for floats
public:
  rgbaPixel( int r, int g, int b, int a )
    { rval= r; gval= g; bval= b; aval= a; }
  void set_r( int value ) { rval= value; }
  void set_g( int value ) { gval= value; }
  void set_b( int value ) { bval= value; }
  void set_a( int value ) { aval= value; }
  int r() const { return( rval ); }
  int g() const { return( gval ); }
  int b() const { return( bval ); }
  int a() const { return( aval ); }
  float fr() const { return( (float)rval/255.0 ); }
  float fg() const { return( (float)gval/255.0 ); }
  float fb() const { return( (float)bval/255.0 ); }
  float fa() const { return( (float)aval/255.0 ); }
private:
  unsigned char rval;
  unsigned char gval;
  unsigned char bval;
  unsigned char aval;
};

class rgbImage;

class baseImage: public baseNetComm {
  friend rgbImage;
public:
  baseImage(int xin, int yin);
  virtual ~baseImage() {}
  int xsize() const { return xdim; }
  int ysize() const { return ydim; }
  int valid() const { return image_valid; };
  virtual void netputself();
  static baseImage *netget();
  int image_is_valid() {return(image_valid);};
protected:
  static void netputparts( baseImage *source );
  static void netgetparts( baseImage *result );
  int xdim;
  int ydim;
  int image_valid;
};

class rgbImage: public baseImage {
public:
  rgbImage(int xdim, int ydim);
  rgbImage( FILE *fp, char *filename );
  rgbImage( baseImage &prototype ); // copy constructor
  virtual ~rgbImage();
  void clear(rgbPixel pix);
  rgbPixel pix( const int i, const int j ) const
  {
    ImVfbPtr p= ImVfbQPtr( buf, i, j );
    return( rgbPixel( ImVfbQRed( buf, p ),
		     ImVfbQGreen( buf, p ),
		     ImVfbQBlue( buf, p ) ) );
  }
  void setpix( const int i, const int j, const rgbPixel& pix )
  {
    ImVfbPtr p= ImVfbQPtr( buf, i, j );
    ImVfbSRed( buf, p, pix.r() );
    ImVfbSGreen( buf, p, pix.g() );
    ImVfbSBlue( buf, p, pix.b() );
  }
  void netputself(); // Must be distinct, to use proper netputparts.
  static rgbImage *netget();
private:
  static void netputparts( rgbImage *source );
  static void netgetparts( rgbImage *result );
  ImVfb *buf;
};

class rgbaImage: public baseImage {
public:
  rgbaImage(int xdim, int ydim);
  rgbaImage( FILE *fp, char *filename );
  virtual ~rgbaImage();
  void clear(rgbaPixel pix);
  rgbaPixel pix( int i, int j ) const;
private:
  ImVfb *buf;
};

