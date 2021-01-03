/****************************************************************************
 * xlogger.h 
 * Author Joel Welling, Rob Earhart
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

#ifdef MOTIF

// This is a motif logger.
// It expects that it will be passed a widget and a resource
// string to use to set the widget text.

class motifLogger : public baseLogger {
public:
  motifLogger( char *cmd,
	       void *widget_in, //so we don\'t have to include Motif everywhere
	       char *res_string_in) : baseLogger(cmd) {
		 widget = widget_in;
		 res_string = res_string_in;
	       }
  ~motifLogger() {};
protected:
  void write_string( char *string );
private:
  void *widget;
  char *res_string;
};

class mlistLogger : public baseLogger {
public:
  mlistLogger( const char *cmd, Widget parent_widget );
  ~mlistLogger();
protected:
  void write_string( char *string );
private:
  static Widget list_widget; // so we don\'t have to include Motif everywhere
  static int list_id;
  XmString cmd_string;
  XmString current_string;
};

#endif // MOTIF

