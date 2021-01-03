/****************************************************************************
 * xtfunhandler.cc
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
#include <stdlib.h>
#include <string.h>
#ifdef ATTCC
#include <osfcn.h>
#define F_OK 0
#else
#include <unistd.h>
#endif

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/TextF.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>
#include <Mrm/MrmAppl.h>

#include "vfleet.h"

int baseXTfunHandler::initialized= 0;
int baseXTfunHandler::id_seed= 1;  // skip zero; error condition uses it
Widget baseXTfunHandler::add_file_selection_dialog= NULL;
int bboxXTfunHandler::initialized= 0;
int sumXTfunHandler::initialized= 0;
int tableXTfunHandler::initialized= 0;
int blockXTfunHandler::initialized= 0;
int maskXTfunHandler::initialized= 0;
int maskXTfunHandler::most_recent_button_press= 0;

dList<baseXTfunHandler::object_lookup_struct*> baseXTfunHandler::instance_list;
sList<MrmRegisterArg*> baseXTfunHandler::object_name_stack;

GC tableXTfunHandler::r_gc= NULL;
GC tableXTfunHandler::g_gc= NULL;
GC tableXTfunHandler::b_gc= NULL;
GC tableXTfunHandler::a_gc= NULL;
GC tableXTfunHandler::prev_gc= NULL;

static const char* DEFAULT_TABLE_TFUN_FNAME= "default_table_tfun.tfn";
static const char* DEFAULT_GRAD_TFUN_FNAME= "default_grad_tfun.tfn";
static const char* DEFAULT_SUM_TFUN_FNAME= "default_sum_tfun.tfn";
static const char* DEFAULT_SSUM_TFUN_FNAME= "default_ssum_tfun.tfn";
static const char* DEFAULT_BBOX_TFUN_FNAME= "default_bbox_tfun.tfn";
static const char* DEFAULT_BLOCK_TFUN_FNAME= "default_block_tfun.tfn";
static const char* DEFAULT_MASK_TFUN_FNAME= "default_mask_tfun.tfn";

baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
				     MrmHierarchy mrm_id,
				     FILE *infile, const GridInfo* grid_in,
				     Widget neighbor )
{
  int tfuntype;

  if ( fscanf( infile, "%d", &tfuntype ) != 1 ) {
    fprintf(stderr,"XTfunHandler_load: bad file!\n");
    return NULL;
  }

  if (baseTfunHandler::get_debug()) 
    fprintf(stderr,"XTfunHandler_load 1: type %d\n",tfuntype);

  baseXTfunHandler *result;

  switch (tfuntype) {
  case BBOX_TFUN:
    result= new bboxXTfunHandler( parent_in, mrm_id, infile, grid_in, 
				  neighbor );
    break;
  case SUM_TFUN:
    result= new sumXTfunHandler( parent_in, mrm_id, infile, grid_in, 
				 neighbor );
    break;
  case TABLE_TFUN:
    result= new tableXTfunHandler( parent_in, mrm_id, infile, grid_in,
				   NULL, NULL, neighbor );
    break;
  case GRADTABLE_TFUN:
    result= new gradtableXTfunHandler( parent_in, mrm_id, infile, grid_in,
				       neighbor );
    break;
  case SSUM_TFUN:
    result= new ssumXTfunHandler( parent_in, mrm_id, infile, grid_in,
				  neighbor );
    break;
  case BLOCK_TFUN:
    result= new blockXTfunHandler( parent_in, mrm_id, infile, grid_in,
				   neighbor );
    break;
  case MASK_TFUN:
    result= new maskXTfunHandler( parent_in, mrm_id, infile, grid_in,
				  neighbor );
    break;
  default:
    fprintf(stderr,"XTfunHandler_load: unknown tfun type %d!\n",tfuntype);
    result= NULL;
    break;
  }

  if (baseTfunHandler::get_debug()) {
    fprintf(stderr,"load 1: type %d complete; %lx\n",tfuntype,(long)result);
    if (result) fprintf(stderr,"              %lx\n",(long)(result->get_tfun()));
  }

  if (result && result->get_tfun()) return result;
  else {
    return NULL;
  }
}

baseXTfunHandler* XTfunHandler_load( void (*update_cb_in)(baseXTfunHandler*),
				     void (*deletion_cb_in)(baseXTfunHandler*),
				     MrmHierarchy mrm_id,
				     FILE *infile, const GridInfo* grid_in,
				     Widget neighbor )
{
  int tfuntype;

  if ( fscanf( infile, "%d", &tfuntype ) != 1 ) {
    fprintf(stderr,"XTfunHandler_load: bad file!\n");
    return NULL;
  }

  if (baseTfunHandler::get_debug()) 
    fprintf(stderr,"XTfunHandler_load 2: type %d\n",tfuntype);
  baseXTfunHandler *result;

  switch (tfuntype) {
  case BBOX_TFUN:
    result= new bboxXTfunHandler( update_cb_in, deletion_cb_in, 
				  mrm_id, infile, grid_in, neighbor );
    break;
  case SUM_TFUN:
    result= new sumXTfunHandler( update_cb_in, deletion_cb_in, 
				 mrm_id, infile, grid_in, neighbor );
    break;
  case TABLE_TFUN:
    result= new tableXTfunHandler( update_cb_in, deletion_cb_in, 
				   mrm_id, infile, grid_in,
				   NULL, NULL, neighbor );
    break;
  case GRADTABLE_TFUN:
    result= new gradtableXTfunHandler( update_cb_in, deletion_cb_in, 
				       mrm_id, infile, grid_in, neighbor );
    break;
  case SSUM_TFUN:
       result= new ssumXTfunHandler( update_cb_in, deletion_cb_in, 
				     mrm_id, infile, grid_in, neighbor );
       break;
  case BLOCK_TFUN:
    result= new blockXTfunHandler( update_cb_in, deletion_cb_in, 
				   mrm_id, infile, grid_in, neighbor );
    break;
  case MASK_TFUN:
    result= new maskXTfunHandler( update_cb_in, deletion_cb_in,
				  mrm_id, infile, grid_in, neighbor );
    break;
  default:
    fprintf(stderr,"XTfunHandler_load: unknown tfun type %d!\n",tfuntype);
    result= NULL;
    break;
  }

  if (baseTfunHandler::get_debug()) {
    fprintf(stderr,"load 2: type %d complete; %lx\n",tfuntype,(long)result);
    if (result) fprintf(stderr,"              %lx\n",(long)(result->get_tfun()));
  }

  if (result && result->get_tfun()) return result;
  else {
    return NULL;
  }
}


baseXTfunHandler::baseXTfunHandler( baseXTfunHandler* parent_in, 
				    MrmHierarchy mrm_id_in, 
				    FILE* infile, const GridInfo* grid_in )
  : baseTfunHandler(infile, grid_in)
{
  if (debug) 
    fprintf(stderr,
	    "baseXTfunHandler::baseXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);

  if (!initialized) {
    MRMRegisterArg mrm_names[10];
    mrm_names[0].name= "xtfunhandler_create_cb";
    mrm_names[0].value= (caddr_t)baseXTfunHandler::create_cb;
    mrm_names[1].name= "xtfunhandler_expose_cb";
    mrm_names[1].value= (caddr_t)baseXTfunHandler::expose_cb;
    mrm_names[2].name= "xtfunhandler_set_edit_color_cb";
    mrm_names[2].value= (caddr_t)baseXTfunHandler::set_edit_color_cb;
    mrm_names[3].name= "xtfunhandler_set_cb";
    mrm_names[3].value= (caddr_t)baseXTfunHandler::set_cb;
    mrm_names[4].name= "xtfunhandler_reset_cb";
    mrm_names[4].value= (caddr_t)baseXTfunHandler::reset_cb;
    mrm_names[5].name= "xtfunhandler_delete_cb";
    mrm_names[5].value= (caddr_t)baseXTfunHandler::delete_cb;
    mrm_names[6].name= "xtfunhandler_save_cb";
    mrm_names[6].value= (caddr_t)baseXTfunHandler::save_cb;
    mrm_names[7].name= "xtfunhandler_save_file_cb";
    mrm_names[7].value= (caddr_t)baseXTfunHandler::save_file_cb;
    mrm_names[8].name= "xtfunhandler_add_cb";
    mrm_names[8].value= (caddr_t)baseXTfunHandler::add_cb;
    mrm_names[9].name= "xtfunhandler_add_file_cb";
    mrm_names[9].value= (caddr_t)baseXTfunHandler::add_file_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    initialized= 1;
  }

  mrm_id= mrm_id_in;
  parent= parent_in;
  update_cb= NULL;
  delete_request_cb= NULL;
  widget= NULL;
  save_file_selection_dialog= NULL;
  weight_text_widget= NULL;
  id= id_seed++;
  instance_list.insert( new object_lookup_struct( id, this ) );

  if (debug) 
    fprintf(stderr,
	    "baseXTfunHandler::baseXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

baseXTfunHandler::baseXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				    void (deletion_cb_in)(baseXTfunHandler*),
				    MrmHierarchy mrm_id_in, 
				    FILE* infile, const GridInfo* grid_in )
  : baseTfunHandler(infile, grid_in)
{
  if (!initialized) {
    MRMRegisterArg mrm_names[10];
    mrm_names[0].name= "xtfunhandler_create_cb";
    mrm_names[0].value= (caddr_t)baseXTfunHandler::create_cb;
    mrm_names[1].name= "xtfunhandler_expose_cb";
    mrm_names[1].value= (caddr_t)baseXTfunHandler::expose_cb;
    mrm_names[2].name= "xtfunhandler_set_edit_color_cb";
    mrm_names[2].value= (caddr_t)baseXTfunHandler::set_edit_color_cb;
    mrm_names[3].name= "xtfunhandler_set_cb";
    mrm_names[3].value= (caddr_t)baseXTfunHandler::set_cb;
    mrm_names[4].name= "xtfunhandler_reset_cb";
    mrm_names[4].value= (caddr_t)baseXTfunHandler::reset_cb;
    mrm_names[5].name= "xtfunhandler_delete_cb";
    mrm_names[5].value= (caddr_t)baseXTfunHandler::delete_cb;
    mrm_names[6].name= "xtfunhandler_save_cb";
    mrm_names[6].value= (caddr_t)baseXTfunHandler::save_cb;
    mrm_names[7].name= "xtfunhandler_save_file_cb";
    mrm_names[7].value= (caddr_t)baseXTfunHandler::save_file_cb;
    mrm_names[8].name= "xtfunhandler_add_cb";
    mrm_names[8].value= (caddr_t)baseXTfunHandler::add_cb;
    mrm_names[9].name= "xtfunhandler_add_file_cb";
    mrm_names[9].value= (caddr_t)baseXTfunHandler::add_file_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    initialized= 1;
  }
  
  mrm_id= mrm_id_in;
  parent= NULL;
  update_cb= update_cb_in;
  delete_request_cb= deletion_cb_in;
  widget= NULL;
  save_file_selection_dialog= NULL;
  weight_text_widget= NULL;
  id= id_seed++;
  instance_list.insert( new object_lookup_struct( id, this ) );
}

baseXTfunHandler::~baseXTfunHandler()
{
  // Cut this one out of the instance list
  dList_iter<object_lookup_struct*> iter(instance_list);
  object_lookup_struct** this_struct;
  while (this_struct= iter.next()) {
    if ((*this_struct)->handler==this) {
      delete *this_struct;
      iter.remove();
      break;
    }
  }
  if (!this_struct)
    fprintf(stderr,
	    "baseXTfunHandler destructor: instance list removal failed!\n");

  if (parent) parent->goodbye(this);
  if (save_file_selection_dialog)
    XtDestroyWidget(save_file_selection_dialog);
  if (widget) {
    XtDestroyWidget(widget);
  }
}

Widget baseXTfunHandler::get_enclosing_widget( baseXTfunHandler* asked_by ) 
     const
{
  return widget;
}

void baseXTfunHandler::goodbye( baseXTfunHandler* asked_by )
{
  // Nothing to do in the base case
}

void baseXTfunHandler::push_object_name()
{
  MrmRegisterArg* name= new MrmRegisterArg;
  name->name= "owner_object";
  name->value= (caddr_t)id;
  MrmRegisterNamesInHierarchy(mrm_id, name, 1);
  object_name_stack.insert(name);
}

void baseXTfunHandler::pop_object_name()
{
  delete object_name_stack.pop();
  if (object_name_stack.head())
    MrmRegisterNamesInHierarchy(mrm_id, object_name_stack.head(), 1);
}

baseXTfunHandler* baseXTfunHandler::get_object( Widget w )
{
  // This recovers the object pointer registered with push_object_name
  // from the widget
  Arg arg;
  long object_id;
  XtSetArg( arg, XmNuserData, &object_id );
  XtGetValues( w, &arg, 1 );
  if (!object_id) {
    fprintf(stderr,"baseXTfunHandler::get_object: lookup failed!\n");
    return NULL;
  }
  dList_iter<object_lookup_struct*> iter(instance_list);
  object_lookup_struct** this_struct;
  while (this_struct= iter.next()) {
    if ((*this_struct)->id == object_id) {
      return (*this_struct)->handler;
    }
  }
  return NULL;
}

void baseXTfunHandler::register_weight( const float weight )
{
  static char buf[16];
  // This sticks a pointer to this XTfunHandler into the mrm hierarchy
  MrmRegisterArg names;
  sprintf(buf,"%-7.4f",weight);
  names.name= "owner_weight";
  names.value= (caddr_t)buf;
  MrmRegisterNamesInHierarchy(mrm_id, &names, 1);
}

Widget baseXTfunHandler::update_datavol_menu( Widget w, Widget parent,
					      dvol_struct* selected,
					      Widget set_target,
					      XtCallbackProc button_cb,
					      sList<Widget>* button_list )
{
  // Some children need the ability to rebuild their datavolume
  // selection menu when the available datavolume list changes.
  // This stuff assumes that the menu is actually an OptionMenu.
  
  Arg args[3];
  char name_buf[16];

  if (parent) { // remake from scratch
    int n= 0;

    // Clear out old buttons
    while (button_list->head()) XtDestroyWidget(button_list->pop());

    dList_iter<dvol_struct*> iter(datavol_list);
    dvol_struct** this_struct;
    Widget button;
    int count= 0;

    while (this_struct= iter.next()) {
      n= 0;
      sprintf(name_buf,"button_%3d",count);
      XtSetArg( args[n], XmNlabelString, (*this_struct)->xstring ); n++;
      XtSetArg( args[n], XmNuserData, this ); n++;
      button= XmCreatePushButtonGadget( w, name_buf, args, n );
      if (button_cb) XtAddCallback( button, XmNactivateCallback,
				    button_cb, (XtPointer)count );
      button_list->append(button);
      XtManageChild(button);
      if (selected && set_target && (*this_struct == selected)) {
	n= 0;
	XtSetArg( args[n], XmNmenuHistory, button ); n++;
	XtSetValues( set_target, args, n );
      }
      count++;
    }    

    return w;
  }
  else { // Creation time- fill and return the existing widget
    dList_iter<dvol_struct*> iter(datavol_list);
    dvol_struct** this_struct;
    Widget button;
    int count= 0;

    while (this_struct= iter.next()) {
      int n= 0;
      sprintf(name_buf,"button_%3d",count);
      XtSetArg( args[n], XmNlabelString, (*this_struct)->xstring ); n++;
      XtSetArg( args[n], XmNuserData, this ); n++;
      button= XmCreatePushButtonGadget( w, name_buf, args, n );
      if (button_cb) XtAddCallback( button, XmNactivateCallback,
				    button_cb, (XtPointer)count );
      button_list->append(button);
      XtManageChild(button);
      if (selected && set_target && (*this_struct == selected)) {
	n= 0;
	XtSetArg( args[n], XmNmenuHistory, button ); n++;
	XtSetValues( set_target, args, n );
      }
      count++;
    }

    return w;
  }
}

int baseXTfunHandler::get_selected_dvol_index( Widget dvol_rowcol )
{
  // This routine provides an easy way for a derived class to find the
  // selected data volume in a list of data volumes.

  Arg args[3];
  int n;
  Widget selected_button;
  XmString xstring;

  n= 0;
  XtSetArg( args[n], XmNmenuHistory, &selected_button ); n++;
  XtGetValues( dvol_rowcol, args, 1 );

  n= 0;
  XtSetArg( args[n], XmNlabelString, &xstring ); n++;
  XtGetValues( selected_button, args, 1 );

  dList_iter<dvol_struct*> iter(datavol_list);
  dvol_struct** this_struct;
  int result= 0;
  while (this_struct= iter.next()) {
    if (XmStringCompare((*this_struct)->xstring, xstring)) 
      return result;
    result++;
  }

  // Failed to find a match!
  fprintf(stderr,"baseXTfunHandler::get_selected_dvol: no dvol selected!\n");
  return 0;
}

void baseXTfunHandler::build( Widget neighbor, char* type_name,
			      char* dialog_name, char* widget_name )
{
  // Build the widget as a child of the parent, with a pointer back here
  push_object_name();
  MrmType mrm_class;
  if (!parent) { // top level
    register_weight(1.0);
    if ( !MrmFetchWidget(mrm_id, dialog_name, app_shell,
			 &widget, &mrm_class) || !widget ) {
	fprintf(stderr,
		"%sXTfunHandler::%sXTfunHandler: can't get UIL resource!\n",
		type_name,type_name);
	pop_object_name();
	return;
    }
  }
  else {
    // Weight already registered by parent
    if (neighbor) { // The neighbor needs to be our top attachment
	Arg args[MAX_ARGS];
	int n= 0;
	XtSetArg( args[n], XmNtopAttachment, XmATTACH_WIDGET ); n++;
	XtSetArg( args[n], XmNtopWidget, neighbor ); n++; 
	if ( !MrmFetchWidgetOverride(mrm_id, widget_name, 
				     parent->get_enclosing_widget(this),
				     NULL, args, n,
				     &widget, &mrm_class) || !widget ) {
	    fprintf(stderr,
		 "%sXTfunHandler::%sXTfunHandler: can't get UIL resource!\n",
		    type_name,type_name);
	    pop_object_name();
	    return;
	}
#ifdef never
	fprintf(stderr,"%s, has neighbor %ld, widget %ld, parent %ld %ld\n",
		type_name,(long)neighbor,
		(long)widget,(long)(parent->get_widget()),
		(long)(parent->get_enclosing_widget(this)));
#endif
    }
    else {
      if ( !MrmFetchWidget(mrm_id, widget_name, 
			   parent->get_enclosing_widget(this),
			   &widget, &mrm_class) || !widget ) {
	  fprintf(stderr,
		  "%sXTfunHandler::%sXTfunHandler: can't get UIL resource!\n",
		  type_name,type_name);
	  pop_object_name();
	  return;
      }
#ifdef never
      fprintf(stderr,"%s, no neighbor, widget %ld, parent %ld %ld\n",type_name,
	      (long)widget,(long)(parent->get_widget()),
	      (long)(parent->get_enclosing_widget(this)));
#endif
    }
  }
  pop_object_name();
}

void baseXTfunHandler::create_cb( Widget w, int *id, unsigned long *reason )
{
  baseXTfunHandler* handler= get_object(w);
  if (handler) handler->create( w, id, reason );
}

void baseXTfunHandler::create( Widget w, int *id, unsigned long *reason )
{
  if (debug) fprintf(stderr,"baseXTfunHandler::create %lx %d\n",(long)this,*id);
  if (*id==0) {
    weight_text_widget= w;
    if (!(get_parent())) XtSetSensitive(w, False); // top weight unsettable
  }
  else
    fprintf(stderr,"baseXTfunHandler::create: out of sync with uid!\n");
  if (debug) fprintf(stderr,"baseXTfunHandler::create exit %lx %d\n",
		     (long)this,*id);
}

void baseXTfunHandler::expose_cb( Widget w, int *id, unsigned long *reason )
{
  // Beware of paradoxical conditions in which an expose happens for
  // an XTfunHandler while it is in the process of being destroyed
  baseXTfunHandler* handler= get_object(w);
  if (handler) handler->expose( w, id, reason );
}

void baseXTfunHandler::set_edit_color_cb( Widget w, int *id,
					  unsigned long *reason )
{
  get_object(w)->set_edit_color( w, id, reason );
}

void baseXTfunHandler::set_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->set( w, id, reason );
}

void baseXTfunHandler::set( Widget w, int *id, unsigned long *reason )
{
  if (parent && weight_text_widget ) {
    float new_weight;
    char *weight_string= XmTextFieldGetString( weight_text_widget );
    sscanf(weight_string,"%f",&new_weight);
    parent->set_weight(this, new_weight);
    XtFree(weight_string);
  }
  child_changed();
}

void baseXTfunHandler::reset_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->reset( w, id, reason );
}

void baseXTfunHandler::reset( Widget w, int* id, unsigned long* reason )
{
  if (parent && weight_text_widget ) {
    char weight_string[16];
    sprintf(weight_string, "%-7.4f", parent->get_weight(this));
    XmTextFieldSetString( weight_text_widget, weight_string );
  }
}

void baseXTfunHandler::save_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->save( w, id, reason );
}

void baseXTfunHandler::save( Widget w, int *id, unsigned long *reason )
{
  // Create the file selection dialog if none exists
  if (!save_file_selection_dialog) {
    push_object_name();
    MrmType mrm_class;
    if ( !MrmFetchWidget(mrm_id, "tfun_subpart_save_file_box", app_shell,
			 &save_file_selection_dialog, &mrm_class) 
	 || !save_file_selection_dialog ) {
      fprintf(stderr,
	      "baseXTfunHandler::save: can't get UIL resource!\n");
      pop_object_name();
      return;
    }
    pop_object_name();
  }
  XtManageChild(save_file_selection_dialog);
}

void baseXTfunHandler::save_file_cb(Widget w, int *id, 
				  XmFileSelectionBoxCallbackStruct *call_data)
{
  get_object(w)->save_file( w, id, call_data );
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
}

static void save_tfun_anyway_cb( Widget w, caddr_t data_in, 
				  caddr_t call_data )
{
  xtfun_file_write_warning_data *data= (xtfun_file_write_warning_data*)data_in;

  save_tfun_to_file( data->handler, data->fname );

  XtUnmanageChild(data->to_be_closed);
  delete data;
}

static void cancel_save_tfun_cb( Widget w, 
				  caddr_t data_in, 
				  caddr_t call_data )
{
  xtfun_file_write_warning_data *data= (xtfun_file_write_warning_data*)data_in;
  // Just pop down the warning dialog
  XtUnmanageChild(data->to_be_closed);
  delete data;
}

void baseXTfunHandler::save_file( Widget w, int *id, 
				  XmFileSelectionBoxCallbackStruct *call_data )
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
  
  if (access(the_name,F_OK)) { // file does not already exist
    save_tfun_to_file( this, the_name );
    XtUnmanageChild(w);
  }
  else {
    xtfun_file_write_warning_data *data= new xtfun_file_write_warning_data;
    data->fname= the_name;
    data->to_be_closed= w;
    data->handler= this;
    pop_warning_dialog( "save_anyway_msg", 
			save_tfun_anyway_cb, 
			cancel_save_tfun_cb,
			(void *)data );
  }
}

void baseXTfunHandler::delete_tfun_anyway_cb( Widget w, caddr_t data_in, 
					      caddr_t call_data )
{
  delete_warning_data *data= (delete_warning_data*)data_in;

  baseXTfunHandler *target= (baseXTfunHandler*)(data->info);
  baseXTfunHandler *parent= target->get_parent();
  if (parent) {
    delete target;
    parent->child_changed();
  }
  else (*(target->delete_request_cb))(target);

  delete data;
}

static void cancel_delete_tfun_cb( Widget w, 
				  caddr_t data_in, 
				  caddr_t call_data )
{
  delete_warning_data *data= (delete_warning_data*)data_in;
  // Do nothing
  delete data;
}

void baseXTfunHandler::delete_cb( Widget w, int *id, unsigned long *reason )
{
  delete_warning_data *data= new delete_warning_data;
  data->info= (void *)(get_object(w));
  data->to_be_closed= NULL;
  pop_warning_dialog( "delete_tfun_anyway_msg", 
		      delete_tfun_anyway_cb, 
		      cancel_delete_tfun_cb,
		      (void *)data );
}

void baseXTfunHandler::add_cb( Widget w, int *id, unsigned long *reason )
{
  get_object(w)->add( w, id, reason );
}

void baseXTfunHandler::add( Widget w, int *id, unsigned long *reason )
{
  fprintf(stderr,"baseXTfunHandler::add method called!\n");
}

void baseXTfunHandler::add_file_cb( Widget w, int *id, 
				   XmFileSelectionBoxCallbackStruct *call_data)
{
  get_object(w)->add_file( w, id, call_data );
}

void baseXTfunHandler::add_file( Widget w, int *id, 
				 XmFileSelectionBoxCallbackStruct *call_data )
{
  fprintf(stderr,"baseXTfunHandler::add_file method called!\n");
}

void baseXTfunHandler::child_changed()
{
  if (parent) parent->child_changed();
  else if (update_cb) (*update_cb)(this);
}

baseXTfunHandler* baseXTfunHandler::load_tfun_by_number(const int id,
							const GridInfo* 
							   grid_in,
							baseXTfunHandler*
							   parent,
							Widget neighbor)
{
  FILE *tfun_file= NULL;

  if (id == 4) { // User selects load file
    // Have to recreate dialog if it was created by someone else
    if (add_file_selection_dialog 
	&& (get_object(add_file_selection_dialog) != parent)) {
      XtDestroyWidget(add_file_selection_dialog);
      add_file_selection_dialog= NULL;
    }
    if (!add_file_selection_dialog) {
      // Load it from the UID
      parent->push_object_name();
      MrmType mrm_class;
      if ( !MrmFetchWidget(parent->mrm_id, "add_tfun_file_box", app_shell,
			   &add_file_selection_dialog, &mrm_class) 
	   || !add_file_selection_dialog ) {
	fprintf(stderr,
     "baseXTfunHandler::load_tfun_by_number: can't get UIL resource!\n");
	parent->pop_object_name();
	return NULL;
      }
      parent->pop_object_name();
    }
    XtManageChild(add_file_selection_dialog);
    return NULL;
  }
  else switch (id) {
  case 0: 
    {
      tfun_file= fopen_read_default_dir( DEFAULT_TABLE_TFUN_FNAME );
    }
    break;
  case 1:
    {
      tfun_file= fopen_read_default_dir( DEFAULT_GRAD_TFUN_FNAME );
    }
    break;
  case 2:
    {
      tfun_file= fopen_read_default_dir( DEFAULT_SUM_TFUN_FNAME );
    }
    break;
  case 3:
    {
      tfun_file= fopen_read_default_dir( DEFAULT_BBOX_TFUN_FNAME );
    }
    break;
  case 8:
    {
      tfun_file= fopen_read_default_dir( DEFAULT_SSUM_TFUN_FNAME );
    }
    break;
  case 9:
    {
      tfun_file= fopen_read_default_dir( DEFAULT_MASK_TFUN_FNAME );
    }
    break;
  case 10:
    {
      tfun_file= fopen_read_default_dir( DEFAULT_BLOCK_TFUN_FNAME );
    }
    break;
  default: // out of sync with UID file
    fprintf(stderr,
  "Error: baseXTfunHandler::load_tfun_by_number(): unknown request id %d!\n",
	    id);
  }

  if (!tfun_file) {
    pop_error_dialog("invalid_tfun_file_msg");
    return NULL;
  }

  baseXTfunHandler* result= XTfunHandler_load( parent, parent->mrm_id, 
					       tfun_file, grid_in, neighbor );
  fclose(tfun_file);
  return result;
}

baseXTfunHandler::WinSize baseXTfunHandler::get_widget_size( Widget w )
{
  WinSize winsize;

  if (w) {
    Arg args[2];
    int n= 0;
    XtSetArg( args[n], XmNwidth, &winsize.x ); n++;
    XtSetArg( args[n], XmNheight, &winsize.y ); n++;
    XtGetValues( w, args, n );
  }
  else {
    winsize.x= winsize.y= 0;
  }

  return winsize;
}

void baseXTfunHandler::set_widget_size( Widget w, 
				       baseXTfunHandler::WinSize& winsize )
{  
  if (w) {
    Arg args[2];
    int n= 0;
    XtSetArg( args[n], XmNwidth, winsize.x ); n++;
    XtSetArg( args[n], XmNheight, winsize.y ); n++;
    XtSetValues( w, args, n );
  }
  else fprintf(stderr,"baseXTfunHandler::set_widget_size: null widget!\n");
}

void baseXTfunHandler::resize()
{
  if (parent) parent->resize();  // pass the buck
  else { // this is top handler; manage resize sequence
    WinSize old_pref_size;
    do {
      old_pref_size= preferred_size;
      recalc_preferred_size();
      if (debug) 
	fprintf(stderr,"baseXTfunHandler::resize loop: (%d %d)->(%d %d)\n",
		old_pref_size.x,old_pref_size.y,
		preferred_size.x, preferred_size.y);
      set_preferred_size();
    } while (old_pref_size != preferred_size);
  }
}

baseXTfunHandler::WinSize baseXTfunHandler::recalc_preferred_size()
{
  preferred_size= get_widget_size( widget );
  // Anything smaller than about 150x50 has got to be a size
  // arbitration error on Motif's part
  if (preferred_size.x<150) preferred_size.x= 150;
  if (preferred_size.y<50) preferred_size.y= 50;
  if (debug) 
    fprintf(stderr,"baseXTfunHandler::recalc_preferred_size: %d %d\n",
	    preferred_size.x,preferred_size.y);
  return preferred_size;
}

void baseXTfunHandler::set_preferred_size()
{
  set_widget_size( widget, preferred_size );
}

int baseXTfunHandler::int_from_text_widget( Widget w, 
					    const int min, const int max )
{
  char *txtstring= XmTextFieldGetString( w );
  int result;
  sscanf(txtstring,"%d",&result);
  if ((result<min) || (result>max)) {
    result= (result<min) ? min : ((result>max) ? max : result);
    char fixstring[16];
    sprintf(fixstring, "%-5d", result);
    XmTextFieldSetString( w, fixstring );
  }
  XtFree(txtstring);
  return result;
}

float baseXTfunHandler::float_from_text_widget( Widget w, const float min, 
						const float max )
{
  char *txtstring= XmTextFieldGetString( w );
  float result;
  sscanf(txtstring,"%f",&result);
  if ((result<min) || (result>max)) {
    result= (result<min) ? min : ((result>max) ? max : result);
    char fixstring[16];
    sprintf(fixstring, "%-7.3f", result);
    XmTextFieldSetString( w, fixstring );
  }
  XtFree(txtstring);
  return result;
}

void baseXTfunHandler::ordered_pair_from_text_widgets( int* v1, int* v2,
						       Widget w1, Widget w2,
						       const int min,
						       const int max )
{
  int val1= int_from_text_widget( w1, min, max );
  int val2= int_from_text_widget( w2, min, max );

  if (val2<val1) { // correct mis-ordering
    int tmp= val2;
    val2= val1;
    val1= tmp;

    char fixstring[16];
    sprintf( fixstring, "%-5d", val1 );
    XmTextFieldSetString( w1, fixstring );
    sprintf( fixstring, "%-5d", val2 );
    XmTextFieldSetString( w2, fixstring );
  }
  
  *v1= val1;
  *v2= val2;
}

void baseXTfunHandler::ordered_pair_from_text_widgets( float* v1, float* v2,
						       Widget w1, Widget w2,
						       const float min,
						       const float max )
{
  float val1= float_from_text_widget( w1, min, max );
  float val2= float_from_text_widget( w2, min, max );

  if (val2<val1) { // correct mis-ordering
    float tmp= val2;
    val2= val1;
    val1= tmp;

    char fixstring[16];
    sprintf( fixstring, "%-7.3f", val1 );
    XmTextFieldSetString( w1, fixstring );
    sprintf( fixstring, "%-7.3f", val2 );
    XmTextFieldSetString( w2, fixstring );
  }
  
  *v1= val1;
  *v2= val2;
}

void bboxXTfunHandler::construct()
{
  if (!initialized) {
    // Nothing to do
    initialized= 1;
  }
}

bboxXTfunHandler::bboxXTfunHandler( baseXTfunHandler* parent_in, 
				    MrmHierarchy mrm_id_in,
				    FILE *infile, const GridInfo* grid_in,
				    Widget neighbor )
  : baseTfunHandler(infile, grid_in), 
    baseXTfunHandler(parent_in, mrm_id_in, infile, grid_in), 
    bboxTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "bboxXTfunHandler::bboxXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct();
  if (valid) build( neighbor, "bbox", "bbox_tfun_dlog", "bbox_tfun_topform" );
  if (valid) XtManageChild(widget);
  if (debug) 
    fprintf(stderr,
	    "bboxXTfunHandler::bboxXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

bboxXTfunHandler::bboxXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				    void (*deletion_cb_in)(baseXTfunHandler*),
				    MrmHierarchy mrm_id_in,
				    FILE *infile, const GridInfo* grid_in,
				    Widget neighbor )
: baseTfunHandler(infile, grid_in), 
  baseXTfunHandler(update_cb_in, deletion_cb_in,mrm_id_in, infile, grid_in), 
  bboxTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "bboxXTfunHandler::bboxXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct();
  if (valid) build( neighbor, "bbox", "bbox_tfun_dlog", "bbox_tfun_topform" );
  if (valid) XtManageChild(widget);
  if (debug) 
    fprintf(stderr,
	    "bboxXTfunHandler::bboxXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

void tableXTfunHandler::construct()
{
  dpy= XtDisplay(app_shell);
  if (!initialized) {
    XWindowAttributes watts;
    Pixel r, g, b, a, prev;
    XGCValues r_gcv, g_gcv, b_gcv, a_gcv, prev_gcv;
    XGetWindowAttributes( dpy, XtWindow(app_shell), &watts );

    if (MrmFetchColorLiteral(mrm_id, "red_line_clr", dpy, watts.colormap, &r)
	!= MrmSUCCESS) {
      fprintf(stderr,"tableXTfunHandler: uid lacks color info!\n");
      exit(-1);
    }
    r_gcv.foreground= r;
    if (MrmFetchColorLiteral(mrm_id, "green_line_clr", dpy, watts.colormap, &g)
	!= MrmSUCCESS) {
      fprintf(stderr,"tableXTfunHandler: uid lacks color info!\n");
      exit(-1);
    }
    g_gcv.foreground= g;
    if (MrmFetchColorLiteral(mrm_id, "blue_line_clr", dpy, watts.colormap, &b)
	!= MrmSUCCESS) {
      fprintf(stderr,"tableXTfunHandler: uid lacks color info!\n");
      exit(-1);
    }
    b_gcv.foreground= b;
    if (MrmFetchColorLiteral(mrm_id, "alpha_line_clr", dpy, watts.colormap, &a)
	!= MrmSUCCESS) {
      fprintf(stderr,"tableXTfunHandler: uid lacks color info!\n");
      exit(-1);
    }
    a_gcv.foreground= a;
    if (MrmFetchColorLiteral(mrm_id, "prev_line_clr", dpy, watts.colormap, 
			     &prev)
	!= MrmSUCCESS) {
      fprintf(stderr,"tableXTfunHandler: uid lacks color info!\n");
      exit(-1);
    }
    prev_gcv.foreground= prev;

    unsigned long valuemask= GCForeground;
    r_gc= XCreateGC( dpy, XtWindow(app_shell), valuemask, &r_gcv );
    g_gc= XCreateGC( dpy, XtWindow(app_shell), valuemask, &g_gcv );
    b_gc= XCreateGC( dpy, XtWindow(app_shell), valuemask, &b_gcv );
    a_gc= XCreateGC( dpy, XtWindow(app_shell), valuemask, &a_gcv );
    prev_gc= XCreateGC( dpy, XtWindow(app_shell), valuemask, &prev_gcv );

    initialized= 1;
  }

  ihandler= NULL;
  image= NULL;
  graph_widget= NULL;
  rpoints= new XPoint[256];
  gpoints= new XPoint[256];
  bpoints= new XPoint[256];
  apoints= new XPoint[256];
  reset_color_table();
  edit_which_color= 0;
  drag_start_valid= 0;
  datavol_selection_widget= NULL;
  datavol_option_menu= NULL;
}

tableXTfunHandler::tableXTfunHandler( baseXTfunHandler* parent_in, 
				      MrmHierarchy mrm_id_in,
				      FILE* infile, const GridInfo* grid_in,
				      char* dialog_name, char* widget_name,
				      Widget neighbor )
  : baseTfunHandler(infile, grid_in), 
    baseXTfunHandler(parent_in, mrm_id_in, infile, grid_in), 
    tableTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "tableXTfunHandler::tableXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct();
  if (valid) {
    if (dialog_name) build(neighbor, "table", dialog_name, widget_name);
    else build(neighbor, "table", "table_tfun_dlog", "table_tfun_topform");
  }

  if (valid) {
    XtManageChild(widget);
  }
  if (debug) 
    fprintf(stderr,
	  "tableXTfunHandler::tableXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

tableXTfunHandler::tableXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				      void (*deletion_cb_in)
				             (baseXTfunHandler*),
				      MrmHierarchy mrm_id_in,
				      FILE* infile, const GridInfo* grid_in,
				      char* dialog_name, char* widget_name,
				      Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    baseXTfunHandler(update_cb_in,deletion_cb_in,mrm_id_in,infile,grid_in), 
    tableTfunHandler(infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	    "tableXTfunHandler::tableXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct();
  if (valid) {
    if (dialog_name) build(neighbor, "table", dialog_name, widget_name);
    else build(neighbor, "table", "table_tfun_dlog", "table_tfun_topform");
  }

  if (valid) {
    XtManageChild(widget);
  }
  if (debug) 
    fprintf(stderr,
	 "tableXTfunHandler::tableXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

tableXTfunHandler::~tableXTfunHandler()
{
  delete ihandler;
  delete image;
  delete rpoints;
  delete gpoints;
  delete bpoints;
  delete apoints;
}

void tableXTfunHandler::create( Widget w, int *id, unsigned long *reason )
{
  if (debug) fprintf(stderr,"tableXTfunHandler::create %lx %d\n",(long)this,*id);
  switch (*id) {
  case 1: // image area
    {
    }
    break;
  case 2: // edit area
    {
      XtAddEventHandler(w, ButtonPressMask | ButtonReleaseMask, FALSE,
			(XtEventHandler)graph_window_input_cb, this);
    }
    break;
  case 3: // data volume selection menu
    {
      datavol_selection_widget= 
	update_datavol_menu(w,NULL,
			    lookup_dvol(current_dvol_index),
			    datavol_option_menu,
			    dvol_button_press_cb,
			    &datavol_menu_button_list);
    }
    break;
  case 4:
    {
      datavol_option_menu= w;
      if (datavol_selection_widget) {
	// Need to set the 'selected' menu item appropriately
	dList_iter<dvol_struct*> iter(datavol_list);
	sList_iter<Widget> widget_iter(datavol_menu_button_list);
	dvol_struct** this_struct;
	dvol_struct* current_datavol= lookup_dvol(current_dvol_index);
	Widget* button;
	while ((this_struct= iter.next()) 
	       && (button= widget_iter.next()) ) {
	  if (*this_struct == current_datavol) {
	    Arg args[3];
	    int n= 0;
	    XtSetArg( args[n], XmNmenuHistory, *button ); n++;
	    XtSetValues( w, args, n );
	  }
	}    
	
      }
    }
    break;
  default: 
    baseXTfunHandler::create( w, id, reason );
  }
  if (debug) fprintf(stderr,"tableXTfunHandler::create exit %lx %d\n",
		     (long)this,*id);
}

void tableXTfunHandler::dvol_button_press_cb( Widget w, XtPointer client_data, 
					      XtPointer call_data )
{
  ((tableXTfunHandler*)get_object(w))->
    dvol_button_press( w, client_data, call_data );
}

void tableXTfunHandler::dvol_button_press( Widget w, XtPointer client_data,
					   XtPointer call_data )
{
  // Nothing to do
}

gBColor* tableXTfunHandler::get_color_table()
{
    return ((TableTransferFunction*)(get_tfun()))->get_table();
}

void tableXTfunHandler::reset_color_table()
{
  gBColor *real_table= get_color_table();

  for (int i=0; i<256; i++) {
    rpoints[i].x= gpoints[i].x= bpoints[i].x= apoints[i].x= i;
    rpoints[i].y= (101*(255 - real_table[i].ir())) >> 8;
    gpoints[i].y= (101*(255 - real_table[i].ig())) >> 8;
    bpoints[i].y= (101*(255 - real_table[i].ib())) >> 8;
    apoints[i].y= (101*(255 - real_table[i].ia())) >> 8;
  }
}

void tableXTfunHandler::expose( Widget w, int *id, unsigned long *reason )
{
  switch (*id) {
  case 1: // image area
    {
      if (!ihandler) { // just created
	ihandler= new XImageHandler(XtDisplay(w), DefaultScreen(XtDisplay(w)),
				    XtWindow(w));
	image= new rgbImage( 256, ihandler->ysize() );
	update_image();
      }
      else ihandler->redraw();
    }
    break;
  case 2: // edit area
    {
      if (!graph_widget) graph_widget= w;
      update_graph();
    }
    break;
  default: 
    fprintf(stderr,"tableXTfunHandler::expose: out of sync with uid!\n");
  }
}

void tableXTfunHandler::update_graph()
{
  XClearWindow(dpy, XtWindow(graph_widget));
  gBColor *real_table= get_color_table();

  if (edit_which_color != 0)
    XDrawLines( dpy, XtWindow(graph_widget), r_gc, rpoints, 256, 
		CoordModeOrigin );

  if (edit_which_color != 1)
    XDrawLines( dpy, XtWindow(graph_widget), g_gc, gpoints, 256, 
		CoordModeOrigin );

  if (edit_which_color != 2)
    XDrawLines( dpy, XtWindow(graph_widget), b_gc, bpoints, 256, 
		CoordModeOrigin );

  if (edit_which_color != 3)
    XDrawLines( dpy, XtWindow(graph_widget), a_gc, apoints, 256, 
		CoordModeOrigin );

  XPoint points[256];
  int i;

  for (i=0; i<256; i++) points[i].x= i;

  if (edit_which_color == 0) {
    for (i=0; i<256; i++)
      points[i].y= (101*(255 - real_table[i].ir())) >> 8;
    XDrawLines( dpy, XtWindow(graph_widget), prev_gc, points, 256, 
		CoordModeOrigin );
    XDrawLines( dpy, XtWindow(graph_widget), r_gc, rpoints, 256, 
		CoordModeOrigin );
  }

  if (edit_which_color == 1) {
    for (i=0; i<256; i++)
      points[i].y= (101*(255 - real_table[i].ig())) >> 8;
    XDrawLines( dpy, XtWindow(graph_widget), prev_gc, points, 256, 
		CoordModeOrigin );
    XDrawLines( dpy, XtWindow(graph_widget), g_gc, gpoints, 256, 
		CoordModeOrigin );
  }

  if (edit_which_color == 2) {
    for (i=0; i<256; i++)
      points[i].y= (101*(255 - real_table[i].ib())) >> 8;
    XDrawLines( dpy, XtWindow(graph_widget), prev_gc, points, 256, 
		CoordModeOrigin );
    XDrawLines( dpy, XtWindow(graph_widget), b_gc, bpoints, 256, 
		CoordModeOrigin );
  }

  if (edit_which_color == 3) {
    for (i=0; i<256; i++)
      points[i].y= (101*(255 - real_table[i].ia())) >> 8;
    XDrawLines( dpy, XtWindow(graph_widget), prev_gc, points, 256, 
		CoordModeOrigin );
    XDrawLines( dpy, XtWindow(graph_widget), a_gc, apoints, 256, 
		CoordModeOrigin );
  }
}

void tableXTfunHandler::set_edit_color( Widget w, int *id, 
					unsigned long *reason )
{
  edit_which_color= *id;
  update_graph();
}

void tableXTfunHandler::update_image()
{
  gBColor *real_table= get_color_table();

  // Map the current color map into the image
  for (int i=0; i<256; i++)
    for (int j=0; j<image->ysize(); j++)
      image->setpix( i, j, real_table[i].alpha_weighted() );

  ihandler->display(image);
}

void tableXTfunHandler::reset( Widget w, int *id, unsigned long *reason )
{
  baseXTfunHandler::reset( w, id, reason );

  reset_color_table();
  update_graph();

  datavol_selection_widget= 
    update_datavol_menu( datavol_selection_widget, 
			XtParent(datavol_option_menu),
			lookup_dvol( current_dvol_index ),
			datavol_option_menu,
			dvol_button_press_cb, &datavol_menu_button_list );
}

void tableXTfunHandler::set_color_table()
{
  gBColor *real_table= ((TableTransferFunction*)(get_tfun()))->get_table();

  for (int i=0; i<256; i++)
      real_table[i]= gBColor( ((100 - rpoints[i].y) << 8) / 100,
			      ((100 - gpoints[i].y) << 8) / 100,
			      ((100 - bpoints[i].y) << 8) / 100,
			      ((100 - apoints[i].y) << 8) / 100);
}

void tableXTfunHandler::set( Widget w, int *id, unsigned long *reason )
{
  set_color_table();

  current_dvol_index= get_selected_dvol_index(datavol_selection_widget);

  update_image();
  update_graph();

  baseXTfunHandler::set( w, id, reason );
}

void tableXTfunHandler::graph_window_input_cb( Widget w,
					       baseXTfunHandler* handler,
					       XEvent *event )
{
  ((tableXTfunHandler*)handler)->graph_window_input( w, event );
}

void tableXTfunHandler::graph_window_input( Widget w, XEvent *event )
{
  XPoint drag_end;

  switch (event->type) {
  case ButtonPress: 
    {
      if (event->xbutton.button == Button1) {
	drag_start.x = event->xbutton.x;
	drag_start.y = event->xbutton.y;
	drag_start_valid= 1;
      }
    }
    break;
  case ButtonRelease:
    {
      if (event->xbutton.button == Button1) {
	drag_end.x = event->xbutton.x;
	drag_end.y = event->xbutton.y;
	drag_start_valid= 0;
	
	int begx, endx, begy, endy;
	if (drag_start.x <= drag_end.x) {
	  begx= drag_start.x;
	  begy= drag_start.y;
	  endx= drag_end.x;
	  endy= drag_end.y;
	}
	else {
	  begx= drag_end.x;
	  begy= drag_end.y;
	  endx= drag_start.x;
	  endy= drag_start.y;
	}
	
	// Trickery to avoid errors due to X window positioning
	if (begx > 255) begx= 255;
	if (endx > 255) endx= 255;
	if (begx < 0) begx= 0;
	if (endx < 0) endx= 0;
	if (begy > 100) begy= 100;
	if (begy < 0) begy= 0;
	if (endy > 100) endy= 100;
	if (endy < 0) endy= 0;

	if (endx > begx) {
	  float incr= (float)(endy-begy)/(float)(endx-begx);
	  
	  switch (edit_which_color) {
	  case 0: // red
	    {
	      for (int i=0; i<=endx-begx; i++) 
		rpoints[i+begx].y= begy + (int)(incr*i);
	    }
	    break;
	  case 1: // green
	    {
	      for (int i=0; i<=endx-begx; i++) 
		gpoints[i+begx].y= begy + (int)(incr*i);
	    }
	    break;
	  case 2: // blue
	    {
	      for (int i=0; i<=endx-begx; i++) 
		bpoints[i+begx].y= begy + (int)(incr*i);
	    }
	    break;
	  case 3: // alpha
	    {
	      for (int i=0; i<=endx-begx; i++) 
		apoints[i+begx].y= begy + (int)(incr*i);
	    }
	    break;
	  }
	}
	else {
	  // just set appropriate pixel to final value
	  switch (edit_which_color) {
	  case 0: rpoints[endx].y= endy; break;
	  case 1: gpoints[endx].y= endy; break;
	  case 2: bpoints[endx].y= endy; break;
	  case 3: apoints[endx].y= endy; break;
	  }
	}
	update_graph();
      }
      break;
    }
  }
}

int tableXTfunHandler::datavol_update_inform( int datavol_removed )
{
  dList_iter<dvol_struct*> iter(datavol_list);
  dvol_struct** this_struct;
  int index_still_valid= 1;
  for (int i=0; i<= current_dvol_index; i++) {
    if (!iter.next()) {
      index_still_valid= 0;
      break;
    }
  }

  if (!index_still_valid) {
    current_dvol_index= 0;
  }

  datavol_selection_widget= 
    update_datavol_menu( datavol_selection_widget, 
			XtParent(datavol_option_menu),
			lookup_dvol(current_dvol_index),
			datavol_option_menu,
			dvol_button_press_cb, &datavol_menu_button_list );
    
  return(1); // always request regen
}

gradtableXTfunHandler::gradtableXTfunHandler( baseXTfunHandler* parent_in, 
					      MrmHierarchy mrm_id_in, 
					      FILE* infile, 
					      const GridInfo* grid_in,
					      Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    tableXTfunHandler(parent_in, mrm_id_in, infile, grid_in,
		      "grad_tfun_dlog", "grad_tfun_topform",
		      neighbor), 
    gradtableTfunHandler( infile, grid_in ),
    tableTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
      "gradtableXTfunHandler::gradtableXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (debug) 
    fprintf(stderr,
  "gradtableXTfunHandler::gradtableXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

gradtableXTfunHandler::gradtableXTfunHandler( void (*update_cb_in)
					         (baseXTfunHandler*),
					      void (*deletion_cb_in)
				                 (baseXTfunHandler*), 
					      MrmHierarchy mrm_id_in, 
					      FILE* infile,
					      const GridInfo* grid_in,
					      Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    tableXTfunHandler(update_cb_in, deletion_cb_in, mrm_id_in, infile, grid_in,
		      "grad_tfun_dlog", "grad_tfun_topform",
		      neighbor), 
    gradtableTfunHandler( infile, grid_in ),
    tableTfunHandler(infile, grid_in)
{
  if (debug) 
    fprintf(stderr,
      "gradtableXTfunHandler::gradtableXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  if (debug) 
    fprintf(stderr,
  "gradtableXTfunHandler::gradtableXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

gradtableXTfunHandler::~gradtableXTfunHandler()
{
}

gBColor* gradtableXTfunHandler::get_color_table()
{
    return ((GradTableTransferFunction*)(get_tfun()))->get_table();
}

void gradtableXTfunHandler::set_color_table()
{
  gBColor *real_table= ((GradTableTransferFunction*)(get_tfun()))->get_table();

  for (int i=0; i<256; i++)
    real_table[i]= gBColor( ((100 - rpoints[i].y) << 8) / 100,
			    ((100 - gpoints[i].y) << 8) / 100,
			    ((100 - bpoints[i].y) << 8) / 100,
			    ((100 - apoints[i].y) << 8) / 100);

}

void sumXTfunHandler::construct( FILE *infile, Widget neighbor,
				 char* type_name, char* dlog_name, 
				 char* widget_name )
{
  if (!valid) return;

  if (!initialized) {
    initialized= 1;
  }

  // For use by create routine
  loadfile= infile;
  kid_holder_widget= NULL;
  kid_scroll_widget= NULL;
  being_deleted= 0;
  child_build_in_progress= 0;
  weight_of_new_child= 0.0;
  
  // The instantiation of the widget causes create_cb to be called,
  // which in turn calls the create method, which reads in the kids
  // and sets up the kids table.

  // Build the enclosing widget, with a pointer back here
  build( neighbor, type_name, dlog_name, widget_name );

  if (valid) {
    XtManageChild( widget );
    if (!parent) resize();
  }

  loadfile= NULL; // To avoid accessing it later, when it might be closed
}

sumXTfunHandler::sumXTfunHandler( baseXTfunHandler* parent_in, 
				  MrmHierarchy mrm_id_in, 
				  FILE* infile, const GridInfo* grid_in,
				  Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    baseXTfunHandler(parent_in, mrm_id_in,infile, grid_in), 
    sumTfunHandler( infile, grid_in, 0 )
{
  if (debug) 
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct( infile, neighbor, "sum", "sum_tfun_dlog", "sum_tfun_topform" );
  update_tfun();
  if (debug) 
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

sumXTfunHandler::sumXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				  void (*deletion_cb_in)
				      (baseXTfunHandler*),
				  MrmHierarchy mrm_id_in, 
				  FILE* infile, const GridInfo* grid_in,
				  Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    baseXTfunHandler(update_cb_in,deletion_cb_in,mrm_id_in,infile, grid_in), 
    sumTfunHandler( infile, grid_in, 0 )
{
  if (debug) 
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct( infile, neighbor, "sum", "sum_tfun_dlog", "sum_tfun_topform" );
  update_tfun();
  if (debug)
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

sumXTfunHandler::sumXTfunHandler( baseXTfunHandler* parent_in, 
				  MrmHierarchy mrm_id_in, 
				  FILE* infile, const GridInfo* grid_in,
				  Widget neighbor,
				  char* type_name, char* dlog_name,
				  char* widget_name )
  : baseTfunHandler(infile, grid_in ),
    baseXTfunHandler(parent_in,mrm_id_in,infile,grid_in), 
    sumTfunHandler( infile, grid_in, 0 )
{
  if (debug)
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct( infile, neighbor, type_name, dlog_name, widget_name );
  if (debug)
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

sumXTfunHandler::sumXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				  void (*deletion_cb_in)
				      (baseXTfunHandler*),
				  MrmHierarchy mrm_id_in, 
				  FILE* infile, const GridInfo* grid_in,
				  Widget neighbor,
				  char* type_name, char* dlog_name,
				  char* widget_name )
  : baseTfunHandler(infile, grid_in),
    baseXTfunHandler(update_cb_in,deletion_cb_in,mrm_id_in,infile,grid_in), 
    sumTfunHandler( infile, grid_in, 0 )
{
  if (debug) 
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct( infile, neighbor, type_name, dlog_name, widget_name );
  if (debug) 
    fprintf(stderr,
	    "sumXTfunHandler::sumXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

sumXTfunHandler::~sumXTfunHandler()
{
  being_deleted= 1;

  while (xkids.head()) {
    delete xkids.pop();  // Deletes XTfun and Tfun parts
    kids.pop();
    factors.pop();
  }

}

void sumXTfunHandler::update_tfun()
{
  if (debug) 
    fprintf(stderr,
	    "sumXTfunHandler::update_tfun %lx: valid %d, tfun %lx\n",
	    (long)this, valid, (long)tfun);
  baseTransferFunction **tfuns= new baseTransferFunction*[ntfuns];
  float *tmpfac= new float[ntfuns];
  dList_iter<baseXTfunHandler*> xiter(xkids);
  dList_iter<baseTfunHandler*> kiter(kids);
  dList_iter<float> fiter(factors);
  
  // We actually count the number of data volumes needed by children,
  // to guard against errors in the loaded file.

  baseXTfunHandler **xkidptr;
  baseTfunHandler **kidptr;
  int i= 0;
  int ndata= 0;

  valid= 1;

  while (xkidptr= xiter.next()) {
    kidptr= kiter.next();
    if ((*xkidptr) && (*kidptr)) {
      tfuns[i]= ((*kidptr)->get_tfun());
      ndata += tfuns[i]->ndata();
      tmpfac[i]= *(fiter.next());
    }
    else valid= 0;
    i++;
  }

  if (valid) {
    if (tfun)
      ((SumTransferFunction*)tfun)->edit( ndata, ntfuns, tmpfac, tfuns );
    else
      tfun= new SumTransferFunction( ndata, ntfuns, tmpfac, tfuns );
  }

  delete [] tmpfac;
  delete [] tfuns;
}

void sumXTfunHandler::create( Widget w, int *id, unsigned long *reason )
{
  if (debug) fprintf(stderr,"sumXTfunHandler::create %lx %d\n",(long)this,*id);
  switch (*id) {
  case 1:
    {
      kid_holder_widget= w;
      
      // This is called when the widget into which sub-handlers are to
      // be inserted becomes available.
      
      Widget last= NULL;
      dList_iter<float> weight_iter(factors);
      for (int i=0; i<ntfuns; i++) {
	baseXTfunHandler *thiskid;
	child_build_in_progress= 1;
	weight_of_new_child= *(weight_iter.next());
	register_weight( weight_of_new_child );
	xkids.append(thiskid= XTfunHandler_load(this, mrm_id, loadfile, 
						&grid, last));
	child_build_in_progress= 0;
	kids.append((baseTfunHandler*)thiskid);
	last= thiskid->get_widget();
      }
      
    };
    break;
  case 2:
    {
      kid_scroll_widget= w;
    };
    break;
  default: 
    baseXTfunHandler::create(w, id, reason);
  }
  if (debug) fprintf(stderr,"sumXTfunHandler::create exit %lx %d\n",
		     (long)this,*id);
}

void sumXTfunHandler::set( Widget w, int* id, unsigned long* reason )
{
  // Set the kids
  dList_iter<baseXTfunHandler*> xkid_iter(xkids);

  baseXTfunHandler** this_kid;
  while (this_kid= xkid_iter.next()) {
    (*this_kid)->set( w, id, reason );
  }

  baseXTfunHandler::set(w, id, reason);
}

void sumXTfunHandler::reset( Widget w, int* id, unsigned long* reason )
{
  baseXTfunHandler::reset(w, id, reason);

  // reset the kids
  dList_iter<baseXTfunHandler*> xkid_iter(xkids);

  baseXTfunHandler** this_kid;
  while (this_kid= xkid_iter.next()) {
    (*this_kid)->reset( w, id, reason );
  }
}

Widget sumXTfunHandler::get_enclosing_widget( baseXTfunHandler* asked_by )
     const
{
  return kid_holder_widget;
}

void sumXTfunHandler::connect_child_widgets()
{
  // Walk down through the child list, attaching all the widgets
  // to appropriate neighbors
  dList_iter<baseXTfunHandler*> xiter(xkids);

//  XtUnmanageChild( kid_holder_widget );

  baseXTfunHandler** kidptr;
  baseXTfunHandler** prev= NULL;
  Arg args[MAX_ARGS];
  while (kidptr= xiter.next()) {
    int n= 0;
    if (prev) { // connect to preceeding widget
      XtSetArg( args[n], XmNtopAttachment, XmATTACH_WIDGET ); n++;
      XtSetArg( args[n], XmNtopWidget, (*prev)->get_widget() ); n++; 
    }
    else { // connect to form
      XtSetArg( args[n], XmNtopAttachment, XmATTACH_FORM ); n++;
    }
    XtSetValues( (*kidptr)->get_widget(), args, n );
    prev= kidptr;
  }

//  XtManageChild( kid_holder_widget );
}

void sumXTfunHandler::goodbye( baseXTfunHandler* asked_by )
{
  if (!being_deleted) {
    dList_iter<baseXTfunHandler*> xiter(xkids);
    dList_iter<baseTfunHandler*> kiter(kids);
    dList_iter<float> fiter(factors);
    baseXTfunHandler** kidptr;
    
    while (kidptr= xiter.next()) {
      (void)kiter.next();  // walk other iterators along too
      (void)fiter.next();
      if ( *kidptr == asked_by ) break;
    }
    
    xiter.remove();
    kiter.remove();
    fiter.remove();
    ntfuns -= 1;
    
    connect_child_widgets();
    recalc_preferred_size();
    update_tfun();
    child_changed();
    resize();
  }
}

void sumXTfunHandler::add( Widget w, int *id, unsigned long *reason )
{
  child_build_in_progress= 1;
  weight_of_new_child= 1.0;
  register_weight(weight_of_new_child);
  baseXTfunHandler *thiskid= load_tfun_by_number( *id, &grid, this,
						  xkids.tail() ? 
						  xkids.tail()->get_widget() 
						  : NULL);
  child_build_in_progress= 0;

  if (!thiskid) return; // error, or file selection box popped up

  factors.append(1.0); 
  xkids.append(thiskid);
  kids.append((baseTfunHandler*)thiskid);

  ntfuns += 1;
  update_tfun();

  resize();
  child_changed();
}

void sumXTfunHandler::add_file( Widget w, int *id,
				XmFileSelectionBoxCallbackStruct *call_data)
{
  if (call_data->reason != XmCR_CANCEL) {
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
    
    FILE *tfun_file= fopen(the_name,"r");
    if (!tfun_file) {
      pop_error_dialog("invalid_tfun_file_msg");
      return;
    }
    
    baseXTfunHandler *thiskid;
    child_build_in_progress= 1;
    weight_of_new_child= 1.0;
    register_weight(weight_of_new_child);
    Widget tail_widget= xkids.tail() ? xkids.tail()->get_widget() : NULL;
    xkids.append(thiskid= XTfunHandler_load( this, mrm_id, tfun_file, &grid,
					     tail_widget ) );
    child_build_in_progress= 0;
    factors.append(1.0); 
    kids.append((baseTfunHandler*)thiskid);
    
    fclose(tfun_file);
    ntfuns += 1;
    update_tfun();
    
    resize();
    child_changed();
  }
}

void sumXTfunHandler::set_weight( baseXTfunHandler *asked_by, 
				  const float weight )
{
  dList_iter<baseXTfunHandler*> xkid_iter(xkids);
  dList_iter<float> factor_iter(factors);

  float *this_weight;
  baseXTfunHandler** this_kid;
  while (this_kid= xkid_iter.next()) {
    this_weight= factor_iter.next();
    if (*this_kid == asked_by) {
      *this_weight= weight;
      update_tfun();
      child_changed();
      return;
    }
  }

  fprintf(stderr,"sumXTfunHandler::set_weight: I don't know that kid!\n");
}

float sumXTfunHandler::get_weight( const baseXTfunHandler* asked_by )
{  
  dList_iter<baseXTfunHandler*> xkid_iter(xkids);
  dList_iter<float> factor_iter(factors);

  float *this_weight;
  baseXTfunHandler** this_kid;
  while (this_kid= xkid_iter.next()) {
    this_weight= factor_iter.next();
    if (*this_kid == asked_by) return *this_weight;
  }

  if (child_build_in_progress) return weight_of_new_child;

  fprintf(stderr,"sumXTfunHandler::get_weight: I don't know that kid!\n");
  return 0.0;
}

int sumXTfunHandler::datavol_update_inform( int datavol_removed )
{
  if (being_deleted) return 0;

  else {
    dList_iter<baseXTfunHandler*> iter(xkids);
    int kid_wants_update= 0;
    baseXTfunHandler** this_kid;
    
    while (this_kid= iter.next()) {
      if ((*this_kid)->datavol_update_inform(datavol_removed))
	kid_wants_update= 1;
    }
    
    return kid_wants_update;
  }
}

void sumXTfunHandler::child_changed()
{
  update_tfun();
  baseXTfunHandler::child_changed();
}

baseXTfunHandler::WinSize sumXTfunHandler::recalc_preferred_size()
{
  WinSize current_total_size= get_widget_size( widget );
  WinSize current_scroll_size= get_widget_size( kid_scroll_widget );

  kid_area_size.x= 0;
  kid_area_size.y= 0;
  dList_iter<baseXTfunHandler*> iter(xkids);
  baseXTfunHandler** this_kid;
  while (this_kid= iter.next()) {
    WinSize kid_size= (*this_kid)->recalc_preferred_size();
    if (kid_size.x > kid_area_size.x) kid_area_size.x= kid_size.x;
    kid_area_size.y += kid_size.y;
  }

  // Calculate size of new scrolling area
  scroll_area_size= kid_area_size;

  // Calculate size of new overall widget, retaining margins
  if (scroll_area_size.x>150) {
    preferred_size.x= scroll_area_size.x +4
      + (current_total_size.x - current_scroll_size.x);
  }
  else preferred_size.x= current_total_size.x;
  preferred_size.y= preferred_size.y= scroll_area_size.y + 4
      + (current_total_size.y - current_scroll_size.y);
    
  if (debug) 
    fprintf(stderr,"sumXTfunHandler::recalc_preferred_size: %d %d\n",
	    preferred_size.x,preferred_size.y);
  return preferred_size;
}

void sumXTfunHandler::set_preferred_size()
{
  set_widget_size( widget, preferred_size );
  set_widget_size( kid_holder_widget, kid_area_size );

  dList_iter<baseXTfunHandler*> iter(xkids);
  baseXTfunHandler** this_kid;
  while (this_kid= iter.next()) {
    (*this_kid)->set_preferred_size();
  }  
}

ssumXTfunHandler::ssumXTfunHandler( baseXTfunHandler* parent_in, 
				    MrmHierarchy mrm_id_in, 
				    FILE* infile, const GridInfo* grid_in,
				    Widget neighbor )
  : baseTfunHandler(infile, grid_in), 
    sumTfunHandler( infile, grid_in, 0 ),
    ssumTfunHandler( infile, grid_in, 0 ),
    sumXTfunHandler(parent_in, mrm_id_in, infile, grid_in, neighbor,
		    "ssum", "ssum_tfun_dlog", "ssum_tfun_topform")
{
  if (debug) 
    fprintf(stderr,
	    "ssumXTfunHandler::ssumXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  update_tfun();
  if (debug)
    fprintf(stderr,
	    "ssumXTfunHandler::ssumXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

ssumXTfunHandler::ssumXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				    void (*deletion_cb_in) (baseXTfunHandler*),
				    MrmHierarchy mrm_id_in, 
				    FILE* infile, const GridInfo* grid_in,
				    Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    sumTfunHandler( infile, grid_in, 0 ),
    ssumTfunHandler( infile, grid_in, 0 ),
    sumXTfunHandler(update_cb_in, deletion_cb_in, mrm_id_in, infile, grid_in,
		    neighbor, "ssum", "ssum_tfun_dlog", "ssum_tfun_topform" )
{
  if (debug) 
    fprintf(stderr,
	    "ssumXTfunHandler::ssumXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  update_tfun();
  if (debug) 
    fprintf(stderr,
	    "ssumXTfunHandler::ssumXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

ssumXTfunHandler::~ssumXTfunHandler()
{
}

void ssumXTfunHandler::update_tfun()
{
  if (debug) 
    fprintf(stderr,"ssumXTfunHandler::update_tfun %lx\n",(long)this);
  baseTransferFunction **tfuns= 
    new baseTransferFunction*[ssumTfunHandler::ntfuns];
  float *tmpfac= new float[ssumTfunHandler::ntfuns];
  dList_iter<baseXTfunHandler*> xiter(xkids);
  dList_iter<baseTfunHandler*> kiter(ssumTfunHandler::kids);
  dList_iter<float> fiter(ssumTfunHandler::factors);
  
  // We actually count the number of data volumes needed by children,
  // to guard against errors in the loaded file.

  baseXTfunHandler **xkidptr;
  baseTfunHandler **kidptr;
  int i= 0;
  int ndata= 0;

  valid= 1;

  while (xkidptr= xiter.next()) {
    kidptr= kiter.next();
    if ((*xkidptr) && (*kidptr)) {
      tfuns[i]= ((*kidptr)->get_tfun());
      ndata += tfuns[i]->ndata();
      tmpfac[i]= *(fiter.next());
    }
    else valid= 0;
    i++;
  }

  if (valid) {
    if (tfun)
      ((SSumTransferFunction*)tfun)->edit( ndata, ssumTfunHandler::ntfuns, 
					   tmpfac, tfuns );
    else
      tfun= new SSumTransferFunction( ndata, ssumTfunHandler::ntfuns, 
				      tmpfac, tfuns );
  }

  delete [] tmpfac;
  delete [] tfuns;
}

void blockXTfunHandler::construct()
{
  if (!initialized) {
    MRMRegisterArg mrm_names[1];
    mrm_names[0].name= "blockxtfunhandler_indx_chng_cb";
    mrm_names[0].value= (caddr_t)blockXTfunHandler::index_change_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    initialized= 1;
  }

  llf_x_widget= llf_y_widget= llf_z_widget= NULL;
  trb_x_widget= trb_y_widget= trb_z_widget= NULL;
  rgba_r_widget= rgba_g_widget= rgba_b_widget= rgba_a_widget= NULL;
  inside_tb_widget= index_tb_widget= NULL;
}

blockXTfunHandler::blockXTfunHandler( baseXTfunHandler* parent_in, 
				      MrmHierarchy mrm_id_in,
				      FILE *infile, const GridInfo* grid_in,
				      Widget neighbor )
  : baseTfunHandler(infile, grid_in), 
    baseXTfunHandler(parent_in, mrm_id_in, infile, grid_in), 
    blockTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	"blockXTfunHandler::blockXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct();
  if (valid) build(neighbor, "block", "block_tfun_dlog", "block_tfun_topform");
  if (valid) XtManageChild(widget);
  if (debug) 
    fprintf(stderr,
	"blockXTfunHandler::blockXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

blockXTfunHandler::blockXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				    void (*deletion_cb_in)(baseXTfunHandler*),
				      MrmHierarchy mrm_id_in,
				      FILE *infile, const GridInfo* grid_in,
				      Widget neighbor )
: baseTfunHandler(infile, grid_in), 
  baseXTfunHandler(update_cb_in, deletion_cb_in,mrm_id_in, infile, grid_in), 
  blockTfunHandler( infile, grid_in )
{
  if (debug) 
    fprintf(stderr,
	"blockXTfunHandler::blockXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct();
  if (valid) build(neighbor, "block", "block_tfun_dlog", "block_tfun_topform");
  if (valid) XtManageChild(widget);
  if (debug) 
    fprintf(stderr,
	"blockXTfunHandler::blockXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

blockXTfunHandler::~blockXTfunHandler()
{
  // nothing to do
}

void blockXTfunHandler::index_change_cb( Widget w, int *id, 
					 unsigned long *reason )
{
  blockXTfunHandler* handler= (blockXTfunHandler*)get_object(w);
  if (handler) handler->index_change( w, id, reason );
}

void blockXTfunHandler::refit_to_volume( const GridInfo* grid_in )
{
  if (debug) fprintf(stderr,"blockXTfunHandler::refit_to_volume %lx\n", 
		     (long)this);

  if (*grid_in != grid) reset(NULL, NULL, NULL);

  blockTfunHandler::refit_to_volume( grid_in );

  if (debug) fprintf(stderr,"blockXTfunHandler::refit_to_volume exit %lx\n", 
		     (long)this);
}

void blockXTfunHandler::create( Widget w, int *id, unsigned long *reason )
{
  if (debug) fprintf(stderr,"blockXTfunHandler::create %lx %d\n",(long)this,*id);
  switch (*id) {
  case 1:
    {
      llf_x_widget= w;
    }
    break;
  case 2:
    {
      llf_y_widget= w;
    }
    break;
  case 3:
    {
      llf_z_widget= w;
    }
    break;
  case 4:
    {
      trb_x_widget= w;
    }
    break;
  case 5:
    {
      trb_y_widget= w;
    }
    break;
  case 6:
    {
      trb_z_widget= w;
    }
    break;
  case 7:
    {
      rgba_r_widget= w;
    }
    break;
  case 8:
    {
      rgba_g_widget= w;
    }
    break;
  case 9:
    {
      rgba_b_widget= w;
    }
    break;
  case 10:
    {
      rgba_a_widget= w;
    }
    break;
  case 11:
    {
      index_tb_widget= w;
    }
    break;
  case 12:
    {
      inside_tb_widget= w;
    }
    break;
  default: 
    baseXTfunHandler::create( w, id, reason );
  }

  if ( llf_x_widget && llf_y_widget && llf_z_widget
       && llf_x_widget && llf_y_widget && llf_z_widget
       && rgba_r_widget && rgba_g_widget && rgba_b_widget && rgba_a_widget
       && index_tb_widget && inside_tb_widget ) {
    reset( NULL, NULL, NULL );
  }

  if (debug) fprintf(stderr,"blockXTfunHandler::create exit %lx %d\n",
		     (long)this,*id);
}

void blockXTfunHandler::set( Widget w, int* id, unsigned long* reason )
{
  if (debug) fprintf(stderr,"blockXTfunHandler::set\n");

  float fxmin;
  float fymin;
  float fzmin;
  float fxmax;
  float fymax;
  float fzmax;
  gBColor clr;
  int inside;

  inside= XmToggleButtonGadgetGetState( inside_tb_widget );

  char* txtstring;
  const gBoundBox& bbox= grid.bbox();
  if (XmToggleButtonGadgetGetState( index_tb_widget )) {
    // read indices
    int low;
    int high;
    ordered_pair_from_text_widgets( &low, &high, llf_x_widget, trb_x_widget,
				    0, grid.xsize()-1 );
    fxmin= (float)low/((float)grid.xsize()-1);
    fxmax= (float)high/((float)grid.xsize()-1);
    ordered_pair_from_text_widgets( &low, &high, llf_y_widget, trb_y_widget,
				    0, grid.ysize()-1 );
    fymin= (float)low/((float)grid.ysize()-1);
    fymax= (float)high/((float)grid.ysize()-1);
    ordered_pair_from_text_widgets( &low, &high, llf_z_widget, trb_z_widget,
				    0, grid.zsize()-1 );
    fzmin= (float)low/((float)grid.zsize()-1);
    fzmax= (float)high/((float)grid.zsize()-1);
    
    clr= gBColor( int_from_text_widget( rgba_r_widget, 0, 255 ),
		  int_from_text_widget( rgba_g_widget, 0, 255 ),
		  int_from_text_widget( rgba_b_widget, 0, 255 ),
		  int_from_text_widget( rgba_a_widget, 0, 255 ) );
  }
  else {
    // read coordinates
    float low;
    float high;
    ordered_pair_from_text_widgets( &low, &high, llf_x_widget, trb_x_widget,
				    bbox.xmin(), bbox.xmax() );
    fxmin= (low-bbox.xmin())/(bbox.xmax()-bbox.xmin());
    fxmax= (high-bbox.xmin())/(bbox.xmax()-bbox.xmin());
    ordered_pair_from_text_widgets( &low, &high, llf_y_widget, trb_y_widget,
				    bbox.ymin(), bbox.ymax() );
    fymin= (low-bbox.ymin())/(bbox.ymax()-bbox.ymin());
    fymax= (high-bbox.ymin())/(bbox.ymax()-bbox.ymin());
    ordered_pair_from_text_widgets( &low, &high, llf_z_widget, trb_z_widget,
				    bbox.zmin(), bbox.zmax() );
    fzmin= (low-bbox.zmin())/(bbox.zmax()-bbox.zmin());
    fzmax= (high-bbox.zmin())/(bbox.zmax()-bbox.zmin());

    clr= gBColor( float_from_text_widget( rgba_r_widget, 0.0, 1.0 ),
		  float_from_text_widget( rgba_g_widget, 0.0, 1.0 ),
		  float_from_text_widget( rgba_b_widget, 0.0, 1.0 ),
		  float_from_text_widget( rgba_a_widget, 0.0, 1.0 ) );
  }

  ((BlockTransferFunction*)get_tfun())->edit( fxmin, fymin, fzmin,
					      fxmax, fymax, fzmax,
					      clr, inside );
  baseXTfunHandler::set( w, id, reason );

  if (debug) fprintf(stderr,"blockXTfunHandler::set exit\n");
}

void blockXTfunHandler::reset( Widget w, int* id, unsigned long* reason )
{
  if (debug) fprintf(stderr,"blockXTfunHandler::reset\n");

  baseXTfunHandler::reset( w, id, reason );

  float fxmin;
  float fymin;
  float fzmin;
  float fxmax;
  float fymax;
  float fzmax;
  gBColor clr;
  int inside;
  ((BlockTransferFunction*)get_tfun())->get_info( &fxmin, &fymin, &fzmin,
						  &fxmax, &fymax, &fzmax,
						  &clr, &inside );

  XmToggleButtonGadgetSetState( inside_tb_widget, inside, False );

  char txtstring[16];
  const gBoundBox& bbox= grid.bbox();
  if (XmToggleButtonGadgetGetState( index_tb_widget )) {
    // write indices
    sprintf(txtstring, "%-5d", (int)(fxmin*(grid.xsize()-1) + 0.5));
    XmTextFieldSetString( llf_x_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fymin*(grid.ysize()-1) + 0.5));
    XmTextFieldSetString( llf_y_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fzmin*(grid.zsize()-1) + 0.5));
    XmTextFieldSetString( llf_z_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fxmax*(grid.xsize()-1) + 0.5));
    XmTextFieldSetString( trb_x_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fymax*(grid.ysize()-1) + 0.5));
    XmTextFieldSetString( trb_y_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fzmax*(grid.zsize()-1) + 0.5));
    XmTextFieldSetString( trb_z_widget, txtstring );

    sprintf(txtstring, "%-5d", clr.ir());
    XmTextFieldSetString( rgba_r_widget, txtstring );
    sprintf(txtstring, "%-5d", clr.ig());
    XmTextFieldSetString( rgba_g_widget, txtstring );
    sprintf(txtstring, "%-5d", clr.ib());
    XmTextFieldSetString( rgba_b_widget, txtstring );
    sprintf(txtstring, "%-5d", clr.ia());
    XmTextFieldSetString( rgba_a_widget, txtstring );
  }
  else {
    // write coordinates
    sprintf(txtstring, "%-7.3f", (fxmin*(bbox.xmax()-bbox.xmin()) 
				  + bbox.xmin()));
    XmTextFieldSetString( llf_x_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fymin*(bbox.ymax()-bbox.ymin()) 
				  + bbox.ymin()));
    XmTextFieldSetString( llf_y_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fzmin*(bbox.zmax()-bbox.zmin()) 
				  + bbox.zmin()));
    XmTextFieldSetString( llf_z_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fxmax*(bbox.xmax()-bbox.xmin()) 
				  + bbox.xmin()));
    XmTextFieldSetString( trb_x_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fymax*(bbox.ymax()-bbox.ymin()) 
				  + bbox.ymin()));
    XmTextFieldSetString( trb_y_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fzmax*(bbox.zmax()-bbox.zmin()) 
				  + bbox.zmin()));
    XmTextFieldSetString( trb_z_widget, txtstring );

    sprintf(txtstring, "%-7.3f", clr.r());
    XmTextFieldSetString( rgba_r_widget, txtstring );
    sprintf(txtstring, "%-7.3f", clr.g());
    XmTextFieldSetString( rgba_g_widget, txtstring );
    sprintf(txtstring, "%-7.3f", clr.b());
    XmTextFieldSetString( rgba_b_widget, txtstring );
    sprintf(txtstring, "%-7.3f", clr.a());
    XmTextFieldSetString( rgba_a_widget, txtstring );
  }

  if (debug) fprintf(stderr,"blockXTfunHandler::reset exit\n");
}

void blockXTfunHandler::index_change( Widget w, int *id, 
				      unsigned long *reason )
{
  if (debug) fprintf(stderr,"blockXTfunHandler::index_change %lx\n", 
		     (long)this);

  char txtstring[16];
  const gBoundBox& bbox= grid.bbox();
  float fxmin;
  float fymin;
  float fzmin;
  float fxmax;
  float fymax;
  float fzmax;
  gBColor clr;
  if (XmToggleButtonGadgetGetState( index_tb_widget )) {
    // change coords to indices
    float low;
    float high;

    ordered_pair_from_text_widgets( &low, &high, llf_x_widget, trb_x_widget,
				    bbox.xmin(), bbox.xmax() );
    fxmin= (low-bbox.xmin())/(bbox.xmax()-bbox.xmin());
    fxmax= (high-bbox.xmin())/(bbox.xmax()-bbox.xmin());
    sprintf(txtstring, "%-5d", (int)(fxmin*(grid.xsize()-1) + 0.5));
    XmTextFieldSetString( llf_x_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fxmax*(grid.xsize()-1) + 0.5));
    XmTextFieldSetString( trb_x_widget, txtstring );

    ordered_pair_from_text_widgets( &low, &high, llf_y_widget, trb_y_widget,
				    bbox.ymin(), bbox.ymax() );
    fymin= (low-bbox.ymin())/(bbox.ymax()-bbox.ymin());
    fymax= (high-bbox.ymin())/(bbox.ymax()-bbox.ymin());
    sprintf(txtstring, "%-5d", (int)(fymin*(grid.ysize()-1) + 0.5));
    XmTextFieldSetString( llf_y_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fymax*(grid.ysize()-1) + 0.5));
    XmTextFieldSetString( trb_y_widget, txtstring );

    ordered_pair_from_text_widgets( &low, &high, llf_z_widget, trb_z_widget,
				    bbox.zmin(), bbox.zmax() );
    fzmin= (low-bbox.zmin())/(bbox.zmax()-bbox.zmin());
    fzmax= (high-bbox.zmin())/(bbox.zmax()-bbox.zmin());
    sprintf(txtstring, "%-5d", (int)(fzmin*(grid.zsize()-1) + 0.5));
    XmTextFieldSetString( llf_z_widget, txtstring );
    sprintf(txtstring, "%-5d", (int)(fzmax*(grid.zsize()-1) + 0.5));
    XmTextFieldSetString( trb_z_widget, txtstring );

    clr= gBColor( float_from_text_widget( rgba_r_widget, 0.0, 1.0 ),
		  float_from_text_widget( rgba_g_widget, 0.0, 1.0 ),
		  float_from_text_widget( rgba_b_widget, 0.0, 1.0 ),
		  float_from_text_widget( rgba_a_widget, 0.0, 1.0 ) );
    sprintf(txtstring, "%-5d", clr.ir());
    XmTextFieldSetString( rgba_r_widget, txtstring );
    sprintf(txtstring, "%-5d", clr.ig());
    XmTextFieldSetString( rgba_g_widget, txtstring );
    sprintf(txtstring, "%-5d", clr.ib());
    XmTextFieldSetString( rgba_b_widget, txtstring );
    sprintf(txtstring, "%-5d", clr.ia());
    XmTextFieldSetString( rgba_a_widget, txtstring );
  }
  else {
    // change indices to coords
    int low;
    int high;
    ordered_pair_from_text_widgets( &low, &high, llf_x_widget, trb_x_widget,
				    0, grid.xsize()-1 );
    fxmin= (float)low/((float)grid.xsize()-1);
    fxmax= (float)high/((float)grid.xsize()-1);
    sprintf(txtstring, "%-7.3f", (fxmin*(bbox.xmax()-bbox.xmin()) 
				  + bbox.xmin()));
    XmTextFieldSetString( llf_x_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fxmax*(bbox.xmax()-bbox.xmin()) 
				  + bbox.xmin()));
    XmTextFieldSetString( trb_x_widget, txtstring );

    ordered_pair_from_text_widgets( &low, &high, llf_y_widget, trb_y_widget,
				    0, grid.ysize()-1 );
    fymin= (float)low/((float)grid.ysize()-1);
    fymax= (float)high/((float)grid.ysize()-1);
    sprintf(txtstring, "%-7.3f", (fymin*(bbox.ymax()-bbox.ymin()) 
				  + bbox.ymin()));
    XmTextFieldSetString( llf_y_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fymax*(bbox.ymax()-bbox.ymin()) 
				  + bbox.ymin()));
    XmTextFieldSetString( trb_y_widget, txtstring );

    ordered_pair_from_text_widgets( &low, &high, llf_z_widget, trb_z_widget,
				    0, grid.zsize()-1 );
    fzmin= (float)low/((float)grid.zsize()-1);
    fzmax= (float)high/((float)grid.zsize()-1);
    sprintf(txtstring, "%-7.3f", (fzmin*(bbox.zmax()-bbox.zmin()) 
				  + bbox.zmin()));
    XmTextFieldSetString( llf_z_widget, txtstring );
    sprintf(txtstring, "%-7.3f", (fzmax*(bbox.zmax()-bbox.zmin()) 
				  + bbox.zmin()));
    XmTextFieldSetString( trb_z_widget, txtstring );

    clr= gBColor( int_from_text_widget( rgba_r_widget, 0, 255 ),
		  int_from_text_widget( rgba_g_widget, 0, 255 ),
		  int_from_text_widget( rgba_b_widget, 0, 255 ),
		  int_from_text_widget( rgba_a_widget, 0, 255 ) );
    sprintf(txtstring, "%-7.3f", clr.r());
    XmTextFieldSetString( rgba_r_widget, txtstring );
    sprintf(txtstring, "%-7.3f", clr.g());
    XmTextFieldSetString( rgba_g_widget, txtstring );
    sprintf(txtstring, "%-7.3f", clr.b());
    XmTextFieldSetString( rgba_b_widget, txtstring );
    sprintf(txtstring, "%-7.3f", clr.a());
    XmTextFieldSetString( rgba_a_widget, txtstring );
  }

  if (debug) fprintf(stderr,"blockXTfunHandler::index_change exit %lx\n", 
		     (long)this);
}

void maskXTfunHandler::construct( FILE *infile, Widget neighbor )
{
  if (!valid) return;

  if (!initialized) {
    MRMRegisterArg mrm_names[1];
    mrm_names[0].name= "maskxtfunhandler_btn_press_cb";
    mrm_names[0].value= (caddr_t)maskXTfunHandler::button_press_cb;
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    initialized= 1;
  }

  // For use by create routine
  loadfile= infile;
  input_holder_widget= NULL;
  input_scroll_widget= NULL;
  input_select_widget= NULL;
  mask_select_widget= NULL;
  mask_holder_widget= NULL;
  mask_scroll_widget= NULL;
  being_deleted= 0;
  mask_xhandler= input_xhandler= NULL;
  current_build_parent= NULL;
  
  // The instantiation of the widget causes create_cb to be called,
  // which in turn calls the create method, which reads in the kids
  // and sets up their widgets.

  // Build the enclosing widget, with a pointer back here
  build( neighbor, "mask", "mask_tfun_dlog", "mask_tfun_topform" );

  if (valid) {
    XtManageChild( widget );
    if (!parent) resize();
  }

  loadfile= NULL; // To avoid accessing it later, when it might be closed
}

maskXTfunHandler::maskXTfunHandler( baseXTfunHandler* parent_in, 
				    MrmHierarchy mrm_id_in, 
				    FILE* infile, const GridInfo* grid_in,
				    Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    baseXTfunHandler(parent_in, mrm_id_in, infile, grid_in), 
    maskTfunHandler( infile, grid_in, 0 )
{
  if (debug) 
    fprintf(stderr,
	    "maskXTfunHandler::maskXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct( infile, neighbor );
  if (debug) 
    fprintf(stderr,
	    "maskXTfunHandler::maskXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

maskXTfunHandler::maskXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
				    void (*deletion_cb_in)(baseXTfunHandler*),
				    MrmHierarchy mrm_id_in, 
				    FILE* infile, const GridInfo* grid_in,
				    Widget neighbor )
  : baseTfunHandler(infile, grid_in),
    baseXTfunHandler(update_cb_in,deletion_cb_in,mrm_id_in,infile,grid_in), 
    maskTfunHandler( infile, grid_in, 0 )
{
  if (debug) 
    fprintf(stderr,
	    "maskXTfunHandler::maskXTfunHandler: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
  construct( infile, neighbor );
  if (debug) 
    fprintf(stderr,
	    "maskXTfunHandler::maskXTfunHandler exit: %lx valid= %d tfun= %lx\n",
	    (long)this,valid,(long)tfun);
}

maskXTfunHandler::~maskXTfunHandler()
{
  being_deleted= 1;

  delete mask_xhandler;
  delete input_xhandler;
}

void maskXTfunHandler::update_tfun()
{
  int ndata= 0;
  baseTransferFunction* new_mask= NULL;
  baseTransferFunction* new_input= NULL;
  valid= 1;
  if (mask_xhandler) {
    new_mask= mask_xhandler->get_tfun();
    ndata += new_mask->ndata();
  }
  if (input_xhandler) {
    new_input= input_xhandler->get_tfun();
    ndata += new_input->ndata();
  }
  if (tfun) {
    ((MaskTransferFunction*)tfun)->edit( ndata, 
					 new_input, input_weight,
					 new_mask, mask_weight );
  }
  else {
    tfun= new MaskTransferFunction( ndata, new_input, input_weight,
				    new_mask, mask_weight );
  }
}

void maskXTfunHandler::create( Widget w, int *id, unsigned long *reason )
{
  if (debug) fprintf(stderr,"maskXTfunHandler::create %lx %d\n",(long)this,*id);

  switch (*id) {
  case 1:
    {
      mask_holder_widget= w;
      
      // This is called when the widget into which mask sub-handlers are to
      // be inserted becomes available.
      
      register_weight( mask_weight );
      if (mask_present) {
	current_build_parent= mask_holder_widget;
	mask_handler= mask_xhandler= XTfunHandler_load(this, mrm_id, 
						       loadfile, &grid,
						       NULL);
	current_build_parent= NULL;
      }
      else mask_handler= mask_xhandler= NULL;
      update_tfun();
    };
    break;
  case 2:
    {
      input_holder_widget= w;
      
      // This is called when the widget into which input sub-handlers are to
      // be inserted becomes available.
      
      register_weight( input_weight );
      if (input_present) {
	current_build_parent= input_holder_widget;
	input_handler= input_xhandler= XTfunHandler_load(this, mrm_id, 
							 loadfile, &grid,
							 NULL);
	current_build_parent= NULL;
      }
      else input_handler= input_xhandler= NULL;
      update_tfun();
    };
    break;
  case 3:
    {
      mask_scroll_widget= w;
    };
    break;
  case 4:
    {
      input_scroll_widget= w;
    };
    break;
  case 5:
    {
      mask_select_widget= w;
      if (mask_present) XtSetSensitive(mask_select_widget,False);
    };
    break;
  case 6:
    {
      input_select_widget= w;
      if (input_present) XtSetSensitive(input_select_widget,False);
    };
    break;
  default: 
    baseXTfunHandler::create(w, id, reason);
  }

  if (debug) fprintf(stderr,"maskXTfunHandler::create exit %lx %d\n",
		     (long)this,*id);
}

void maskXTfunHandler::set( Widget w, int* id, unsigned long* reason )
{
  if (mask_xhandler) mask_xhandler->set( w, id, reason );
  if (input_xhandler) input_xhandler->set( w, id, reason );
  baseXTfunHandler::set(w, id, reason);
}

void maskXTfunHandler::reset( Widget w, int* id, unsigned long* reason )
{
  baseXTfunHandler::reset(w, id, reason);
  if (mask_xhandler) mask_xhandler->reset( w, id, reason );
  if (input_xhandler) input_xhandler->reset( w, id, reason );
}

Widget maskXTfunHandler::get_enclosing_widget( baseXTfunHandler* asked_by )
     const
{
  if (asked_by == mask_xhandler) return mask_holder_widget;
  else if (asked_by == input_xhandler) return input_holder_widget;
  else return current_build_parent;
}

void maskXTfunHandler::goodbye( baseXTfunHandler* asked_by )
{
  if (!being_deleted) {
    if (asked_by == mask_xhandler) {
      mask_handler= mask_xhandler= NULL;
      XtSetSensitive( mask_select_widget, True );
    }
    else if (asked_by == input_xhandler) {
      input_handler= input_xhandler= NULL;
      XtSetSensitive( input_select_widget, True );
    }

    recalc_preferred_size();
    update_tfun();
    child_changed();
    resize();
  }
}

void maskXTfunHandler::add( Widget w, int *id, unsigned long *reason )
{
  if (*id==4) {
    // Prevent evil users from loading an input tfun while the mask tfun
    // file selection dialog is up, or vice versa
    XtSetSensitive( mask_select_widget, False );
    XtSetSensitive( input_select_widget, False );
  }

  switch (most_recent_button_press) {
  case 0:
    {
      mask_weight= 1.0; register_weight(1.0);
      current_build_parent= mask_holder_widget;
      baseXTfunHandler *thiskid= load_tfun_by_number( *id, &grid, 
						      this, NULL );
      current_build_parent= NULL;
      if (!thiskid) return; // error, or file selection box popped up
      mask_handler= mask_xhandler= thiskid;
      XtSetSensitive( mask_select_widget, False );
    }
    break;
  case 1:
    {
      input_weight= 1.0; register_weight(1.0);
      current_build_parent= input_holder_widget;
      baseXTfunHandler *thiskid= load_tfun_by_number( *id, &grid, 
						      this, NULL );
      current_build_parent= NULL;
      if (!thiskid) return; // error, or file selection box popped up
      input_handler= input_xhandler= thiskid;
      XtSetSensitive( input_select_widget, False );
    }
    break;
  }

  update_tfun();
  resize();
  child_changed();
}

void maskXTfunHandler::add_file( Widget w, int *id,
				XmFileSelectionBoxCallbackStruct *call_data)
{
  if (call_data->reason == XmCR_CANCEL) {
      if (!input_xhandler) XtSetSensitive( input_select_widget, True );
      if (!mask_xhandler) XtSetSensitive( mask_select_widget, True );
  }
  else {
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
    
    FILE *tfun_file= fopen(the_name,"r");
    if (!tfun_file) {
      pop_error_dialog("invalid_tfun_file_msg");
      return;
    }
    
    switch (most_recent_button_press) {
    case 0:
      {
	mask_weight= 1.0; register_weight(1.0);
	current_build_parent= mask_holder_widget;
	baseXTfunHandler *thiskid= XTfunHandler_load( this, mrm_id, 
						      tfun_file, &grid,
						      NULL );
	current_build_parent= NULL;
	if (!thiskid) return; // error, or file selection box popped up
	mask_handler= mask_xhandler= thiskid;
	if (!input_xhandler) XtSetSensitive( input_select_widget, True );
      }
      break;
    case 1:
      {
	input_weight= 1.0; register_weight(1.0);
	current_build_parent= input_holder_widget;
	baseXTfunHandler *thiskid= XTfunHandler_load( this, mrm_id, 
						      tfun_file, &grid,
						      NULL );
	current_build_parent= NULL;
	if (!thiskid) return; // error, or file selection box popped up
	input_handler= input_xhandler= thiskid;
	if (!mask_xhandler) XtSetSensitive( mask_select_widget, True );
      }
      break;
    }
    
    fclose(tfun_file);
    
    update_tfun();
    resize();
    child_changed();
  }
}

void maskXTfunHandler::set_weight( baseXTfunHandler *asked_by, 
				  const float weight )
{
  if (asked_by==mask_xhandler) mask_weight= weight;
  else if (asked_by==input_xhandler) input_weight= weight;
  else {
    fprintf(stderr,"maskXTfunHandler::set_weight: I don't know that kid!\n");
    return;
  }
  update_tfun();
  child_changed();
}

float maskXTfunHandler::get_weight( const baseXTfunHandler* asked_by )
{  
  if (asked_by==mask_xhandler) return mask_weight;
  else if (asked_by==input_xhandler) return input_weight;
  else if (current_build_parent==mask_holder_widget) return mask_weight;
  else if (current_build_parent==input_holder_widget) return input_weight;

  fprintf(stderr,"maskXTfunHandler::get_weight: I don't know that kid!\n");
  return 0.0;
}

int maskXTfunHandler::datavol_update_inform( int datavol_removed )
{
  if (being_deleted) return 0;
  else {
    int kids_want_update= 0;

    if (mask_xhandler) 
      kids_want_update += 
	mask_xhandler->datavol_update_inform(datavol_removed);
    if (input_xhandler) 
      kids_want_update += 
	input_xhandler->datavol_update_inform(datavol_removed);
    
    return kids_want_update;
  }
}

void maskXTfunHandler::child_changed()
{
  update_tfun();
  baseXTfunHandler::child_changed();
}

baseXTfunHandler::WinSize maskXTfunHandler::recalc_preferred_size()
{
  WinSize current_total_size= get_widget_size( widget );
  WinSize current_mask_scroll_size= get_widget_size( input_scroll_widget );
  WinSize current_input_scroll_size= get_widget_size( mask_scroll_widget );

  if (mask_xhandler) 
    mask_area_size= mask_xhandler->recalc_preferred_size();
  else {
    mask_area_size.x= 0;
    mask_area_size.y= 0;
  }
  if (input_xhandler) 
    input_area_size= input_xhandler->recalc_preferred_size();
  else {
    input_area_size.x= 0;
    input_area_size.y= 0;
  }

  // Calculate size of new scrolling areas
  mask_scroll_area_size= mask_area_size + WinSize(4,4);
  input_scroll_area_size= input_area_size + WinSize(4,4);

  // Calculate size of new overall widget, retaining margins
  if (mask_scroll_area_size.x >= input_scroll_area_size.x) {
    preferred_size.x= mask_scroll_area_size.x
      + (current_total_size.x - current_mask_scroll_size.x);
  }
  else {
    preferred_size.y= input_scroll_area_size.x
      + (current_total_size.x - current_input_scroll_size.x);
  }
  preferred_size.y= mask_scroll_area_size.y + input_scroll_area_size.y
    + (current_total_size.y 
       - (current_mask_scroll_size.y + current_input_scroll_size.y));

  if (preferred_size.x<150) preferred_size.x= 150;

  if (debug) 
    fprintf(stderr,"maskXTfunHandler::recalc_preferred_size: %d %d\n",
	    preferred_size.x,preferred_size.y);
  return preferred_size;
}

void maskXTfunHandler::set_preferred_size()
{
  set_widget_size( widget, preferred_size );
  set_widget_size( mask_holder_widget, mask_area_size );
  set_widget_size( mask_scroll_widget, mask_scroll_area_size );
  set_widget_size( input_holder_widget, input_area_size );
  set_widget_size( input_scroll_widget, input_scroll_area_size );

  if (mask_xhandler) mask_xhandler->set_preferred_size();
  if (input_xhandler) input_xhandler->set_preferred_size();
}

void maskXTfunHandler::button_press_cb( Widget w, int *id, 
					unsigned long *reason )
{
  most_recent_button_press= *id;
}

