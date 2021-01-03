/****************************************************************************
 * logimagehandler.cc
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
#include "rgbimage.h"
#include "imagehandler.h"
#include "logger.h"
#include "logimagehandler.h"

logImageHandler::logImageHandler( baseImageHandler *ihandler_in,
				 baseLogger *logger_in,
				 void (*display_cb_in)(baseImageHandler*))
{
  ihandler= ihandler_in;
  logger= logger_in;
  display_cb= display_cb_in;
}

void logImageHandler::display( rgbImage *image, short refresh )
{
  logger->comment("Image arriving...");
  ihandler->display(image, refresh);
  logger->comment("Image ready.");
  (*display_cb)(this);
}
