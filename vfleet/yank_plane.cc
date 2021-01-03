#include <stdio.h>
#include <stdlib.h>
#include "datafile.h"

main( int argc, char *argv[] )
{
  if (argc != 3) {
    fprintf(stderr,"%s: usage: %s filename whichplane\n",argv[0],argv[0]);
    exit(-1);
  }

  baseDataFile *infile= new hdfDataFile(argv[1]);
  int whichplane;
  sscanf(argv[2],"%d",&whichplane);
  fprintf(stderr,"valid= %d\n",infile->valid());
  fprintf(stderr,"label= <%s>, units= <%s>\n",
	  infile->data_label(),infile->data_unit());
  fprintf(stderr,"coordsys= <%s>, format= <%s>\n",
	  infile->coordinate_system(), 
	  ((hdfDataFile *)infile)->data_print_format());
  fprintf(stderr,"dimensions %d %d %d\n",
	  infile->xsize(),infile->ysize(),infile->zsize());
  fprintf(stderr,"datatype= %d\n",infile->datatype());
  fprintf(stderr,"Grabbing plane %d\n",whichplane);

  // Get the plane
  unsigned char *buf= 
    (unsigned char *)infile->get_xplane( whichplane, baseDataFile::ByteU );
  if (!buf) {
    fprintf(stderr,"Could not get plane %d!\n",whichplane);
    exit(-1);
  }
  for (int i=0; i<(infile->ysize()*infile->zsize()); i++) putchar(buf[i]);

  delete infile;
  delete buf;
}
