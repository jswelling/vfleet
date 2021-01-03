/****************************************************************************
 * logger.h 
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

class multiLogger;

// The base logger object
class baseLogger {
friend class multiLogger;
public:
  baseLogger( const char *cmd);
  virtual ~baseLogger();
  virtual void comment( char *string) {
      write_string(string);
  };
protected:
  void open_logger();
  void close_logger();
  virtual void write_string( char *string);
  char *user;
  char *procname;
  int id;
  static int curr_logger_id;
  char *get_time_str();
};

// This version writes to a file, including lots of useful user info
class fileLogger : public baseLogger {
public:
  fileLogger(char *fname, const char *cmd);
  ~fileLogger();
protected:
  void write_string( char *string);
  FILE *ofile;
};

// This version writes to stdout, without the useful user info
class transpstdoutLogger : public baseLogger {
public:
  transpstdoutLogger( const char *cmd);
  transpstdoutLogger( const char *cmd, int quiet_in);
  ~transpstdoutLogger();
protected:
  void write_string( char *string);
  int quiet;
};

// This version writes to a file, without the useful user info
class transparentLogger : public baseLogger {
public:
  transparentLogger( char *fname, const char *cmd);
  ~transparentLogger();
protected:
  void write_string( char *string);
  FILE *ofile;
};

// This class sends its messages to multiple loggers
class multiLogger : public baseLogger {
public:
    multiLogger( const char *cmd, int nloggers_in,
		 baseLogger **loggers_in);
    ~multiLogger();
protected:
    void write_string( char *string);
private:
    int nloggers;
     baseLogger **loggers;
};
    
// This class is useful if one doesn't actually want to do any IO
class dummyLogger : public baseLogger {
public:
  dummyLogger( const char* cmd ) : baseLogger(cmd) {};
  ~dummyLogger() {}
protected:
  void write_string( char *string ) { /* Do nothing */ }
};
