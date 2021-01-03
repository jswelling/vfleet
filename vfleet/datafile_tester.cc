#include <stdio.h>
#include <stdlib.h>
#ifdef INCL_HDF
#include <hdf.h>
#endif
#include "datafile.h"

main( int argc, char *argv[] )
{
  if (argc != 2) {
    fprintf(stderr,"%s: usage: %s filename\n",argv[0],argv[0]);
    exit(-1);
  }

  baseDataFile *infile= new hdfDataFile(argv[1]);
  fprintf(stderr,"valid= %d\n",infile->valid());
  fprintf(stderr,"label= <%s>, units= <%s>\n",
	  infile->data_label(),infile->data_unit());
  fprintf(stderr,"coordsys= <%s>, format= <%s>\n",
	  infile->coordinate_system(), 
	  ((hdfDataFile *)infile)->data_print_format());
  fprintf(stderr,"dimensions %d %d %d\n",
	  infile->xsize(),infile->ysize(),infile->zsize());
  fprintf(stderr,"datatype= %d\n",infile->datatype());
  switch (infile->datatype()) {
  case baseDataFile::Float32:
    fprintf(stderr,"Range %f to %f\n",
	    infile->min().float32,infile->max().float32);
    break;
  case baseDataFile::Float64:
    fprintf(stderr,"Range %f to %f\n",
	    infile->min().float64,infile->max().float64);
    break;
  case baseDataFile::ByteS:
    fprintf(stderr,"Range %d to %d\n",
	    infile->min().bytes,infile->max().bytes);
    break;
  case baseDataFile::ByteU:
    fprintf(stderr,"Range %d to %d\n",
	    infile->min().byteu,infile->max().byteu);
    break;
  case baseDataFile::IntS16:
    fprintf(stderr,"Range %d to %d\n",
	    infile->min().ints16,infile->max().ints16);
    break;
  case baseDataFile::IntU16:
    fprintf(stderr,"Range %d to %d\n",
	    infile->min().intu16,infile->max().intu16);
    break;
  case baseDataFile::IntS32:
    fprintf(stderr,"Range %d to %d\n",
	    infile->min().ints32,infile->max().ints32);
    break;
  case baseDataFile::IntU32:
    fprintf(stderr,"Range %d to %d\n",
	    infile->min().intu32,infile->max().intu32);
    break;
  default: // Do nothing
    break;
  }

  // Generate a histogram
  int histo_buf[256];
  int i;
  for (i=0; i<256; i++) histo_buf[i]= 0;
  for (i=0; i<infile->xsize(); i++) {
    unsigned char *slice= 
      (unsigned char *)infile->get_xplane( i, baseDataFile::ByteU );
    if (!slice) fprintf(stderr,"Could not get slice %d!\n",i);
    else {
      for (int runner=0; runner<(infile->ysize()*infile->zsize()); runner++)
	histo_buf[ *(slice+runner) ]++;
      delete slice;
    }
  }
  fprintf(stderr,"Histogram of byte equivalents follows:\n");
  for (i=0; i<256; i++) fprintf(stderr,"   %3d: %6d\n",i,histo_buf[i]);
  fprintf(stderr,"\n");

  while (infile->next_dataset()) 
    fprintf(stderr,"Found another dataset\n");

  fprintf(stderr,"deleting...\n");
  delete infile;
}
