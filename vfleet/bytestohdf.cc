#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

main(int argc, char *argv[])
{
  int c;
  int errflg= 0;
  char *label= "";
  char *units= "";
  char *format= "";
  char *coordsys= "";
  char *filename= "bytes.hdf";
  int header_bytes= 0;

  while ((c= getopt(argc, argv, "l:u:f:c:o:s:")) != EOF)
    switch (c) {
    case 'l': label= optarg; break;
    case 'u': units= optarg; break;
    case 'f': format= optarg; break;
    case 'c': coordsys= optarg; break;
    case 'o': filename= optarg; break;
    case 's': sscanf(optarg,"%d",&header_bytes); break;
    case '?': errflg++; break;
    }
  if (argc-optind != 3) errflg++;
  if (errflg) {
    fprintf(stderr,
	    "%s: usage: %s [-llabelstring] [-uunitstring] [-fformatstring] \
[-ccoordsysstring] [-sheader_bytes] [-ooutfilename] nx ny nz\n",
	    argv[0],argv[0]);
    fprintf(stderr,
            "Note: the Z index varies fastest, so the number of slices is the X dim!\n");
    exit(-1);
  }

  int nx, ny, nz;
  sscanf(argv[optind],"%d",&nx);
  sscanf(argv[optind+1],"%d",&ny);
  sscanf(argv[optind+2],"%d",&nz);

  int i;
  if (header_bytes) {
    for (i=0; i<header_bytes; i++) (void)getchar();
  }

  unsigned char *buf= new unsigned char[nx*ny*nz];
  unsigned char *runner= buf;
  for (i=0; i<nx*ny*nz; i++) {
    if (feof(stdin)) break;
    *runner++= getchar();
  }

  fprintf(stderr,"got %d bytes; writing to file <%s>\n",i,filename);
  if (i<(nx*ny*nz)-1) {
    fprintf(stderr,"File not complete;  filling with %d zeros\n",
	    (nx*ny*nz)-(i+1));
    while (runner < (buf + (nx*ny*nz) - 1)) *runner++= 0;
  }

  unsigned char min= *buf;
  unsigned char max= *buf;
  for (runner=buf; runner<buf+nx*ny*nz; runner++) {
    if (*runner>max) max= *runner;
    if (*runner<min) min= *runner;
  }  

  int32 dims[3];
  dims[0]= nx;
  dims[1]= ny;
  dims[2]= nz;

  if (DFSDsetdims(3, dims)) 
    fprintf(stderr,"DFSDsetdims failed!\n");
  if (DFSDsetNT( DFNT_UINT8 )) 
    fprintf(stderr,"DFSDsetNT failed!\n");
  if (DFSDsetdatastrs(label,units,format,coordsys)) 
    fprintf(stderr,"DFSDsetdatastrs failed!\n");
  if (DFSDsetrange( (VOIDP)&max, (VOIDP)&min )) 
    fprintf(stderr,"DFSDsetrange failed!\n");
  if (DFSDputdata(filename, 3, dims, (VOIDP)buf)) 
    fprintf(stderr,"DFSDadddata failed!\n");

  delete buf;
}

