/****************************************************************************
 * imagehandler.h
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

class baseImageHandler { 
public:
  baseImageHandler();
  virtual ~baseImageHandler();
  virtual void display( rgbImage *image , short refresh=1)= 0;
  rgbImage *last_image() {return(current_image);};
protected:
  rgbImage *current_image;
};

class fileImageHandler : public baseImageHandler {
public:
  fileImageHandler( char *fname_in, char *format_in );
  ~fileImageHandler();
  void display( rgbImage *image, short refresh=1);
protected:
  char *fname;
  char *format;
};
