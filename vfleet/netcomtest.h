/****************************************************************************
 * netcomtest.h 
 * Author Joel Welling
 * Copyright 1992, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

// This is the network comtest object
class netComTest : public baseNet {
public:
  netComTest( RemObjInfo *rem_in );
  netComTest();
  ~netComTest();
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
  void doit(int i);
protected:
  static int initialized;
  static baseLogger *logger;
};

// Reference info for ComTestServers
extern char *ComTestServerExeName;

// This server produces netComTests
class ComTestServer : public baseServer, public baseNet {
public:
  ComTestServer( char *arch= NULL );
  ComTestServer( RemObjInfo *rem_in );
  ~ComTestServer();
  int netrcv(NetMsgType msg);
  static void initialize( char *cmd );
  void create_comtest( RemObjInfo *rem_in,  char *param_info="" );
protected:
  static int initialized;
  static void handle_server_request();
};

