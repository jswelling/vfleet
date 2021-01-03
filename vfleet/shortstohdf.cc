#include <string.h>
#include <limits.h>

#if ( !MAKING_DEPEND || INCL_HDF )
/* If HDF is not configured in, we need to protect these
 * includes from makedepend.
 */

#ifndef __HDF_INCLUDED__
extern "C" {
#include <dfsd.h>
#define __HDF_INCLUDED__
}
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

  while ((c= getopt(argc, argv, "l:u:f:c:o:")) != EOF)
    switch (c) {
    case 'l': label= optarg; break;
    case 'u': units= optarg; break;
    case 'f': format= optarg; break;
    case 'c': coordsys= optarg; break;
    case 'o': filename= optarg; break;
    case '?': errflg++; break;
    }
  if (argc-optind != 3) errflg++;
  if (errflg) {
    fprintf(stderr,
	    "%s: usage: %s [-llabelstring] [-uunitstring] [-fformatstring] \
[-ccoordsysstring] [-ooutfilename] nx ny nz\n",
	    argv[0],argv[0]);
    exit(-1);
  }

  int nx, ny, nz;
  sscanf(argv[optind],"%d",&nx);
  sscanf(argv[optind+1],"%d",&ny);
  sscanf(argv[optind+2],"%d",&nz);

  unsigned short *buf= new unsigned short[nx*ny*nz];
  unsigned char *runner= (unsigned char *)buf;
  unsigned char *end= (unsigned char *)buf + 2*nx*ny*nz - 1;
  int i;
  for (i=0; i<2*nx*ny*nz; i++) {
    if (feof(stdin)) break;
    *runner++= getchar();
  }

  fprintf(stderr,"got %d bytes; writing to file <%s>\n",i,filename);
  if (i%2) fprintf(stderr,"Byte count odd;  these are not shorts!\n");
  if (i<(2*nx*ny*nz)-1) {
    fprintf(stderr,"File not complete;  filling with %d zeros\n",
	    (2*nx*ny*nz)-(i+1));
    while (runner < end) *runner++= 0;
  }

  unsigned short min= *buf;
  unsigned short max= *buf;
  unsigned short *srunner;
  for (srunner=buf; srunner<buf+nx*ny*nz; srunner++) {
    if (*srunner>max) max= *srunner;
    if (*srunner<min) min= *srunner;
  }  
  fprintf(stderr,"Rescaling using min= %d, max= %d\n",(int)min,(int)max);

  for (srunner=buf; srunner<buf+nx*ny*nz; srunner++)
    *srunner= (USHRT_MAX*(*srunner - min))/(max-min);

  int32 dims[3];
  dims[0]= nx;
  dims[1]= ny;
  dims[2]= nz;

  if (DFSDsetdims(3, dims)) 
    fprintf(stderr,"DFSDsetdims failed!\n");
  if (DFSDsetNT( DFNT_UINT16 )) 
    fprintf(stderr,"DFSDsetNT failed!\n");
  if (DFSDsetdatastrs(label,units,format,coordsys)) 
    fprintf(stderr,"DFSDsetdatastrs failed!\n");
  if (DFSDsetrange( (void *)&max, (void *)&min )) 
    fprintf(stderr,"DFSDsetrange failed!\n");
  if (DFSDputdata(filename, 3, dims, buf)) 
    fprintf(stderr,"DFSDadddata failed!\n");

  delete buf;
}

