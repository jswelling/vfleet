/****************************************************************************
 * lightinfo.cc
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

// Values used in computing specular lighting component.  Cutoff should
// be such that pow( specular_cos_cutoff, specular_exponent ) < 1/255.
static const float specular_exponent= 20.0;
static const float specular_cos_cutoff= 0.75; 
static const float specular_weight= 2.0;
static const int specular_curve_length= 256;

// Hook for the precomputed specular lighting curve;
float* LightInfo::specular_curve= NULL;

// Fast lighting table will be accessed using the first 5 digits of each
// gradient component, which sets the table size.
gBColor* LightInfo::fast_lighting_table= new gBColor[32*32*32];
gBColor* LightInfo::fast_specular_table= new gBColor[32*32*32];

// Initialize fast lighting info to an invalid value- all black and null dir
LightInfo LightInfo::fast_lighting_info= LightInfo( gColor(), gBVector(), 
						    gColor() );

LightInfo::LightInfo( const gColor& ltcolor_in, const gBVector& ltdir_in,
		      const gColor& ambclr_in )
{
  // Build the specular lighting curve if necessary
  if (!specular_curve) {
    specular_curve= new float[specular_curve_length];
    float cos_factor_step= 1.0/(float)(specular_curve_length-1);
    float cos_factor= 0.0;
    for (int i=0; i<specular_curve_length; i++) {
      if (cos_factor > 1.0) cos_factor= 1.0;
      specular_curve[i]= specular_weight*pow(cos_factor, specular_exponent);
      cos_factor += cos_factor_step;
    }
  }

  // Opacities must be 1.0 or color bounding errors can result on
  // accumulation of voxel colors.
  ltcolor= gColor(ltcolor_in.r(),ltcolor_in.g(),ltcolor_in.b(),1.0);
  ltdir= ltdir_in;
  ltdir.normalize();
  ambclr= gColor(ambclr_in.r(),ambclr_in.g(),ambclr_in.b(),1.0);
}

LightInfo::LightInfo( const LightInfo& other )
{
  ltcolor= other.ltcolor;
  ltdir= other.ltdir;
  ltdir.normalize();
  ambclr= other.ambclr;
  central_viewdir= other.central_viewdir;
}

LightInfo::~LightInfo() 
{
  // Nothing to delete
}

LightInfo& LightInfo::operator=( const LightInfo& other )
{
  if (this != &other ) {
    ltcolor= other.ltcolor;
    ltdir= other.ltdir;
    ambclr= other.ambclr;
    central_viewdir= other.central_viewdir;
  }
  return( *this );
}

void LightInfo::reset_lighting_tables()
{
  if ( *this != fast_lighting_info ) {
    gBColor clear_black;
    for (int i=0; i<32*32*32; i++) {
      fast_lighting_table[i]= clear_black;
      fast_specular_table[i]= clear_black;
    }
    fast_lighting_info= *this;
  }
}

gColor LightInfo::calc_light_clr( const gBVector neg_normal, const int debug )
{
  gVector r_neg_normal= neg_normal;
  r_neg_normal *= (1/1.7320508); // scale max norm to 1
  gVector r_dir= dir();
  float cos_factor= r_dir * r_neg_normal;
  cos_factor= cos_factor >= 0.0 ? cos_factor : 0.0;
  gColor light_clr= amb() + clr()*cos_factor;
  light_clr.clamp();
  if (debug) {
    fprintf(stderr,
  "diffuse light: dir= (%5.2f %5.2f %5.2f), neg normal= (%5.2f %5.2f %5.2f)\n",
	    dir().x(),dir().y(),dir().z(),
	    r_neg_normal.x(),r_neg_normal.y(),r_neg_normal.z());
    fprintf(stderr,
      "               cos factor %f, light_clr= ( %5.2f %5.2f %5.2f %5.2f )\n",
	    cos_factor,
	    light_clr.r(),light_clr.g(),light_clr.b(),light_clr.a());
  }
  return light_clr;
}

gColor LightInfo::calc_specular_clr( const gBVector neg_normal, 
				    const int debug )
{
  if ((neg_normal.ix()==0) && (neg_normal.iy()==0) && (neg_normal.iz()==0)) {
    // no gradient to light!
    return gColor(); // transparent black
  }
  gVector r_neg_normal= neg_normal; // convert to floats
  gVector r_dir= dir();
  if (r_neg_normal * r_dir < 0.0) // backface cull check
    return gColor(); // transparent black
  float norm_mag= r_neg_normal.length();
  r_neg_normal *= (1.0/norm_mag); // normalize
  norm_mag *= (1.0/1.7320508); // scale max norm to 1
  gVector neg_reflection_dir= r_dir
      -  ( r_neg_normal * (2.0*(r_dir*r_neg_normal)) ); // still normalized
  float cos_factor= -1.0*(viewdir * neg_reflection_dir);
  gColor spec_clr; // initializes to transparent black
  if (cos_factor > specular_cos_cutoff) {
    int index= (int)((specular_curve_length-1)*cos_factor);
    spec_clr= ltcolor * (norm_mag * specular_curve[index]);
  }
  if (debug) {
    fprintf(stderr,
 "specular light: dir= (%5.2f %5.2f %5.2f), neg normal= (%5.2f %5.2f %5.2f)\n",
	    dir().x(), dir().y(), dir().z(),
	    r_neg_normal.x(), r_neg_normal.y(), r_neg_normal.z());
    fprintf(stderr,
	 "                norm_mag= %f, neg_refl_dir= (%5.2f %5.2f %5.2f)\n",
	    norm_mag,
	    neg_reflection_dir.x(),neg_reflection_dir.y(),
	    neg_reflection_dir.z());
    fprintf(stderr,
	    "                viewdir= (%5.2f %5.2f %5.2f)\n",
	    viewdir.x(),viewdir.y(),viewdir.z());
    fprintf(stderr,
      "                cos_factor= %f, spec_clr= (%5.2f %5.2f %5.2f %5.2f)\n",
	    cos_factor,
	    spec_clr.r(), spec_clr.g(), spec_clr.b(), spec_clr.a());

  }

  return spec_clr;
}

gColor LightInfo::calc_specular_clr_central_dir( const gBVector neg_normal )
{
  if ((neg_normal.ix()==0) && (neg_normal.iy()==0) && (neg_normal.iz()==0)) {
    // no gradient to light!
    return gColor();
  }
  gVector r_neg_normal= neg_normal; // convert to floats
  gVector r_dir= dir();
  if (r_neg_normal * r_dir < 0.0) // backface cull check
    return gColor();
  float norm_mag= r_neg_normal.length();
  r_neg_normal *= (1.0/norm_mag); // normalize
  norm_mag *= (1.0/1.7320508); // scale max norm to 1
  gVector neg_reflection_dir= r_dir
      -  ( r_neg_normal * (2.0*(r_dir*r_neg_normal)) ); // still normalized
  float cos_factor= -1.0*(central_viewdir * neg_reflection_dir);
  gColor spec_clr; // initializes to transparent black
  if (cos_factor > specular_cos_cutoff) {
    int index= (int)((specular_curve_length-1)*cos_factor);
    spec_clr= ltcolor * (norm_mag * specular_curve[index]);
  }
  return spec_clr;
}

