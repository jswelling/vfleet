/****************************************************************************
 * tinydraw.cc
 * Author Joel Welling
 * Copyright 1995, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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

#include <stdlib.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>

#include "camera.h"
#include "tinydraw.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "ximagehandler.h"
#include "xdrawih.h"
#include "sphere.h"

TDrawNode::~TDrawNode()
{
  switch (type) 
    {
    case POLY_TYPE: delete poly; break;
    case COMPOUND_TYPE: delete geom; break;
    }
}

TDrawGeom::~TDrawGeom()
{
  // Unref all the nodes
  while (nodelist.head()) nodelist.pop()->unref();
}

void TDrawGeom::add( Poly* poly_in )
{
  poly_in->mask= POLY_MASK(nx) | POLY_MASK(ny) | POLY_MASK(nz) 
    | POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw)
    | POLY_MASK(r) | POLY_MASK(g) | POLY_MASK(b);
  TDrawNode* new_node= new TDrawNode(poly_in);
  new_node->ref();
  nodelist.append( new_node );
}

void TDrawGeom::add( TDrawGeom* geom_in )
{
  TDrawNode* new_node= new TDrawNode(geom_in);
  new_node->ref();
  nodelist.append( new_node );
}

void TDrawGeom::add_sphere( const float r, const float g, const float b )
{
  Poly *poly;
  int ctbl= 0;
  for (int i=0; i<sphere_facets; i++) {
    poly= new Poly;
    poly->n= sphere_v_counts[i];
    for (int j=0; j<poly->n; j++) {
      int vtx= sphere_connect[ctbl];
      poly->vert[j].x= sphere_coords[vtx][0];
      poly->vert[j].y= sphere_coords[vtx][1];
      poly->vert[j].z= sphere_coords[vtx][2];
      poly->vert[j].nx= sphere_coords[vtx][0];
      poly->vert[j].ny= sphere_coords[vtx][1];
      poly->vert[j].nz= sphere_coords[vtx][2];
      poly->vert[j].r= r;
      poly->vert[j].g= g;
      poly->vert[j].b= b;
      ctbl++;
    }
    add( poly );
  }
}

TDrawLights::TDrawLights( const gColor& ltclr_in, const gVector& ltdir_in,
			  const gColor& ambclr_in )
{
  ltclr= ltclr_in; 
  ltdir= ltdir_in;
  ltdir.normalize();
  ambclr= ambclr_in;
}

gColor TDrawLights::calc_clr( const Poly::Vert& vert )
{
  gColor raw_clr( vert.r, vert.g, vert.b );
  gVector norm= gVector( vert.nx, vert.ny, vert.nz );
  if ( (norm.x()== 0.0) && (norm.y()==0.0) && (norm.z()==0.0) ) {
    return raw_clr;
  }
  float cos= -1.0*(norm * ltdir);
  if (cos<0.0) cos= 0.0;
  gColor lighted_clr= raw_clr * ( ambclr + (ltclr*cos) );
  return lighted_clr;
}

gColor TDrawLights::calc_clr_spec( const Poly::Vert& vert, const gPoint& from )
{
  gColor raw_clr( vert.r, vert.g, vert.b );
  gVector norm= gVector( vert.nx, vert.ny, vert.nz );
  if ( (norm.x()== 0.0) && (norm.y()==0.0) && (norm.z()==0.0) ) {
    return raw_clr;
  }
  norm.normalize();
  float cos= -1.0*(norm * ltdir);
  if (cos<0.0) cos= 0.0;
  gColor lighted_clr= raw_clr * ( ambclr + (ltclr*cos) );
  gVector neg_norm= norm*-1.0;
  if (neg_norm * dir()>= 0.0) {
    // front facing
    gVector neg_refl_dir= dir() - (neg_norm*(2.0*(dir()*neg_norm)));
    gVector viewdir= gPoint(vert.x,vert.y,vert.z) - from;
    viewdir.normalize();
    float spec_cos= -1.0*(viewdir*neg_refl_dir);
    float spec_weight= spec_cos*spec_cos;
    spec_weight *= spec_weight;
    spec_weight *= spec_weight;
    spec_weight *= spec_weight;
    lighted_clr += gColor(1.0,1.0,1.0)*spec_weight;
  }
  return lighted_clr;
}

int TDrawEngine::initial_primbuf_length= 1000;
int TDrawEngine::initial_vtxbuf_length= 1000;
int TDrawEngine::default_phong_flag= 0;
int TDrawEngine::default_spec_flag= 0;
short TDrawEngine::screen_minz= -32768;
short TDrawEngine::screen_maxz= 32767;

TDrawEngine::TDrawEngine( XDrawImageHandler* ihandler_in )
{
  ihandler= ihandler_in; 
  nice_image= NULL;
  zbuf_raster= NULL;
  zbuf= NULL;
  nice_image_valid= 0;
  current_geom= NULL;
  primbuf_length= initial_primbuf_length;
  primbuf= new PrimRec[ primbuf_length ];
  vtxbuf_length= initial_vtxbuf_length;
  vtxbuf= new VtxRec[ vtxbuf_length ];
  phong_flag= default_phong_flag;
  spec_flag= default_spec_flag;
  cam= NULL;
  cam_transform= NULL;
}

TDrawEngine::~TDrawEngine()
{
  delete [] primbuf;
  delete [] vtxbuf;
  delete nice_image;
  delete [] zbuf;
  delete [] zbuf_raster;
  delete cam_transform;
}

void TDrawEngine::update_cam_trans()
{
  int ih_xdim;
  int ih_ydim;
  ihandler->winsize(&ih_ydim, &ih_xdim);
  if (cam_transform) delete cam_transform;
  cam_transform= 
    cam->screen_projection_matrix( ihandler->xsize(), ihandler->ysize(),
				    screen_minz, screen_maxz );
}

void TDrawEngine::grow_primbuf()
{
  PrimRec* old_pbuf= primbuf;
  int old_length= primbuf_length;
  primbuf_length= 2*primbuf_length;
  primbuf= new PrimRec[primbuf_length];
  for (int i=0; i<old_length; i++)
    primbuf[i]= old_pbuf[i];
  delete [] old_pbuf;
}

void TDrawEngine::grow_vtxbuf()
{
  VtxRec* old_vbuf= vtxbuf;
  int old_length= vtxbuf_length;
  vtxbuf_length= 2*vtxbuf_length;
  vtxbuf= new VtxRec[vtxbuf_length];
  for (int i=0; i<old_length; i++)
    vtxbuf[i]= old_vbuf[i];
  delete [] old_vbuf;
}

int TDrawEngine::sort_compare(const void* p1, const void* p2)
{
  PrimRec* prim1= (PrimRec*)p1;
  PrimRec* prim2= (PrimRec*)p2;
  if (prim1->depth < prim2->depth) return -1;
  if (prim1->depth > prim2->depth) return 1;
  else return 0;
}

void TDrawEngine::redraw()
{
  if (nice_image_valid) {
    int ih_xdim;
    int ih_ydim;
    ihandler->winsize(&ih_ydim, &ih_xdim);
    if ((nice_image->xsize()==ih_xdim)
	&& (nice_image->ysize()==ih_ydim)) {
      ihandler->redraw();
    }
    else if (current_geom) draw_nicely(current_geom);
  }
  else if (current_geom) draw(current_geom);
  else {
    // do nothing
  }
}

int TDrawEngine::shadepixel_cb( const int i, const int j,
				Poly::Vert* p, Poly::SCAN step,
				void* cb_data )
{
  TDrawEngine* this_engine= (TDrawEngine*)cb_data;

  // Write pixel if it is in front
  if (p->sz > this_engine->zbuf[i][j]) { 
    // write the pixel
    this_engine->zbuf[i][j]= (short)p->sz;
    gBColor clr;
    if (this_engine->phong_flag) {
      clr= this_engine->current_lights.calc_clr( *p );
    }
    else clr= gBColor( (float)p->r, (float)p->g, (float)p->b );
    this_engine->nice_image->setpix( i, this_engine->nice_image->ysize()-j, 
				     clr );
  }

  return 0;
}

int TDrawEngine::shadepixel_spec_cb( const int i, const int j,
				     Poly::Vert* p, Poly::SCAN step,
				     void* cb_data )
{
  TDrawEngine* this_engine= (TDrawEngine*)cb_data;

  // Write pixel if it is in front
  if (p->sz > this_engine->zbuf[i][j]) { 
    // write the pixel
    this_engine->zbuf[i][j]= (short)p->sz;
    gBColor clr;
    if (this_engine->phong_flag) {
      clr= this_engine->current_lights.calc_clr_spec(*p, 
						     this_engine->cam->from());
    }
    else clr= gBColor( (float)p->r, (float)p->g, (float)p->b );
    this_engine->nice_image->setpix( i, this_engine->nice_image->ysize()-j, 
				     clr );
  }

  return 0;
}

void TDrawEngine::draw_nicely( TDrawGeom* geom_in, int recur_level )
{
  current_geom= geom_in;

  if (!recur_level) { // top level call
    if (!cam) {
      fprintf(stderr,"TDrawEngine::draw_nicely: camera not set!\n");
      return;
    }
    update_cam_trans();

    int ih_xdim;
    int ih_ydim;
    int i;
    int j;
    ihandler->winsize(&ih_ydim, &ih_xdim);
    if (nice_image && ((nice_image->xsize() != ih_xdim)
		       || (nice_image->ysize() != ih_ydim))) {
      delete nice_image;
      nice_image= NULL;
      delete [] zbuf_raster;
      zbuf_raster= NULL;
      delete [] zbuf;
      zbuf= NULL;
    }

    if (!nice_image) {
      nice_image= new rgbImage( ih_xdim, ih_ydim );
      zbuf_raster= new short[ ih_xdim*ih_ydim ];
      zbuf= new short*[ih_xdim];
      for (i=0; i<ih_xdim; i++) zbuf[i]= zbuf_raster+i*ih_ydim;
    }

    nice_image->clear();
    for (i=0; i<ih_xdim; i++)
      for (j=0; j<ih_ydim; j++) zbuf[i][j]= screen_minz;
  }

  // Render here
  isList_iter<TDrawNode> iter( geom_in->nodelist );
  TDrawNode* this_node;
  Poly_box poly_box;
  poly_box.x0= 0;
  poly_box.y0= 0;
  poly_box.z0= (double)screen_minz;
  poly_box.x1= ihandler->xsize();
  poly_box.y1= ihandler->ysize();
  poly_box.z1= (double)screen_maxz;
  Poly_window poly_win;
  poly_win.x0= poly_win.y0= 0;
  poly_win.x1= ihandler->xsize()-1;
  poly_win.y1= ihandler->ysize()-1;

  while (this_node= iter.next()) {
    switch (this_node->type) {
    case TDrawNode::POLY_TYPE: 
      {
	int i; 
	Poly poly= *(this_node->poly);
	
	// Skip lines and points for now
	if (poly.n<3) continue;

	// transform with view trans
	poly.trans_world( world, world_norm );
	
	// transform with cam trans, clip, homogenize
	if (!spec_flag) {
	  // No specular lighting
	  if (!phong_flag) {
	    // Gouraud 
	    for (i=0; i<poly.n; i++) 
	      current_lights.apply( poly.vert[i] );
	    poly.trans_to_screen( *cam_transform );
	    if ( poly.clip_to_box( &poly_box ) != Poly::CLIP_OUT ) {
	      poly.homogenize();
	      poly.scan( &poly_win, shadepixel_cb, (void*)this );
	    }	
	  }
	  else {
	    // Phong
	    poly.trans_to_screen( *cam_transform );
	    if ( poly.clip_to_box( &poly_box ) != Poly::CLIP_OUT ) {
	      poly.homogenize();
	      poly.scan( &poly_win, shadepixel_cb, (void*)this );
	    }	
	  }
	}
	else {
	  // Specular lighting on
	  // interpolate world coords
	  unsigned long old_mask= poly.mask;
	  poly.mask |= (POLY_MASK(x) | POLY_MASK(y) | POLY_MASK(z));
	  if (!phong_flag) {
	    // Gouraud 
	    for (i=0; i<poly.n; i++) 
	      current_lights.apply_spec( poly.vert[i], cam->from() );
	    poly.trans_to_screen( *cam_transform );
	    if ( poly.clip_to_box( &poly_box ) != Poly::CLIP_OUT ) {
	      poly.homogenize();
	      poly.scan( &poly_win, shadepixel_spec_cb, (void*)this );
	    }	
	  }
	  else {
	    // Phong
	    poly.trans_to_screen( *cam_transform );
	    if ( poly.clip_to_box( &poly_box ) != Poly::CLIP_OUT ) {
	      poly.homogenize();
	      poly.scan( &poly_win, shadepixel_spec_cb, (void*)this );
	    }	
	  }
	  poly.mask= old_mask;
	}
      }
      break;
      
    case TDrawNode::COMPOUND_TYPE:
      draw_nicely( this_node->geom, recur_level+1 );
      break;
    }
  }
  
  if (!recur_level) {
    ihandler->display(nice_image);
    nice_image_valid= 1;
  }
}

void TDrawEngine::draw( TDrawGeom* geom_in, int recur_level )
{
  current_geom= geom_in;

  if (!recur_level) {
    if (!cam) {
      fprintf(stderr,"TDrawEngine::draw: camera not set!\n");
      return;
    }
    update_cam_trans();

    primbuf_top= primbuf;
    vtxbuf_top= vtxbuf;
  }

  isList_iter<TDrawNode> iter( geom_in->nodelist );
  TDrawNode* this_node;
  while (this_node= iter.next()) {
    switch (this_node->type) {
    case TDrawNode::POLY_TYPE: 
    {
      int i; 
      Poly poly= *(this_node->poly);

      // transform with view trans
      poly.trans_world( world, world_norm );
      
      // light vertices
      if (!spec_flag) {
	for (i=0; i<poly.n; i++) current_lights.apply( poly.vert[i] );
      }
      else {
	for (i=0; i<poly.n; i++) current_lights.apply_spec( poly.vert[i],
							    cam->from() );
      }
      
      // transform with cam trans, homogenize
      poly.trans_to_screen( *cam_transform );
      poly.homogenize();
      
      // copy to primbuf and vtxbuf
      if (primbuf_top >= primbuf + primbuf_length) grow_primbuf();
      if (vtxbuf_top + poly.n >= vtxbuf + vtxbuf_length) grow_vtxbuf();
      float ztemp= 0;
      gColor ctemp;
      primbuf_top->offset= vtxbuf_top - vtxbuf;
      for (i=0; i<poly.n; i++) {
	ztemp += poly.vert[i].sz;
	ctemp.add_noclamp( gColor( poly.vert[i].r, poly.vert[i].g,
				   poly.vert[i].b ) );
	vtxbuf_top->x= (short)(poly.vert[i].sx + 0.5);
	vtxbuf_top->y= (short)ihandler->ysize() - 
	    (short)(poly.vert[i].sy + 0.5);
	vtxbuf_top++;
      }
      primbuf_top->clr= ctemp * (1.0/poly.n);
      primbuf_top->depth= ztemp * (1.0/poly.n);
      if (poly.n > 2) primbuf_top->type= POLYGON;
      else primbuf_top->type= LINE;
      primbuf_top->nverts= poly.n;
      primbuf_top++;
    }
    break;

    case TDrawNode::COMPOUND_TYPE:
      draw( this_node->geom, recur_level+1 );
      break;
    }
  }

  // Sort and draw the primbuf
  qsort( primbuf, primbuf_top - primbuf, sizeof(PrimRec), 
	 sort_compare );
  draw_primbuf();
  nice_image_valid= 0;
}

void TDrawEngine::draw_primbuf()
{
  // This routine actually does the drawing
  // of the sorted depth buffer

  ihandler->clear(0, 0, 0);

  for (PrimRec* runner= primbuf; runner<primbuf_top; runner++) {
    switch (runner->type) {
    case POLYGON:
      ihandler->pgon( runner->clr.ir(), runner->clr.ig(), runner->clr.ib(),
		      runner->nverts,
		      (XPoint*)(vtxbuf + runner->offset) );
      break;

    case LINE:
      ihandler->pline( runner->clr.ir(), runner->clr.ig(), runner->clr.ib(),
		       2, (XPoint*)(vtxbuf + runner->offset) );
      break;
    }
  }

  // Display the result
  ihandler->redraw();
}

