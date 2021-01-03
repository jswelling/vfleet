/****************************************************************************
 * tinydraw.h
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

#include "lists.h"
#include "geometry.h"
#include "polyscan.h"

class rgbImage;

class XDrawImageHandler;

class TDrawGeom;

class TDrawNode : public sLink_base {
public:
  TDrawNode( Poly* poly_in )
    { type= POLY_TYPE; poly= poly_in; refcount= 0; }
  TDrawNode( TDrawGeom* geom_in )
    { type= COMPOUND_TYPE; geom= geom_in; refcount= 0; };
  ~TDrawNode();
  void ref() { refcount++; }
  void unref() { refcount--; if (!refcount) delete this; }
  enum {
    POLY_TYPE,
    COMPOUND_TYPE
  } type;
  union {
    Poly* poly;
    TDrawGeom* geom;
  };
private:
  int refcount;
};

class TDrawGeom {
friend class TDrawEngine;
public:
  TDrawGeom() {}
  ~TDrawGeom();
  void add( Poly* poly_in );
  void add( TDrawGeom* geom_in );
  void add_sphere( const float r, const float g, const float b );
private:
  isList<TDrawNode> nodelist;
};

class TDrawLights {
public:
  TDrawLights( const gColor& ltclr_in, const gVector& ltdir_in,
	       const gColor& ambclr_in );
  TDrawLights() {}
  ~TDrawLights() {}
  gColor clr() const { return ltclr; }
  gVector dir() const { return ltdir; }
  gColor amb() const { return ambclr; }
  gColor calc_clr( const Poly::Vert& vert);
  gColor calc_clr_spec( const Poly::Vert& vert, const gPoint& lookfm );
  void apply( Poly::Vert& vert )
  {
    gColor lighted_clr= calc_clr( vert );
    vert.r= lighted_clr.r();
    vert.g= lighted_clr.g();
    vert.b= lighted_clr.b();
  }
  void apply_spec( Poly::Vert& vert, const gPoint& from )
  {
    gColor lighted_clr= calc_clr_spec( vert, from );
    vert.r= lighted_clr.r();
    vert.g= lighted_clr.g();
    vert.b= lighted_clr.b();
  }
private:
  gColor ltclr;
  gVector ltdir;
  gColor ambclr;
};

class TDrawEngine {
public:
  TDrawEngine( XDrawImageHandler* ihandler_in );
  ~TDrawEngine();
  void draw( TDrawGeom* geom_in, int recur_level= 0 );
  void draw_nicely( TDrawGeom* geom_in, int recur_level= 0 );
  void redraw();
  TDrawLights lights() const
    { return current_lights; }
  void set_lights( const TDrawLights& new_lights )
    { current_lights= new_lights; }
  gTransfm world_trans() const
    { return world; }
  void set_world_trans( const gTransfm& new_world, 
			const gTransfm& new_world_norm )
    { 
      world= new_world; 
      world_norm= new_world_norm;
    }
  gTransfm cam_trans() const
  { return *cam_transform; }
  void set_camera( Camera* new_cam ) 
  { cam= new_cam; nice_image_valid= 0; }
  void set_phong( const int phong_flag_in ) 
  { phong_flag= phong_flag_in; nice_image_valid= 0; }
  void set_spec( const int spec_flag_in ) 
  { spec_flag= spec_flag_in; nice_image_valid= 0; }
  enum PrimType {
    POLYGON,
    LINE
  };
private:
  XDrawImageHandler* ihandler;
  rgbImage* nice_image;
  short* zbuf_raster;
  short** zbuf;
  TDrawGeom* current_geom;
  TDrawLights current_lights;
  gTransfm world;
  gTransfm world_norm;
  Camera* cam;
  gTransfm* cam_transform;
  struct PrimRec {
   float depth;
   PrimType type;
   int nverts;
   int offset;
   gColor clr;
  };
  PrimRec* primbuf;
  PrimRec* primbuf_top;
  int primbuf_length;
  static int initial_primbuf_length;
  void grow_primbuf();
  struct VtxRec {
    short x;
    short y;
  };
  VtxRec* vtxbuf;
  VtxRec* vtxbuf_top;
  int vtxbuf_length;
  static int initial_vtxbuf_length;
  void grow_vtxbuf();
  static int sort_compare( const void* p1, const void* p2 );
  void draw_primbuf();
  int nice_image_valid;
  int phong_flag;
  static int default_phong_flag;
  int spec_flag;
  void update_cam_trans();
  static int default_spec_flag;
  static int shadepixel_cb( const int i, const int j, 
			    Poly::Vert* p, Poly::SCAN step,
			    void* cb_data );
  static int shadepixel_spec_cb( const int i, const int j, 
				 Poly::Vert* p, Poly::SCAN step,
				 void* cb_data );
  static short screen_minz;
  static short screen_maxz;
};

