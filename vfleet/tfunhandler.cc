/****************************************************************************
 * tfunhandler.cc
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

#include <stdio.h>
#include <string.h>
#include <Xm/Xm.h>        // needed to satisfy some defs in vfleet.h
#include <Mrm/MrmAppl.h>  // likewise

#include "vfleet.h"

int baseTfunHandler::debug= 0;

baseTfunHandler* TfunHandler_load( FILE *infile, const GridInfo* grid_in )
{
  int tfuntype;

  if ( fscanf( infile, "%d", &tfuntype ) != 1 ) {
    fprintf(stderr,"TfunHandler_load: bad file!\n");
    return NULL;
  }

  if (baseTfunHandler::debug) 
    fprintf(stderr,"TfunHandler_load: type %d\n",tfuntype);

  baseTfunHandler *result;

  switch (tfuntype) {
  case BASE_TFUN:
    result= new baseTfunHandler( infile, grid_in );
    break;
  case BBOX_TFUN:
    result= new bboxTfunHandler( infile, grid_in );
    break;
  case SUM_TFUN:
    result= new sumTfunHandler( infile, grid_in );
    break;
  case TABLE_TFUN:
    result= new tableTfunHandler( infile, grid_in );
    break;
  case GRADTABLE_TFUN:
    result= new gradtableTfunHandler( infile, grid_in );
    break;
  case SSUM_TFUN:
    result= new ssumTfunHandler( infile, grid_in );
    break;
  case MASK_TFUN:
    result= new maskTfunHandler( infile, grid_in );
    break;
  case BLOCK_TFUN:
    result= new blockTfunHandler( infile, grid_in );
    break;
  default:
    fprintf(stderr,"TfunHandler_load: unknown tfun type %d!\n",tfuntype);
    result= NULL;
    break;
  }

  if (baseTfunHandler::debug) {
    fprintf(stderr,"load: type %d complete; %lx\n", tfuntype,(long)result);
    if (result) fprintf(stderr,"              %lx\n",(long)(result->get_tfun()));
  }

  if (result && result->get_tfun()) return result;
  else {
    delete result;
    return NULL;
  }
}

baseTfunHandler::baseTfunHandler( FILE* infile, const GridInfo* grid_in )
  : grid( *grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "baseTfunHandler::baseTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  int ndata;
  if ( fscanf( infile, "%d", &ndata ) != 1 ) {
    fprintf(stderr,"baseTfunHandler read routine: bad file!\n");
    tfun= NULL;
    valid= 0;
  }
  else {
    tfun= new baseTransferFunction( ndata );
    valid= 1;
  }
  if (debug) 
    fprintf(stderr,
	    "baseTfunHandler::baseTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

baseTfunHandler::~baseTfunHandler()
{
  delete tfun;
}

int baseTfunHandler::save( FILE* ofile )
{
  if (valid) {
    fprintf( ofile, "%d %d ", 
	     (int)tfun->type(), tfun->ndata() );
    return 1;
  }
  else {
    fprintf( ofile, "%d %d \n",
	     (int)tfun->type(), 0 );
    return 0;
  }
}

dvol_struct* baseTfunHandler::lookup_dvol( const int index )
{
  // Just find the index'th element of the datavol list
  if (index<0) return datavol_list.head();

  dvol_struct** this_struct;
  dList_iter<dvol_struct*> iter(datavol_list);
  for (int i=0; i<=index; i++) {
    this_struct= iter.next();
    if (!this_struct) { // Walked off end of list
      // File requests a datavol with higher index than we have
      return datavol_list.head();
    }
  }
  return *this_struct;
}

void baseTfunHandler::refit_to_volume( const GridInfo* grid_in )
{
  grid= *grid_in;
}

bboxTfunHandler::bboxTfunHandler( FILE *infile, const GridInfo* grid_in )
: baseTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "bboxTfunHandler::bboxTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {
    int xsize, ysize, zsize;
    float bounds[6];
    fscanf( infile, "%*s\n" );
    delete tfun;
    tfun= new BoundBoxTransferFunction( gBoundBox( -0.5, -0.5, -0.5,
						   0.5, 0.5, 0.5 ),
					64, 64, 64 );
  }
   if (debug) 
    fprintf(stderr,
	    "bboxTfunHandler::bboxTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

int bboxTfunHandler::save( FILE* ofile )
{
  if (baseTfunHandler::save( ofile )) {
    BoundBoxTransferFunction* btfun= (BoundBoxTransferFunction*)tfun;
    int xsize, ysize, zsize;
    gBoundBox bbox= btfun->boundbox();
    btfun->get_sizes( &xsize, &ysize, &zsize );
    fprintf( ofile, "BoundBoxTransferFunction\n" );
    return 1;
  }
  else return 0;
}

void bboxTfunHandler::refit_to_volume( const GridInfo* grid_in )
{
  baseTfunHandler::refit_to_volume(grid_in);

  BoundBoxTransferFunction* btfun= (BoundBoxTransferFunction*)get_tfun();

  ((BoundBoxTransferFunction*)get_tfun())->set_grid( *grid_in );
}

tableTfunHandler::tableTfunHandler( FILE* infile, const GridInfo* grid_in )
: baseTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "tableTfunHandler::tableTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {
    gBColor *clr_table= NULL;
    
    int ndata= tfun->ndata();
    
    fscanf(infile,"%*s");
    if (fscanf(infile,"%d", &current_dvol_index) != 1) valid= 0;
    if (valid) clr_table= load_color_table( infile );
    
    if (valid) {
      delete tfun;
      tfun= new TableTransferFunction( ndata, clr_table );
    }

    delete clr_table;
  }    
  if (debug) 
    fprintf(stderr,
	    "tableTfunHandler::tableTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

gBColor* tableTfunHandler::load_color_table( FILE* infile )
{
  int index;
  int r, g, b, a;
  gBColor* clr_table= new gBColor[256];
  for (int i=0; i<256; i++) {
    if ( fscanf( infile, 
		"%d: %d %d %d %d",
		&index, &r, &g, &b, &a) == 5
	&& (index<256) && (index>=0) ) {
      clr_table[index]= gBColor(r,g,b,a);
    }
    else {
      valid= 0;
      break;
    }
  }

  return clr_table;
}

void tableTfunHandler::save_color_table( FILE* ofile, gBColor* ctbl )
{
  for (int i=0; i<256; i++) {
    gBColor clr= ctbl[i];
    fprintf(ofile,"%d: %d %d %d %d \n",
	    i, clr.ir(), clr.ig(), clr.ib(), clr.ia());
  }
}

int tableTfunHandler::save( FILE* ofile )
{
  if (baseTfunHandler::save( ofile )) {
    fprintf(ofile,"TableTransferFunction\n");
    fprintf(ofile,"%d \n",current_dvol_index);
    save_color_table(ofile, ((TableTransferFunction*)tfun)->get_table());
    return 1;
  }
  else return 0;
}

void tableTfunHandler::append_needed_dvols( int *count, 
					   sList<dvol_struct*> *needed )
{
  *count += 1;
  dvol_struct* current_dvol= lookup_dvol(current_dvol_index);
  needed->append( current_dvol );
}

gradtableTfunHandler::gradtableTfunHandler( FILE* infile,
					    const GridInfo* grid_in )
: baseTfunHandler(infile, grid_in), tableTfunHandler( infile, grid_in )
{
#ifdef CRAY_ARCH_C90
  if (tableTfunHandler::debug) 
#else
  if (debug) 
#endif
    fprintf(stderr,
  "gradtableTfunHandler::gradtableTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {
    TableTransferFunction* ttfun= (TableTransferFunction*)tfun;
    tfun= new GradTableTransferFunction( ttfun->ndata(), ttfun->get_table() );
    delete ttfun;
  }
#ifdef CRAY_ARCH_C90
  if (tableTfunHandler::debug) 
#else
  if (debug) 
#endif
    fprintf(stderr,
   "gradtableTfunHandler::gradtableTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

int gradtableTfunHandler::save( FILE* ofile )
{
  if (baseTfunHandler::save( ofile )) {
    fprintf(ofile,"GradTableTransferFunction\n");
    fprintf(ofile,"%d \n",current_dvol_index);
    save_color_table(ofile, ((GradTableTransferFunction*)tfun)->get_table());
    return 1;
  }
  else return 0;
}

sumTfunHandler::sumTfunHandler( FILE* infile, const GridInfo* grid_in,
				int load_kids )
: baseTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "sumTfunHandler::sumTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {
    int i;
    
    fscanf(infile, "%*s %d",&ntfuns);

    float *tmpfac= new float[ntfuns];
    for (i=0; i<ntfuns; i++) fscanf(infile,"%f",tmpfac+i);
    for (i=0; i<ntfuns; i++) factors.append( *(tmpfac+i) );
    
    if (load_kids) {
      baseTransferFunction **tfuns= new baseTransferFunction*[ntfuns];
      
      for (i=0; i<ntfuns; i++) {
	baseTfunHandler *thiskid;
	kids.append( thiskid= TfunHandler_load( infile, grid_in ) );
	if (thiskid) tfuns[i]= thiskid->get_tfun();
	else valid= 0;
      }
      
      if (valid) {
	// We actually count the number of data volumes needed by children,
	// to guard against errors in the loaded file.
	int ndata= 0;
	dList_iter<baseTfunHandler*> iter(kids);
	baseTfunHandler** kidptr;
	while (kidptr= iter.next())
	  ndata += (*kidptr)->get_tfun()->ndata();
	delete tfun;
	tfun= new SumTransferFunction( ndata, ntfuns, tmpfac, tfuns );
      }
      delete [] tfuns;
    }
    else {
      delete tfun;
      tfun= NULL;
    }
    delete [] tmpfac;
  }
  if (debug)
    fprintf(stderr,
	    "sumTfunHandler::sumTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

sumTfunHandler::~sumTfunHandler()
{
  dList_iter<baseTfunHandler*> iter(kids);
  baseTfunHandler** kidptr;
  while( kidptr= iter.next() ) {
    delete *kidptr;
  }
}

int sumTfunHandler::save( FILE* ofile )
{
  if ( baseTfunHandler::save( ofile ) ) {

    SumTransferFunction *stfun= (SumTransferFunction*)tfun;

    fprintf(ofile,"SumTransferFunction\n");
    fprintf(ofile,"%d \n",stfun->tfun_count());

    for (int i=0; i<stfun->tfun_count(); i++) 
      fprintf(ofile,"%f ",stfun->factor_table()[i]);
    fprintf(ofile,"\n");

    dList_iter<baseTfunHandler*> iter( kids );
    baseTfunHandler** kidptr;
    while (kidptr= iter.next())
      (*kidptr)->save(ofile);

    return 1;
  }
  else return 0;
}

void sumTfunHandler::append_needed_dvols( int *count, 
					 sList<dvol_struct*> *needed )
{
  dList_iter<baseTfunHandler*> iter(kids);
  baseTfunHandler** this_kid;

  while (this_kid= iter.next()) 
    (*this_kid)->append_needed_dvols( count, needed );
}

void sumTfunHandler::refit_to_volume( const GridInfo* grid_in )
{
  baseTfunHandler::refit_to_volume(grid_in);

  dList_iter<baseTfunHandler*> iter(kids);
  baseTfunHandler** this_kid;

  while (this_kid= iter.next()) 
    (*this_kid)->refit_to_volume( grid_in );
}

ssumTfunHandler::ssumTfunHandler( FILE* infile, const GridInfo* grid_in,
				  int load_kids )
: baseTfunHandler( infile, grid_in ), 
  sumTfunHandler( infile, grid_in, load_kids )
{
#ifdef CRAY_ARCH_C90
  if (tableTfunHandler::debug) 
#else
  if (debug) 
#endif
    fprintf(stderr,
	    "ssumTfunHandler::ssumTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {

    if (load_kids) {
      baseTransferFunction **tmp_tfuns= new baseTransferFunction*[ntfuns];
      float *tmp_fac= new float[ntfuns];
      SumTransferFunction* stfun= (SumTransferFunction*)tfun;

      int ndata= stfun->ndata();

      for (int i=0; i<ntfuns; i++) {
	tmp_tfuns[i]= stfun->tfun_table()[i];
	tmp_fac[i]= stfun->factor_table()[i];
      }

      delete tfun;
      tfun= new SSumTransferFunction( ndata, ntfuns, tmp_fac, tmp_tfuns );

      delete [] tmp_fac;
      delete [] tmp_tfuns;
    }
    else {
      tfun= NULL;
    }
  }
#ifdef CRAY_ARCH_C90
  if (tableTfunHandler::debug) 
#else
  if (debug) 
#endif
    fprintf(stderr,
	    "ssumTfunHandler::ssumTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

ssumTfunHandler::~ssumTfunHandler()
{
  // Nothing to delete
}

int ssumTfunHandler::save( FILE* ofile )
{
  if ( baseTfunHandler::save( ofile ) ) {

    SSumTransferFunction *stfun= (SSumTransferFunction*)tfun;

    fprintf(ofile,"SSumTransferFunction\n");
    fprintf(ofile,"%d \n",stfun->tfun_count());

    for (int i=0; i<stfun->tfun_count(); i++) 
      fprintf(ofile,"%f ",stfun->factor_table()[i]);
    fprintf(ofile,"\n");

    dList_iter<baseTfunHandler*> iter( kids );
    baseTfunHandler** kidptr;
    while (kidptr= iter.next())
      (*kidptr)->save(ofile);

    return 1;
  }
  else return 0;
}

maskTfunHandler::maskTfunHandler( FILE* infile, const GridInfo* grid_in,
				  int load_kids )
: baseTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "maskTfunHandler::maskTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {
    if ( fscanf(infile, "%*s %d %f %d %f", 
		&mask_present, &mask_weight, 
		&input_present, &input_weight) != 4 ) {
      valid= 0;
      delete tfun;
      tfun= NULL;
      return;
    }

    if (load_kids) {
      if (mask_present) {
	mask_handler= TfunHandler_load( infile, grid_in );
	if (!mask_handler) {
	  valid= 0;
	  delete tfun;
	  tfun= NULL;
	  return;
	}
      }
      else mask_handler= NULL;
      if (input_present) {
	input_handler= TfunHandler_load( infile, grid_in );
	if (!input_handler) {
	  valid= 0;
	  delete tfun;
	  tfun= NULL;
	  return;
	}
      }
      else input_handler= NULL;

      // We actually count the number of data volumes needed by children,
      // to guard against errors in the loaded file.
      int ndata= 0;
      if (mask_handler) 
	ndata += mask_handler->get_tfun()->ndata();
      if (input_handler)
	ndata += input_handler->get_tfun()->ndata();
      if (mask_handler && input_handler) {
	delete tfun;
	tfun= new MaskTransferFunction( ndata, 
					input_handler->get_tfun(), 
					input_weight, 
					mask_handler->get_tfun(), 
					mask_weight );
      }
      else {
	delete tfun;
	tfun= NULL;
      }
    }
    else {
      delete tfun;
      tfun= NULL;
      mask_handler= input_handler= NULL;
    }
  }
  if (debug) 
    fprintf(stderr,
	    "maskTfunHandler::maskTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

maskTfunHandler::~maskTfunHandler()
{
  delete mask_handler;
  delete input_handler;
}

int maskTfunHandler::save( FILE* ofile )
{
  if ( baseTfunHandler::save( ofile ) ) {

    if (!mask_handler || !input_handler) {
      fprintf(stderr,"maskTfunHandler::save: incomplete tfun!\n");
      return 0;
    }

    MaskTransferFunction *mtfun= (MaskTransferFunction*)tfun;

    fprintf(ofile,"MaskTransferFunction\n");
    fprintf(ofile,"%d %f %d %f\n",
	    (mask_handler != NULL), mtfun->get_mask_weight(), 
	    (input_handler != NULL), mtfun->get_input_weight());

    if ((!mask_handler) || !(mask_handler->save(ofile))) return 0;
    if ((!input_handler) || !(input_handler->save(ofile))) return 0;

    return 1;
  }
  else return 0;
}

void maskTfunHandler::append_needed_dvols( int* count, 
					   sList<dvol_struct*> *needed )
{
  if (mask_handler) mask_handler->append_needed_dvols( count, needed );
  if (input_handler) input_handler->append_needed_dvols( count, needed );
}

void maskTfunHandler::refit_to_volume( const GridInfo* grid_in )
{
  baseTfunHandler::refit_to_volume(grid_in);

  if (mask_handler) mask_handler->refit_to_volume( grid_in );
  if (input_handler) input_handler->refit_to_volume( grid_in );
}

blockTfunHandler::blockTfunHandler( FILE* infile, const GridInfo* grid_in )
  : baseTfunHandler(infile,grid_in)
{
  if (debug) 
    fprintf(stderr,
	    "blockTfunHandler::blockTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (valid) {
    float fxmin;
    float fymin;
    float fzmin;
    float fxmax;
    float fymax;
    float fzmax;
    if ( fscanf(infile, "%*s %f %f %f %f %f %f", 
		&fxmin, &fymin, &fzmin, &fxmax, &fymax, &fzmax) != 6 ) {
      valid= 0;
      delete tfun;
      tfun= NULL;
      return;
    }

    int ir;
    int ig;
    int ib;
    int ia;
    int inside;
    if ( fscanf(infile, "%d %d %d %d %d",&ir,&ig,&ib,&ia,&inside) != 5 ) {
      valid= 0;
      delete tfun;
      tfun= NULL;
      return;
    }

    delete tfun;
    tfun= new BlockTransferFunction( fxmin, fymin, fzmin,
				     fxmax, fymax, fzmax,
				     gBColor( ir, ig, ib, ia ), inside );
  if (debug) 
    fprintf(stderr,
	    "blockTfunHandler::blockTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  }
}

blockTfunHandler::~blockTfunHandler()
{
}

int blockTfunHandler::save( FILE* ofile )
{
  if ( baseTfunHandler::save( ofile ) ) {
    BlockTransferFunction *btfun= (BlockTransferFunction*)tfun;

    fprintf(ofile,"BlockTransferFunction\n");
    float fxmin;
    float fymin;
    float fzmin;
    float fxmax;
    float fymax;
    float fzmax;
    gBColor clr;
    int inside;
    btfun->get_info( &fxmin, &fymin, &fzmin, &fxmax, &fymax, &fzmax,
		     &clr, &inside );
    fprintf(ofile,"%f %f %f %f %f %f\n",
	    fxmin, fymin, fzmin, fxmax, fymax, fzmax );
    fprintf(ofile,"%d %d %d %d %d\n",
	    clr.ir(),clr.ig(),clr.ib(),clr.ia(),inside);

    return 1;
  }
  else return 0;
}

void blockTfunHandler::refit_to_volume( const GridInfo* grid_in )
{
  baseTfunHandler::refit_to_volume(grid_in);

  ((BlockTransferFunction*)get_tfun())->set_grid( *grid_in );
}

