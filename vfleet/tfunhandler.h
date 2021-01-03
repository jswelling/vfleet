/****************************************************************************
 * tfunhandler.h 
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

#include "lists.h"

class DataVolume;
struct dvol_struct;

class baseTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile, 
					  const GridInfo* grid_in );
public:
  virtual ~baseTfunHandler();
  virtual int save( FILE* ofile );
  baseTransferFunction* get_tfun() const { return tfun; }
  virtual void append_needed_dvols( int *count, 
				    sList<dvol_struct*> *needed ) {}
  virtual void refit_to_volume( const GridInfo* grid_in );
  dvol_struct* lookup_dvol( const int list_index );
  static void set_debug( const int val ) { debug= val; }
  static int get_debug() { return debug; }
protected:
  static int debug;
  GridInfo grid;
  baseTfunHandler( FILE* ifile, const GridInfo* grid_in );
  baseTransferFunction *tfun;
  int valid;
};

baseTfunHandler* TfunHandler_load( FILE *ifile, const GridInfo* grid_in );

class bboxTfunHandler: virtual public baseTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile, 
					  const GridInfo* grid_in );
public:
  int save( FILE* ofile );
  void refit_to_volume( const GridInfo* grid_in );
protected:
  bboxTfunHandler( FILE* infile, const GridInfo* grid_in );
};

class tableTfunHandler: virtual public baseTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile,
					  const GridInfo* grid_in );
public:
  virtual int save( FILE* ofile );
  void append_needed_dvols( int *count, sList<dvol_struct*> *needed );
protected:
  tableTfunHandler( FILE* infile, const GridInfo* grid_in );
  gBColor* load_color_table( FILE* infile ); // allocs 256 gBColors
  void save_color_table( FILE* ofile, gBColor* ctbl );
  int current_dvol_index;
};

class gradtableTfunHandler: virtual public tableTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile,
					  const GridInfo* grid_in );
public:
  int save( FILE* ofile );
protected:
  gradtableTfunHandler( FILE* infile, const GridInfo* grid_in );
};

class sumTfunHandler: virtual public baseTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile,
					  const GridInfo* grid_in );
public:
  ~sumTfunHandler();
  int save( FILE* ofile );
  void append_needed_dvols( int *count, sList<dvol_struct*> *needed );
  void refit_to_volume( const GridInfo* grid_in );
protected:
  sumTfunHandler( FILE* infile, const GridInfo* grid_in, int load_kids=1 );
  int ntfuns;
  dList<float> factors;
  dList<baseTfunHandler*> kids;
};

class ssumTfunHandler: virtual public sumTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile,
					  const GridInfo* grid_in );
public:
  ~ssumTfunHandler();
  int save( FILE* ofile );
protected:
  ssumTfunHandler( FILE* infile, const GridInfo* grid_in, int load_kids=1 );
};

class maskTfunHandler: virtual public baseTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile, 
					  const GridInfo* grid_in );
public:
  ~maskTfunHandler();
  int save( FILE* ofile );
  void append_needed_dvols( int *count, sList<dvol_struct*> *needed );
  void refit_to_volume( const GridInfo* grid_in );
protected:
  maskTfunHandler( FILE* infile, const GridInfo* grid_in, int load_kids=1 );
  baseTfunHandler* mask_handler;
  int mask_present;
  float mask_weight;
  baseTfunHandler* input_handler;
  int input_present;
  float input_weight;
};

class blockTfunHandler: virtual public baseTfunHandler {
friend baseTfunHandler* TfunHandler_load( FILE *infile, 
					  const GridInfo* grid_in );
public:
  ~blockTfunHandler();
  int save( FILE* ofile );
  void refit_to_volume( const GridInfo* grid_in );
protected:
  blockTfunHandler( FILE* infile, const GridInfo* grid_in );
};
