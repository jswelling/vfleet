/****************************************************************************
 * voltool.h
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

extern gfm_popup_objects *file_dialog;
extern int file_select_callback(gfm_popup_objects *ip, 
				char *directory, char *file);
extern void popup_notice(Frame owner, char *text);
extern void voltool_quit();
extern void voltool_render_start( voltool_mainwindow_objects *ip );

extern Attr_attribute WIN_IHANDLER_KEY;
