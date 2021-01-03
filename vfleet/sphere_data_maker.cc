#include <stdio.h>
#include <limits.h>
#include <math.h>

#define DataType unsigned char
#define DataTypeMin 0
#define DataTypeMax UCHAR_MAX

static void make_data( DataType *buffer,
			int xdim,
			int ydim,
			int zdim ) {
    int i, j, k;
    float x, y, z, r= 0.5, val;
    
    for (i=0; i<xdim; i++) {
	x= (float)(i - (xdim/2))/(float)xdim;
	for (j=0; j<ydim; j++) {
	    y= (float)(j - (ydim/2))/(float)ydim;
	    for (k=0; k<zdim; k++) {
	      z= (float)(k - (zdim/2))/(float)zdim;
	      val= 0.5 - sqrt( x*x + y*y + z*z );
	      if (val>1.0) val= 1.0;
	      if (val<0.0) val= 0.0;
	      *(buffer + i*ydim*zdim + j*zdim + k)= 
		(DataType)((DataTypeMax-DataTypeMin)*val + DataTypeMin);
	    }
	}
    }
}

main( int argc, char *argv[] )
{
  if (argc != 4) {
    fprintf(stderr,"%s: usage: %s nx ny nz\n",argv[0],argv[0]);
    exit(-1);
  }

  int nx,ny,nz;
  sscanf(argv[1],"%d",&nx);
  sscanf(argv[2],"%d",&ny);
  sscanf(argv[3],"%d",&nz);

  unsigned char *buf= new unsigned char[nx*ny*nz];

  make_data(buf,nx,ny,nz);

  unsigned char *runner;
  for (runner=buf; runner<buf+(nx*ny*nz); ) putchar( *runner++ );
  delete buf;
}

