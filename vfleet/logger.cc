/****************************************************************************
 * logger.cc
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef ATTCC
#include <osfcn.h>
#else
#include <unistd.h>
#endif
#include <pwd.h>
#include "logger.h"

#define MAX_LOGIN_NAMELENGTH 63

int baseLogger::curr_logger_id= 0;

baseLogger::baseLogger(const char *cmd)
{
  id= curr_logger_id++;
  char *logname= getlogin();
  procname= new char[ strlen(cmd)+1 ];
  strcpy(procname,cmd);
  if (logname) {
    user= new char[ strlen(logname)+1 ];
    strcpy(user,logname);
  }
  else {
    user= new char[MAX_LOGIN_NAMELENGTH+1];
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    passwd *info= NULL;
#else
    passwd *info= getpwuid(getuid());
#endif
    if (info) strncpy(user,info->pw_name,MAX_LOGIN_NAMELENGTH);
    else strncpy(user,"*user_unknown*",MAX_LOGIN_NAMELENGTH);
  }
}

baseLogger::~baseLogger()
{
  delete user;
  delete procname;
}

void baseLogger::open_logger()
{
  write_string("New logger");
}

void baseLogger::close_logger()
{
  write_string("Logger closed");
}

void baseLogger::write_string( char *string )
{
  fprintf(stdout,"%s: <%s> <%s> %d: %s\n", 
	  get_time_str(),user, procname, id, string);
  fflush(stdout);
}

char *baseLogger::get_time_str()
{
#ifdef __GNUC__
  time_t tm;
#elif ( DECCXX || SGI_CC )
  time_t tm;
#else
  long tm;
#endif

  (void)time(&tm);
  char *str= ctime( &tm );
  str[ strlen(str)-1 ]= '\0'; // wipe out newline
  return( str );
}

fileLogger::fileLogger(char *fname, const char *cmd) : baseLogger(cmd)
{
  ofile= fopen(fname,"a");
  if (!ofile) {
    fprintf(stderr,
	    "fileLogger::fileLogger: could not open <%s> for append!\n",
	    fname);
    exit(2);
  }
  open_logger();
}

fileLogger::~fileLogger()
{
  close_logger();
  fclose(ofile);
}

void fileLogger::write_string(char *string)
{
  fprintf(ofile,"%s: <%s> <%s> %d: %s\n", 
	  get_time_str(), user, procname, id, string);
  fflush(ofile);
}

transpstdoutLogger::transpstdoutLogger(const char *cmd) 
: baseLogger(cmd)
{
  quiet= 0;
  open_logger();
}

transpstdoutLogger::transpstdoutLogger(const char *cmd, int quiet_in) 
: baseLogger(cmd)
{
  quiet= quiet_in;
  if (!quiet) open_logger();
}

transpstdoutLogger::~transpstdoutLogger()
{
  if (!quiet) close_logger();
}

void transpstdoutLogger::write_string(char *string)
{
  fprintf(stdout,"%s: %s\n", get_time_str(), string);
  fflush(stdout);
}

transparentLogger::transparentLogger(char *fname, const char *cmd) 
: baseLogger(cmd)
{
  ofile= fopen(fname,"a");
  if (!ofile) {
    fprintf(stderr,
     "transparentLogger::transparentLogger: could not open <%s> for append!\n",
	    fname);
    exit(2);
  }
  open_logger();
}

transparentLogger::~transparentLogger()
{
  close_logger();
  fclose(ofile);
}

void transparentLogger::write_string(char *string)
{
  fprintf(ofile,"%s: %s\n", 
	  get_time_str(), string);
  fflush(ofile);
}

multiLogger::multiLogger( const char *cmd, int nloggers_in,
			  baseLogger **loggers_in ) 
: baseLogger(cmd)
{
  nloggers = nloggers_in;
  loggers = new baseLogger*[nloggers];
  for (int lupe=0; lupe<nloggers; lupe++)
    loggers[lupe] = loggers_in[lupe];
}

multiLogger::~multiLogger()
{
  for (int i=0; i<nloggers; i++) delete loggers[i];
  delete [] loggers;
}

void multiLogger::write_string( char *string )
{
  for (int lupe=0; lupe<nloggers; lupe++)
    loggers[lupe]->write_string(string);
}
