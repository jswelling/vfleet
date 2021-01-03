/****************************************************************************
 * imagehandlerserver.h
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

// Reference info for ImageHandlerServers
// Name is lower case because it must match info in command lines
const char ImageHandlerServerExeName[]= "ihandlerserver";

class ImageHandlerServer: public baseServer, public baseNet {
public:
  ImageHandlerServer();
  ImageHandlerServer( RemObjInfo *rem_in );
  ~ImageHandlerServer();
  int netrcv( NetMsgType msg );
  static void initialize( baseImageHandler *ihandler, char *cmd );
  void create_ihandler( RemObjInfo *rem_in );
protected:
  static int initialized;
  static void handle_server_request();
  static baseImageHandler *default_ihandler;
};

