/****************************************************************************
 * cball.h
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/*
This class provides crystal ball user interface functionality.
*/

/* Struct to hold mouse position information */
struct MousePosition { int x, y, maxx, maxy; };

class CBall {
public:
  CBall( void (*motion_callback_in)(gTransfm *, void *, CBall *),
	 void *cb_info_in );
  ~CBall();
  void set_view( gTransfm& new_viewmatrix ) { *viewmatrix= new_viewmatrix; }
  const gTransfm *view() const { return viewmatrix; }
  void move( const MousePosition& mousedown, const MousePosition& mouseup );
private:
  gVector mouse_to_3d( const MousePosition& mouse );
  void mouse_delta( const MousePosition& mouse, float *dx, float *dy );
  void (*motion_callback)(gTransfm *, void *, CBall *);
  void *cb_info;
  gTransfm *viewmatrix;
};
