/****************************************************************************
 * composite.h
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

#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include <unistd.h>
#else
#include <sys/resource.h>
#endif

class compImageHandler;

class Compositor {
public:
  Compositor( const int nplanes_in, const int nihandlers_in,
	      const gBoundBox& bbox_in, baseImageHandler *ihandler_in,
	      baseLogger *logger_in= NULL );
  ~Compositor();
  baseImageHandler *get_ihandler( const int id );
  virtual void set_lookfrom( const gPoint& lookfm_in );
  virtual void set_boundbox( const gBoundBox& bbox_in );
  virtual void add_image( rgbImage* image, const int id )= 0;
  void note_start_time();
  void log_end_time();
protected:
  int nplanes;
  int nihandlers;
  gBoundBox bbox;
  baseImageHandler *output_ihandler;
  gPoint lookfm;
  gVector dir;
  compImageHandler **ihandler_table;
  rgbImage **plane;
  baseLogger *logger;
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  long wc_long_start;
  long time_start;  
#elif SYSV_TIMING
  time_t wc_start;
  struct tms time_start;
#else
  time_t wc_start;
  rusage start_rusage;
#endif
};

class compImageHandler: public baseImageHandler {
friend class Compositor;
public:
  compImageHandler( const int id_in, Compositor *creator_in );
  ~compImageHandler();
  void display( rgbImage *image, short refresh=1);
protected:
  Compositor *creator;
  int id;
};
