/****************************************************************************
 * vfleet_tfun.cc
 * Author Joel Welling
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University *
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

// This module provides renderer control callbacks

#include <stdlib.h>
#include <stdio.h>
#ifdef ATTCC
#include <osfcn.h>
#define F_OK 0
#else
#include <unistd.h>
#endif

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Scale.h>
#include <Xm/ToggleBG.h>
#include <Mrm/MrmAppl.h>

#include "vfleet.h"
#include "datafile.h"

baseTfunHandler *tfun_handler= NULL;
baseXTfunHandler *xtfun_handler= NULL;

baseTransferFunction *main_tfun= NULL;
static baseTransferFunction *registered_tfun= NULL;

int tfun_changed_flag= 1;

dList<XSliceViewer*> slice_viewer_list;

static Widget tfun_open_id= NULL;
static Widget tfun_dlog_id= NULL;
static Widget tfun_save_id= NULL;

static const char* DEFAULT_TFUN_FNAME= "default_tfun.tfn";

static void tfun_method( Sample& smpl, int i, int j, int k, int ndata,
			 DataVolume **data_table )
// Note that generated colors must be multiplied by alpha, because
// the color accumulation algorithm assumes pre-multiplication.
{
  if (ndata<1) {
    fprintf(stderr,"tfun_method: %d is not enough data volumes!\n",ndata);
    exit(-1);
  }
  
  float r, g, b, alpha, value;
  value= (*data_table)->fval(i,j,k);
  if (value < 0.1)
    r= g= b= alpha= 0.0;
  else if (value < 0.3) {
    alpha= 0.3*value;
    b= 1.0;
    r= g= 0.0;
  }
  else if (value < 0.7) {
    alpha= value;
    r= b= 0.0;
    g= 1.0;
  }
  else {
    alpha= 1.0;
    r= b= 1.0;
    g= 0.0;
  }
 
  smpl.clr= gBColor( r, g, b, alpha );
}

void update_tfuns()
// This routine packages together the current set of tfuns and registers
// the result.
{
  if (tfun_handler && main_tfun) {
    GridInfo grid= datavol_list.head()->dvol->gridinfo();
    tfun_handler->refit_to_volume(&grid);
    delete registered_tfun;
    registered_tfun= main_vren->register_tfun( main_tfun );
  }
}

static void delete_slice_viewer( XSliceViewer* delete_me )
{
  dList_iter<XSliceViewer*> iter(slice_viewer_list);
  XSliceViewer** this_viewer;
  while (this_viewer= iter.next())
    if (*this_viewer==delete_me) iter.remove();
  delete delete_me;
}

static DataVolume** build_local_dvol_table()
// Allocates a dvol table, which the caller must free
{
  if (xtfun_handler) {
    sList<dvol_struct*> needed;
    int ndvol= 0;
    
    xtfun_handler->append_needed_dvols( &ndvol, &needed );
    
    if (ndvol != main_tfun->ndata()) {
      fprintf(stderr,"build_local_dvol_table: datavolume count mismatch!\n");
      exit(-1);
    }
    
    if (ndvol) {
      DataVolume** dvol_table= new DataVolume*[ main_tfun->ndata() ];
      for (int i=0; i<main_tfun->ndata(); i++) {
#ifdef LOCAL_VREN
	dvol_table[i]= needed.pop()->dvol;
#else
	if (num_remote_procs == 0) {
	  dvol_table[i]= needed.pop()->dvol;
	}
	else {
	  dvol_struct *this_struct= needed.pop();
#ifdef SC94
	  // This version introduces memory leaks and dysfunctional slices!
	  if (this_struct->file) {
	    dvol_table[i]= new sliceDataVolume( this_struct->file,
						this_struct->dvol->boundbox());
	    dvol_table[i]->finish_init();
	  }
	  else dvol_table[i]= this_struct->dvol;
#else
	  dvol_table[i]= new sliceDataVolume( this_struct->file,
					      this_struct->dvol->boundbox() );
	  dvol_table[i]->finish_init();
#endif
	}
#endif
      }

      return dvol_table;
    }
    else return NULL; // no dvols needed
  }
  else {
    return NULL;
  }
}

static DataVolume** build_remote_dvol_table()
// Allocates a dvol table, which the caller must free
{
  if (xtfun_handler) {
    sList<dvol_struct*> needed;
    int ndvol= 0;
    
    xtfun_handler->append_needed_dvols( &ndvol, &needed );
    
    if (ndvol != main_tfun->ndata()) {
      fprintf(stderr,"build_remote_dvol_table: datavolume count mismatch!\n");
      exit(-1);
    }
    
    if (ndvol) {
      DataVolume** dvol_table= new DataVolume*[ main_tfun->ndata() ];
      for (int i=0; i<main_tfun->ndata(); i++)
	dvol_table[i]= needed.pop()->dvol;
      
      return dvol_table;
    }
    else return NULL; // no dvols needed
  }
  else {
    return NULL;
  }
}

static void update_sliceviewers()
{
  if (xtfun_handler && slice_viewer_list.head()) {

    // Each needs a separate copy of the table, since each may delete
    // its copy.
    dList_iter<XSliceViewer*> iter(slice_viewer_list);
    XSliceViewer** this_viewer;
    while (this_viewer= iter.next()) {
      DataVolume** table= build_local_dvol_table();
      GridInfo grid= datavol_list.head()->dvol->gridinfo();
      (*this_viewer)->set_tfun( grid, main_tfun, main_tfun->ndata(), table );
      delete [] table;
    }
  }
}

static void delete_all_sliceviewers()
{
  XSliceViewer* this_viewer;
  while (slice_viewer_list.head()) delete slice_viewer_list.pop();
}

static void tfunhandler_update_cb( baseXTfunHandler *handler )
{
  tfun_changed_flag= 1;
  tfun_handler= xtfun_handler;
  main_tfun= tfun_handler->get_tfun();

  update_sliceviewers();
}

static void tfunhandler_delete_cb( baseXTfunHandler *handler )
{
  tfun_changed_flag= 1;

  delete_all_sliceviewers();

  if (handler) {
    delete xtfun_handler;  // deletes tfun_handler and main_tfun as well
    xtfun_handler= NULL;
    tfun_handler= NULL;
    main_tfun= NULL;
  }
}

void datavol_update_inform( int datavol_removed )
// This routine is called when the list of valid datavols changes
{
  if (xtfun_handler) 
    tfun_changed_flag= xtfun_handler->datavol_update_inform( datavol_removed );

  if (datavol_removed) { // May need to fix things
    if (datavol_list.head()) { // There is at least one datavol left
      if (tfun_changed_flag) update_sliceviewers();
    }
    else { // Last datavol deleted
      tfun_changed_flag= 1;

      delete_all_sliceviewers();
      
      delete xtfun_handler;  // deletes tfun_handler and main_tfun as well
      xtfun_handler= NULL;
      tfun_handler= NULL;
      main_tfun= NULL;
    }
  }
  else { // A datavol has been added
    // If an old datavol was loaded, a samplevolume created, that
    // datavol deleted, and another datavol of different dimensions
    // or resolution loaded, the old samplevolume will be invalid.
    // (We want to reuse it if possible to save some regeneration time).
    // Check here to see if we have to delete the old samplevolume.
    if (datavol_list.head()) { // better be true
      DataVolume* first_dvol= datavol_list.head()->dvol;
      if ( main_svol &&
	  ( ( first_dvol->xsize() != main_svol->xsize() )
	   || (first_dvol->ysize() != main_svol->ysize() )
	   || (first_dvol->zsize() != main_svol->zsize() )
	   || (first_dvol->boundbox() != main_svol->boundbox() )) ) { 
	// force regen of svol
	delete main_svol;
	main_svol= NULL;
	delete main_volgob;
	main_volgob= NULL;
      }
    }
    else {
      fprintf(stderr,
	 "datavol_update_inform: internal error: called inconsistently!\n");
      abort();
    }
  }
}

static baseTransferFunction *create_tfun(const char *fname)
{
  tfun_changed_flag= 1;

  FILE *tfun_file= fopen(fname,"r");
  if (!tfun_file) {
    pop_error_dialog("invalid_tfun_file_msg");
    return NULL;
  }
  
  GridInfo grid= datavol_list.head()->dvol->gridinfo();
  baseXTfunHandler* new_xtfun_handler= 
    XTfunHandler_load( tfunhandler_update_cb, 
		       tfunhandler_delete_cb,
		       mrm_id, tfun_file, &grid );
  if (new_xtfun_handler) {
    delete xtfun_handler;
    xtfun_handler= new_xtfun_handler;
  }
  else {
    pop_error_dialog("invalid_tfun_file_msg");
    return NULL;
  }
  tfun_handler= xtfun_handler;
  baseTransferFunction *result= tfun_handler->get_tfun();
  
  fclose(tfun_file);
  
  return( result );
}

static void save_tfun_to_file( baseTfunHandler *tfhandler, char *the_name )
{
  // Save the transfer function
  FILE *ofile= fopen(the_name, "w");
  if (!ofile) {
    fprintf(stderr,
	    "save_tfun_file_cb: unexpectedly cannot write file <%s>!\n",
	    the_name);
    return;
  }
  tfhandler->save(ofile);
  if (fclose(ofile)==EOF) {
    fprintf(stderr,"save_tfun_file_cb: unexpectedly cannot close <%s>!\n",
	    the_name);
    return;
  }

  // Log that we saved the file
  char *string= new char[ strlen(the_name) 
			  + strlen("Saved tfun to ") + 1 ];
  strcpy(string,"Saved tfun to ");
  strcat(string, the_name);
  logger->comment(string);
  delete string;
}

static void save_tfun_anyway_cb( Widget w, caddr_t data_in, 
				  caddr_t call_data )
{
  file_write_warning_data *data= (file_write_warning_data*)data_in;

  save_tfun_to_file( tfun_handler, data->fname );

  XtUnmanageChild(data->to_be_closed);
  delete data;
}

static void cancel_save_tfun_cb( Widget w, 
				  caddr_t data_in, 
				  caddr_t call_data )
{
  file_write_warning_data *data= (file_write_warning_data*)data_in;
  // Just pop down the warning dialog
  XtUnmanageChild(data->to_be_closed);
  delete data;
}

static void save_tfun_file_cb (Widget w,
			       XtPointer blah,
			       XmFileSelectionBoxCallbackStruct *call_data)
{
  char *the_name, *label_name, *s;
  
  if (!tfun_handler) { // no tfun exists yet
    pop_error_dialog("no_tfun_yet_msg");
    XtUnmanageChild(w);
    return;
  }

  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  for (s = the_name; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  if (! *label_name) {
    pop_error_dialog("invalid_tfun_file_msg");
    return;
  }
  
  if (access(the_name,F_OK)) { // file does not already exist
    XtUnmanageChild(w);
    
    save_tfun_to_file( tfun_handler, the_name );
  }
  else {
    file_write_warning_data *data= new file_write_warning_data;
    data->fname= the_name;
    data->to_be_closed= w;
    pop_warning_dialog( "save_anyway_msg", 
			save_tfun_anyway_cb, 
			cancel_save_tfun_cb,
			(void *)data );
  }
}

static void open_tfun_file_cb (Widget w,
			       XtPointer blah,
			       XmFileSelectionBoxCallbackStruct *call_data) 
{
  char *the_name, *label_name, *s;
  
  if(! XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,&the_name))
    return;
  
  for (s = the_name; *s; s++)
    if (*s == '/')
      label_name = s+1;
  
  if (! *label_name) {
    pop_error_dialog("invalid_tfun_file_msg");
    return;
  }
  
  XtUnmanageChild(w);
  XmUpdateDisplay(w);
  
  if (xtfun_handler) {
    // An old tfun is being replaced
    baseTransferFunction *new_tfun= create_tfun(the_name);
    if (new_tfun) { 
      main_tfun= new_tfun;
      update_sliceviewers();
    }
  }
  else {
    // New tfun
    tfunhandler_delete_cb( xtfun_handler );
    baseTransferFunction *new_tfun= create_tfun(the_name);
    if (new_tfun) main_tfun= new_tfun;
  }

  logger->comment("Tfun loaded");
}

int load_tfun( const char* fname )
// This routine is used by scripts wishing to load a transfer function
{
  if (!datavol_list.head()) { // must read a datavolume first
    char *msgbuf= new char[strlen(fname) + 64];
    sprintf(msgbuf,"Datavol not loaded; Tfun load of %s failed!",fname);
    logger->comment(msgbuf);
    delete [] msgbuf;
    return 0;
  }

  tfunhandler_delete_cb( xtfun_handler );
  baseTransferFunction *new_tfun= create_tfun(fname);
  if (new_tfun) {
    main_tfun= new_tfun;
    char *msgbuf= new char[strlen(fname) + 32];
    sprintf(msgbuf,"Tfun %s loaded",fname);
    logger->comment(msgbuf);
    delete [] msgbuf;
    return 1;
  }
  else {
    char *msgbuf= new char[strlen(fname) + 32];
    sprintf(msgbuf,"Tfun load of %s failed!",fname);
    logger->comment(msgbuf);
    delete [] msgbuf;
    return 0;
  }
}

static void edit_tfun_cb( Widget w, XtPointer unused, XtPointer unused2 )
{
  if (!datavol_list.head()) { // Must read datavolume first
    pop_error_dialog("no_dataset_loaded_msg");
    return;
  }

  if (xtfun_handler) // a tfun has been loaded, just pop the widget
    XtManageChild( xtfun_handler->get_widget() );
  else { // open the default file
    char *full_tfun_fname= add_default_path( DEFAULT_TFUN_FNAME );
    baseTransferFunction *new_tfun= create_tfun( full_tfun_fname );
    if (new_tfun) {
      delete main_tfun;
      main_tfun= new_tfun;
    }
    else {
      fprintf(stderr,"edit_tfun_cb: unable to load default tfun file <%s>!\n",
	      full_tfun_fname);
    }
    delete full_tfun_fname;
  }
}

static void create_x_sliceviewer_cb( Widget w, XtPointer unused, 
				     XtPointer unused2 )
{
  if (!main_tfun) {
    pop_error_dialog("no_tfun_yet_msg");
    return;
  }

  GridInfo grid= datavol_list.head()->dvol->gridinfo();
  tfun_handler->refit_to_volume(&grid);

  DataVolume** dvol_table= build_local_dvol_table();

#ifdef LOCAL_VREN
  slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
					      grid, main_tfun,
					      main_tfun->ndata(), dvol_table,
					      0,
					      delete_slice_viewer, 0 ) );
#else
  if (num_remote_procs == 0) {
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						0,
						delete_slice_viewer, 0 ) );
  }
  else {
#ifdef SC94
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						0,
						delete_slice_viewer, 0 ) );
#else
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						0,
						delete_slice_viewer, 1 ) );
#endif
  }
#endif

  delete [] dvol_table;
}

static void create_y_sliceviewer_cb( Widget w, XtPointer unused, 
				     XtPointer unused2 )
{
  if (!main_tfun) {
    pop_error_dialog("no_tfun_yet_msg");
    return;
  }

  GridInfo grid= datavol_list.head()->dvol->gridinfo();
  tfun_handler->refit_to_volume(&grid);

  DataVolume** dvol_table= build_local_dvol_table();

#ifdef LOCAL_VREN
  slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
					      grid, main_tfun,
					      main_tfun->ndata(), dvol_table,
					      1,
					      delete_slice_viewer, 0 ) );
#else
  if (num_remote_procs == 0) {
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						1,
						delete_slice_viewer, 0 ) );
  }
  else {
#ifdef SC94
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						1,
						delete_slice_viewer, 0 ) );
#else
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						1,
						delete_slice_viewer, 1 ) );
#endif
  }
#endif

  delete [] dvol_table;
}

static void create_z_sliceviewer_cb( Widget w, XtPointer unused, 
				     XtPointer unused2 )
{
  if (!main_tfun) {
    pop_error_dialog("no_tfun_yet_msg");
    return;
  }

  GridInfo grid= datavol_list.head()->dvol->gridinfo();
  tfun_handler->refit_to_volume(&grid);

  DataVolume** dvol_table= build_local_dvol_table();

#ifdef LOCAL_VREN
  slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
					      grid, main_tfun,
					      main_tfun->ndata(), dvol_table,
					      2,
					      delete_slice_viewer, 0 ) );
#else
  if (num_remote_procs==0) {
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						2,
						delete_slice_viewer, 0 ) );
  }
  else {
#ifdef SC94
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						2,
						delete_slice_viewer, 0 ) );
#else
    slice_viewer_list.insert( new XSliceViewer( app_shell, mrm_id,
						grid, main_tfun,
						main_tfun->ndata(), dvol_table,
						2,
						delete_slice_viewer, 1 ) );
#endif
  }
#endif

  delete [] dvol_table;
}

int precalculate_svol()
{
  if (!xtfun_handler) { // No transfer function yet loaded
    pop_error_dialog("no_tfun_loaded_msg");
    return 0;
  }

  if (!main_tfun) main_tfun= xtfun_handler->get_tfun();
  update_tfuns();
  
  logger->comment("Precalculations begin");
  
  DataVolume** dvol_table;
  dvol_table= build_remote_dvol_table();
  if (main_svol) {
    // An appropriate svol exists;  need only regen with new tfun
    main_svol->regenerate( *registered_tfun, 
			   registered_tfun->ndata(), dvol_table );
  }
  else {
    GridInfo grid= datavol_list.head()->dvol->gridinfo();
    main_svol= main_vren->create_sample_volume( grid, *registered_tfun, 
						registered_tfun->ndata(), 
						dvol_table );
    main_svol->set_size_scale( current_opac_scale );
  }
  delete [] dvol_table;
  
  logger->comment("Precalculations complete");

  tfun_changed_flag= 0;
  return 1;
}

void vfleet_tfun_create(Widget w,
			int *id,
			unsigned long *reason) 
{
  switch (*id) {
  case k_table_tfun_open_id:
    tfun_open_id= w;
    break;
  case k_table_tfun_id:
    tfun_dlog_id= w;
    break;
  case k_table_tfun_save_id:
    tfun_save_id= w;
    break;
  }
}

void vfleet_tfun_open( Widget w, int *tag, caddr_t cb )
{
  switch (*tag) {
  case k_table_tfun_open_id:
    if (datavol_list.head()) XtManageChild( tfun_open_id );
    else { // Must read datavolume first
      pop_error_dialog("no_dataset_loaded_msg");
    }
    break;
  case k_table_tfun_save_id:
    XtManageChild( tfun_save_id );
    break;
  case k_table_tfun_id:
    if (datavol_list.head()) XtManageChild( tfun_dlog_id );
    else { // Must read datavolume first
      pop_error_dialog("no_dataset_loaded_msg");
    }
    break;
  }
}

static MRMRegisterArg mrm_names[] = {
  {"open_tfun_file_cb", (caddr_t) open_tfun_file_cb},
  {"save_tfun_file_cb", (caddr_t) save_tfun_file_cb},
  {"save_tfun_anyway_cb", (caddr_t) save_tfun_anyway_cb},
  {"cancel_save_tfun_cb", (caddr_t) cancel_save_tfun_cb},
  {"edit_tfun_cb", (caddr_t) edit_tfun_cb},
  {"create_x_sliceviewer_cb", (caddr_t)create_x_sliceviewer_cb},
  {"create_y_sliceviewer_cb", (caddr_t)create_y_sliceviewer_cb},
  {"create_z_sliceviewer_cb", (caddr_t)create_z_sliceviewer_cb},
};

void vfleet_tfun_reg()
// Register appropriate callback names
{
  MrmRegisterNames(mrm_names, XtNumber(mrm_names));
}

void vfleet_tfun_cleanup()
{
  delete_all_sliceviewers();
  delete main_tfun;
  delete registered_tfun;
}
