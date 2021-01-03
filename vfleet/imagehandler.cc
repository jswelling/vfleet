/****************************************************************************
 * imagehandler.cc
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
#include <string.h>
#include "rgbimage.h"
#include "imagehandler.h"

baseImageHandler::baseImageHandler()
{
  current_image= NULL;
}

baseImageHandler::~baseImageHandler()
{
}

fileImageHandler::fileImageHandler( char *fname_in, char *format_in )
{
  fname= new char[strlen(fname_in)+1];
  strcpy(fname,fname_in);
  format= new char[strlen(format_in)+1];
  strcpy(format, format_in);
}

fileImageHandler::~fileImageHandler()
{
  delete fname;
  delete format;
}

void fileImageHandler::display( rgbImage *image, short refresh )
{
  if (image->valid()) (void)image->save(fname,format);
}
