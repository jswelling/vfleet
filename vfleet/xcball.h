/****************************************************************************
 * xcball.h
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
This class provides crystal ball user interface functionality for an
X input window.
*/

class XmCBall : public CBall {
public:
  XmCBall( Widget widget_in,
	  void (*motion_callback_in)(gTransfm *, void *, CBall *),
	  void *cb_info_in );
  ~XmCBall();
private:
  static void window_input_callback(Widget w, XmCBall *cball, XEvent *event);
  Widget widget;
};
