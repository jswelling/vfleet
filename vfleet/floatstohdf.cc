#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#if ( !MAKING_DEPEND || INCL_HDF )
/* If HDF is not configured in, we need to protect these
 * includes from makedepend.
 */

#ifndef __HDF_INCLUDED__
#define PROTOTYPE
extern "C" {
#include <hdf.h>
#define __HDF_INCLUDED__
}
#undef PROTOTYPE
#endif

#endif

const int seek_buf_size= 1024;

main(int argc, char *argv[])
{
  int c;
  int errflg= 0;
  char *label= "";
  char *units= "";
  char *format= "";
  char *coordsys= "";
  char *filename= "floats.hdf";
  float max, min;
  int min_set= 0;
  int max_set= 0;
  int binary_set = 0;
  int fortran_set = 0;
  int header_bytes = 0;

  while ((c= getopt(argc, argv, "b:l:u:f:c:M:m:o:s:")) != EOF)
    switch (c) {
      case 'l': label= optarg; break;
      case 'u': units= optarg; break;
      case 'f': format= optarg; break;
      case 'c': coordsys= optarg; break;
      case 'o': filename= optarg; break;
      case 'M': sscanf(optarg,"%f",&max); min_set= 1; break;
      case 'm': sscanf(optarg,"%f",&min); max_set= 1; break;
      case 'b': binary_set = 1; 
		if (toupper(*optarg) =='F')fortran_set =1; 
		break;
      case 's': sscanf(optarg,"%d",&header_bytes); break;
      case '?': errflg++; break;
      }
  if (argc-optind != 3) errflg++;
  if (errflg) {
    fprintf(stderr,
	    "%s: usage: %s [-llabelstring] [-uunitstring] [-fformatstring] [-ccoordsysstring] [-Mmaxval] [-mminval] [-ooutfilename] [-bFortran/C] [-sheader_bytes] nx ny nz\n",
	    argv[0],argv[0]);
    fprintf(stderr,
            "Note: the Z index varies fastest, so the number of slices is the X dim!\n");
    exit(-1);
  }

  int nx, ny, nz;
  sscanf(argv[optind],"%d",&nx);
  sscanf(argv[optind+1],"%d",&ny);
  sscanf(argv[optind+2],"%d",&nz);
  
  float *buf= new float[nx*ny*nz];
  float *runner= buf;
  unsigned int i;
  int retcode;
  float val;
  if ( binary_set) {
    if (fortran_set) printf("fortran\n");
    if ( fortran_set) i = read(0,buf,4);
    else {
      if (header_bytes) {
	float* seek_buf= new float[seek_buf_size];
	int seek_count= 0;
	while (header_bytes) {
	  seek_count= (header_bytes > seek_buf_size) ? 
	    seek_buf_size : header_bytes;
	  i= read(0, seek_buf, seek_count);
	  header_bytes -= seek_count;
	}
	delete [] seek_buf;
      }
    }
    i = read(0, buf, nx*ny*nz*sizeof(float));
    if (fortran_set) i /= 4;
  }
  
  else {
    for (i=0; i<nx*ny*nz; i++) {
      if (feof(stdin)) break;
      retcode= scanf("%g",&val);
      *runner= val;
      runner++; // scanf might be a macro
    }
  }
  
  fprintf(stderr,"got %d floats; writing to file <%s>\n",i,filename);
  if (i<(nx*ny*nz)-1) {
    fprintf(stderr,"File not complete;  filling with %d zeros\n",
	    (nx*ny*nz)-(i+1));
    while (runner < (buf + (nx*ny*nz) - 1)) *runner++= 0.0;
  }
  
  if (!min_set) {
    min= *buf;
    for (runner=buf; runner<buf+nx*ny*nz; runner++) {
      if (*runner<min) min= *runner;
    }
  }
  
  if (!max_set) {
    max= *buf;
    for (runner=buf; runner<buf+nx*ny*nz; runner++) {
      if (*runner>max) max= *runner;
    }
  }
  
  fprintf(stderr,"min= %f, max= %f\n",min, max);
  
  int32 dims[3];
  if (fortran_set) {
    dims[0]= nz;
    dims[1]= ny;
    dims[2]= nx;
  }	
  else {
    dims[0]= nx;
    dims[1]= ny;
    dims[2]= nz;
  }
  
  if (DFSDsetdims(3, dims))
    fprintf(stderr,"DFSDsetdims failed!\n");
  if (DFSDsetNT( DFNT_FLOAT32 ))
    fprintf(stderr,"DFSDsetNT failed!\n");
  if (DFSDsetdatastrs(label,units,format,coordsys))
    fprintf(stderr,"DFSDsetdatastrs failed!\n");
  if (DFSDsetrange( (VOIDP)&max, (VOIDP)&min ))
    fprintf(stderr,"DFSDsetrange failed!\n");
  if (DFSDputdata(filename, 3, dims, (VOIDP)buf))
    fprintf(stderr,"DFSDadddata failed!\n");
  
  delete buf;
}



