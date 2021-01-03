/* This example uses the pg_isosurface function to produce an iso-valued
 * surface from 3D gridded data.  A bounding box shows the bounds of the
 * computational region.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include <time.h>
#include <unistd.h>

#else /* ifdef CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E */

#ifdef DECCXX
extern "C" {
#endif

#ifdef SYSV_TIMING
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#include <sys/resource.h>

#ifdef DECCXX
}
#endif

#endif /* ifdef CRAY_ARCH_C90 */

#include "geometry.h"

void do_a_bunch_of_ops(const int n_iter)
{
#ifdef never
  gVector axis= gVector(1.0,1.0,0.0);
  gTransfm* trans= gTransfm::rotation( &axis, 0.0 );
  gVector vec= gVector(4.0,3.0,2.0,1.0);
  gVector result;

  for (int i=0; i<n_iter; i++) result= *trans * vec;
#endif

#ifdef never
  gVector axis1= gVector(1.0,1.0,0.0);
  gVector axis2= gVector(0.0,1.0,1.0);
  gTransfm* trans1= gTransfm::rotation( &axis1, 0.0 );
  gTransfm* trans2= gTransfm::rotation( &axis2, 0.0 );
  gTransfm result;

  for (int i=0; i<n_iter; i++) result= *trans1 * *trans2;
#endif

  gVector thisvec= gVector( 1.0, 2.0, 3.0 );
  gVector workvec;

  for (int i=0; i<n_iter; i++) {
    workvec= thisvec;
    workvec.normalize();
  }
}

main( int argc, char *argv[] )
{
  /* Get parameters */
  if (argc != 2) {
    fprintf(stderr,"usage: %s n_iterations\n", argv[0]);
    exit(-1);
  }

  int n_iter= atoi(argv[1]);

  // Get start timing info
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  long time_start= cpused();
#elif SYSV_TIMING
  struct tms time_start;
  (void)times( &time_start );
#else
  rusage start_rusage;
  getrusage(RUSAGE_SELF,&start_rusage);
#endif

  do_a_bunch_of_ops(n_iter);

  // Get and print end timing info
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  long ticks_per_sec= sysconf(_SC_CLK_TCK);
  long time_end= cpused();
  long s_usec= 0;
  long s_sec= 0;
  long u_sec= (time_end-time_start)/ticks_per_sec;
  long u_usec= ((time_end-time_start)%ticks_per_sec)/(ticks_per_sec/1000000.0);
#elif SYSV_TIMING
  struct tms time_end;
  (void)times( &time_end );
  long s_usec= (long)((1000000.0/CLK_TCK)
                      * (time_end.tms_stime - time_start.tms_stime));
  long s_sec= s_usec / 1000000;
  s_usec= s_usec % 1000000;
  long u_usec= (long)((1000000.0/CLK_TCK)
                      * (time_end.tms_utime - time_start.tms_utime));
  long u_sec= u_usec / 1000000;
  u_usec= u_usec % 1000000;
#else // not CRAY_ARCH_C90 or CRAY_ARCH_T3D or CRAY_ARCH_T3E
  rusage end_rusage;
  getrusage(RUSAGE_SELF,&end_rusage);
  long s_usec= end_rusage.ru_stime.tv_usec - start_rusage.ru_stime.tv_usec;
  long s_sec= end_rusage.ru_stime.tv_sec - start_rusage.ru_stime.tv_sec;
  long u_usec= end_rusage.ru_utime.tv_usec - start_rusage.ru_utime.tv_usec;
  long u_sec= end_rusage.ru_utime.tv_sec - start_rusage.ru_utime.tv_sec;
#endif
  
  if (s_usec<0) {
    s_sec -= 1;
    s_usec += 1000000;
  }
  if (u_usec<0) {
    u_sec -= 1;
    u_usec += 1000000;
  }
#if ( CRAY_ARCH_C90 || CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  fprintf(stdout,"%d.%06d sec total\n",u_sec,u_usec);
#else
  fprintf(stdout,"%d.%06du   %d.%06ds\n",u_sec,u_usec,s_sec,s_usec);
#endif
}

