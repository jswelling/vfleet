#include <stdio.h>

#include <stdlib.h>
#if (CRAY_ARCH_T3D || CRAY_ARCH_T3E)
#include <mpp/shmem.h>
#else
****ERROR**** This utility can only be built on Cray MPP systems
#endif
#include <time.h>
#include "rgbimage.h"

const int xdim= 300;
const int ydim= 300;
const int n_iter= 30;

int kludge_data_space= xdim*ydim;

class matchedImage: public rgbImage {
public:
  matchedImage( const int xdim_in, const int ydim_in, 
	       const gBColor& clr1, const gBColor& clr2, const gBColor& clr3 );
};

matchedImage::matchedImage( const int xdim_in, const int ydim_in,
			   const gBColor& clr1, const gBColor& clr2,
			   const gBColor& clr3 )
: rgbImage( xdim_in, ydim_in,
	   xdim_in*ydim_in,get_matched_memory(xdim_in, ydim_in),
	   1, &kludge_data_space, 0 )
{
  unsigned char* runner= byte_comp_pixels;
  for (int i=0; i<xdim_in*ydim_in; i++) {
    switch ((i/8)%3) {
    case 0:
      *runner++= clr1.ir();
      *runner++= clr1.ig();
      *runner++= clr1.ib();
      *runner++= clr1.ia();
      break;
    case 1:
      *runner++= clr2.ir();
      *runner++= clr2.ig();
      *runner++= clr2.ib();
      *runner++= clr2.ia();
      break;
    case 2:
      *runner++= clr3.ir();
      *runner++= clr3.ig();
      *runner++= clr3.ib();
      *runner++= clr3.ia();
      break;
    }
  }
}

main()
{
#if ! ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  fprintf(stderr,"This timing routine is designed to run on the T3D or T3E only!\n");
  exit(0);
#endif

  long t_start, t_final;
  int i;

  gBColor clr1(10, 10, 10, 10);
  gBColor clr2(0, 0, 0, 0);
  gBColor clr3(10, 10, 10, 255);
  gBColor clr4(255, 255, 255, 255);

  rgbImage* image1= new rgbImage(xdim,ydim);
  image1->setpix(0,0,clr1);
  for (i=1; i<xdim*ydim; i++) {
    switch ((i/8)%4) {
    case 0:
      image1->setnextpix(clr1);
      break;
    case 1:
      image1->setnextpix(clr2);
      break;
    case 2:
      image1->setnextpix(clr3);
      break;
    case 3:
      image1->setnextpix(clr4);
      break;
    }
  }

  matchedImage* image2= new matchedImage(xdim, ydim, clr1, clr2, clr3);

  shmem_udcflush();

  t_start= rtclock();

  for (i=0; i<n_iter; i++)
    image1->add_under(image2);

  t_final= rtclock();

  extern int _MPP_MY_PE;
  if (!_MPP_MY_PE) {
    fprintf(stderr,"Timing results: %d clocks\n",(t_final-t_start)/n_iter);
  }
}
