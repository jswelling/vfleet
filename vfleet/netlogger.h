/****************************************************************************
 * netlogger.h 
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

// This is the network logger object
class netLogger : public baseNet, public baseLogger {
public:
  netLogger( baseLogger *logger, RemObjInfo *rem_in );
  netLogger(  char *cmd );
  ~netLogger();
  int netrcv(NetMsgType msg);
  static void initialize( const char *name );
protected:
  static int initialized;
  static baseLogger *default_real_logger;
  static void handle_netlogger_request();
  virtual void write_string( char *string);
  baseLogger *outlogger;
};

// Reference info for LogServers
// Name is lower case because it must match info in command lines
extern char *LogServerExeName;

// This server produces netLoggers
class logServer : public baseServer, public baseNet {
public:
  logServer( char *arch= NULL );
  logServer( RemObjInfo *rem_in );
  ~logServer();
  int netrcv(NetMsgType msg);
  static void initialize( char *logfile,  char *cmd );
  static void initialize( char *cmd );
  static void initialize( baseLogger *outlogger_in, char *cmd );
  static void initialize( baseLogger *(*outlogger_create)(), char *cmd );
  void create_logger( RemObjInfo *rem_in,  char *param_info="" );
protected:
  static int initialized;
  static void handle_server_request();
  static baseLogger *outlogger;
  static baseLogger *(*outlogger_creation_cb)();
};

