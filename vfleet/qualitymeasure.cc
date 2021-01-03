/****************************************************************************
 * qualitymeasure.cc
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"

QualityMeasure::QualityMeasure( const QualityMeasure& other )
{
  opacity_limit= other.opacity_limit;
  color_comp_error= other.color_comp_error;
  opacity_min= other.opacity_min;
}

QualityMeasure& QualityMeasure::operator=( const QualityMeasure& other )
{
  if (this != &other) {
    opacity_limit= other.opacity_limit;
    color_comp_error= other.color_comp_error;
    opacity_min= other.opacity_min;
  }
  return( *this );
}

void QualityMeasure::add_max_deviation( gBColor& clr, gBColor& ref_clr,
					const QualityMeasure& qual )
{
  // Color component handling
  int r_err= clr.ir() > ref_clr.ir() ? 
    clr.ir() - ref_clr.ir() : ref_clr.ir() - clr.ir();
  int g_err= clr.ig() > ref_clr.ig() ? 
    clr.ig() - ref_clr.ig() : ref_clr.ig() - clr.ig();
  int b_err= clr.ib() > ref_clr.ib() ? 
    clr.ib() - ref_clr.ib() : ref_clr.ib() - clr.ib();
  int a_err= clr.ia() > ref_clr.ia() ? 
    clr.ia() - ref_clr.ia() : ref_clr.ia() - clr.ia();
  int max_comp_err= r_err;
  if (g_err > max_comp_err) max_comp_err= g_err;
  if (b_err > max_comp_err) max_comp_err= b_err;
  if (a_err > max_comp_err) max_comp_err= a_err;
  max_comp_err += qual.color_comp_error;
  if (max_comp_err > 255) max_comp_err= 255;
  color_comp_error= ( color_comp_error < max_comp_err ) ? 
    max_comp_err : color_comp_error;
}

