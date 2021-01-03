/****************************************************************************
 * baseimage.cc
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
#include <sdsc.h>
#include "netv.h"
#include "basenet.h"
#include "image.h"

baseImage::baseImage( int xin, int yin )
{
  xdim= xin;
  ydim= yin;
  image_valid= 1;
}

void baseImage::netputself()
{
  netputparts( this );
}

baseImage *baseImage::netget()
{
  baseImage *result= new baseImage( 0, 0 );
  netgetparts( result );
  return( result );
}

void baseImage::netputparts( baseImage *source )
{
  netputnint( &(source->xdim), 1 );
  netputnint( &(source->ydim), 1 );
  netputnint( &(source->image_valid), 1 );
}

void baseImage::netgetparts( baseImage *result )
{
  netgetnint( &(result->xdim), 1 );
  netgetnint( &(result->ydim), 1 );
  netgetnint( &(result->image_valid), 1 );  
}

