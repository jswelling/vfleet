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
#include "octree.h"

/* total cells hit */
static long cell_hit_count= 0;

/* total rays to traverse */
const int total_rays= 30000;

#ifdef never
class Sample {
public:
  Sample() { grad_data[0]= grad_data[1]= grad_data[2]= 128; }
  gBColor clr;
private:
  unsigned char grad_data[3];
};

class octSample : public Sample {
public:
  octSample() { clr_error= 0; }
  unsigned char clr_error;
  inline void color_ray() const;
};
#endif

class octSample {
public:
  octSample() { clr_error= 0; grad_data[0]= grad_data[1]= grad_data[2]= 128;}
  gBColor clr;
  unsigned char clr_error;
  inline void color_ray() const;
private:
  unsigned char grad_data[3];
};

void octSample::color_ray() const
{
  cell_hit_count++;
}

void leaf_initialize( void* cell_in,
		      int i_in, int j_in, int k_in, void *init )
{
  // Do nothing
}

void intersect_walk( Octcell_intersect_iter<octSample>& iter,
		    gVector& lookdir, gPoint& lookfm, 
		    const float mindist, const float maxdist)
{
#ifdef never
  float dist= 0.0;
  octSample *next_cell;
  while (next_cell= iter.next()) {
    if (iter.leaf()) {
//      dist += iter.rough_dist_in_cell();
      next_cell->color_ray();
    }
    else {
      Octcell_intersect_iter<octSample><octSample> sub_iter= iter.child_iter();
      intersect_walk( sub_iter, lookdir, lookfm, mindist, maxdist );
    }
  }
#endif

  float dist= 0.0;
  iter.reset( lookdir, lookfm, mindist, maxdist );
  octSample *next_cell;
  while (next_cell= iter.next()) {
    if (iter.leaf()) {
//      dist += iter.rough_dist_in_cell();
      next_cell->color_ray();
    }
    else iter.push();
  }
}

void walk_a_bunch_of_rays(Octree<octSample>* tree)
{
  gPoint lookfm(0.0, 0.0, 2.0);
  gVector lookdir(0.0,0.0,-1.0);

  int theta_range= total_rays / 3;
  int phi_range= sqrt( (float)theta_range );

#ifdef never
  for (int i=0; i<total_rays; i++) {
    float theta= M_PI * ((float)(i%theta_range)/(float)(theta_range-1));
    float phi= 2*M_PI * ((float)(i%phi_range)/(float)(phi_range-1));
    float x= 2.0*sin(theta)*cos(phi);
    float y= 2.0*sin(theta)*sin(phi);
    float z= 2.0*cos(theta);
    lookfm= gPoint(x,y,z);
    lookdir= gPoint(0.0,0.0,0.0) - lookfm;
    lookdir.normalize();
    Octcell_intersect_iter<octSample>
      isect_iter(tree, lookdir, lookfm, 0.0, 10.0);
    intersect_walk( isect_iter, lookdir, lookfm, 0.0, 10.0 );
  }
#endif

  Octcell_intersect_iter<octSample>
    isect_iter(tree, lookdir, lookfm, 0.0, 10.0);
  for (int i=0; i<total_rays; i++) {
    float theta= M_PI * ((float)(i%theta_range)/(float)(theta_range-1));
    float phi= 2*M_PI * ((float)(i%phi_range)/(float)(phi_range-1));
    float x= 2.0*sin(theta)*cos(phi);
    float y= 2.0*sin(theta)*sin(phi);
    float z= 2.0*cos(theta);
    lookfm= gPoint(x,y,z);
    lookdir= gPoint(0.0,0.0,0.0) - lookfm;
    lookdir.normalize();
    intersect_walk( isect_iter, lookdir, lookfm, 0.0, 10.0 );
  }
}

main( int argc, char *argv[] )
{
  /* Get parameters */
  if (argc != 4) {
    fprintf(stderr,"usage: %s nx ny nz\n", argv[0]);
    exit(-1);
  }

  fprintf(stdout,"sizeof(octSample)= %d\n",sizeof(octSample));

  int nx;
  int ny;
  int nz;
  nx= atoi(argv[1]);
  ny= atoi(argv[2]);
  nz= atoi(argv[3]);

  // Build the tree
  gBoundBox bbox(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5);
  Octree<octSample>* tree= 
    new Octree<octSample>(bbox,nx,ny,nz,leaf_initialize, NULL);

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

  walk_a_bunch_of_rays(tree);

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
  fprintf(stdout,"%d.%06d sec total;  ",u_sec,u_usec);
#else
  fprintf(stdout,"%d.%06du   %d.%06ds;  ",
          u_sec,u_usec,s_sec,s_usec);
#endif
  fprintf(stdout,"%d cell intersections\n",cell_hit_count);

  delete tree;
}

