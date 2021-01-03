/****************************************************************************
 * netrgbimage.h
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

class netRgbImage: public baseNetComm, public rgbImage {
public:
  netRgbImage(int xin, int yin) : rgbImage(xin,yin) {}
  netRgbImage( const rgbImage& other ) 
    : rgbImage( other.xsize(), other.ysize(), other.buf ) {}
  netRgbImage( const int xin, const int yin, 
	      const int n_comp_pixels_in, gBColor* comp_pixels_in,
	      const int n_comp_runs_in, int* comp_runs_in,
	      const int owns_compbuf_in= 1 )
    : rgbImage( xin, yin, n_comp_pixels_in, comp_pixels_in,
	       n_comp_runs_in, comp_runs_in, owns_compbuf_in ) {}
  netRgbImage( const int xin, const int yin, 
	      const int n_comp_pixels_in, unsigned char* comp_pixels_in,
	      const int n_comp_runs_in, int* comp_runs_in,
	      const int owns_compbuf_in= 1 )
    : rgbImage( xin, yin, n_comp_pixels_in, comp_pixels_in,
	       n_comp_runs_in, comp_runs_in, owns_compbuf_in ) {}
  virtual ~netRgbImage() {};
  void netputself();
  static netRgbImage *netget();
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  void shmem_send_self_info( long* shmem_landing_area, int partner_pe );
  static netRgbImage* get_from_shmem( const long* shmem_landing_area );
#endif
protected:
  static void netputparts( netRgbImage *source );
  static void netgetparts( netRgbImage **result );
  static int run_table_length;
  static int* run_table;
  static int pix_table_length;
  static unsigned char* pix_table;
};

