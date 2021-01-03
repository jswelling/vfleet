/****************************************************************************
 * xtfunhandler.h 
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

class baseXTfunHandler;

struct xtfun_file_write_warning_data {
  char *fname;
  Widget to_be_closed;
  baseXTfunHandler *handler;
};

class baseXTfunHandler : virtual public baseTfunHandler {
friend class sumXTfunHandler;
friend class ssumXTfunHandler;
friend class maskXTfunHandler;
public:
  struct WinSize {
    Dimension x;
    Dimension y;
    WinSize( const int x_in= 0, const int y_in= 0 ) 
      { x= (Dimension)x_in; y= (Dimension)y_in; }
    WinSize operator+( const WinSize& o ) const
    { return WinSize( x+o.x, y+o.y ); }
    WinSize& operator+=(const WinSize& o )
    { x += o.x; y += o.y; return *this; }
    int operator==( const WinSize& o ) const
    { return ((x==o.x) && (y==o.y)); }
    int operator!=( const WinSize& o ) const
    { return ((x!=o.x) || (y!=o.y)); }
  };
  baseXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id_in,
		    FILE* infile, const GridInfo* grid_in );
  baseXTfunHandler( void (*update_cb_in)(baseXTfunHandler* update_me),
		    void (*deletion_cb_in)(baseXTfunHandler* delete_me),
		    MrmHierarchy mrm_id_in, FILE* infile, 
		    const GridInfo* grid_in );
  virtual ~baseXTfunHandler();
  Widget get_widget() const { return widget; }
  baseXTfunHandler* get_parent() const { return parent; }
  virtual Widget get_enclosing_widget( baseXTfunHandler *asked_by ) const;
  virtual void goodbye( baseXTfunHandler *asked_by );
  virtual void set_weight( baseXTfunHandler *asked_by, const float weight ) {}
  virtual float get_weight( const baseXTfunHandler *asked_by )
    { return 0.0; }
  virtual int datavol_update_inform( int datavol_removed ) { return 0; }
  virtual void resize();
  struct object_lookup_struct {
    int id;
    baseXTfunHandler* handler;
    object_lookup_struct(const int id_in, baseXTfunHandler* handler_in)

      { id= id_in; handler= handler_in; }
  };
protected:
  static dList<object_lookup_struct*> instance_list;
  MrmHierarchy mrm_id;
  baseXTfunHandler *parent;
  void (*update_cb)(baseXTfunHandler* update_me);
  void (*delete_request_cb)(baseXTfunHandler* delete_me);
  virtual void child_changed();
  static Widget add_file_selection_dialog;
  Widget widget;
  Widget save_file_selection_dialog;
  Widget weight_text_widget;
  static int initialized;
  static int id_seed;
  int id;
  WinSize preferred_size;
  void push_object_name();
  void pop_object_name();
  static sList<MrmRegisterArg*> object_name_stack;
  static baseXTfunHandler* get_object( Widget w );
  void register_weight( const float weight );
  Widget update_datavol_menu( Widget w, Widget parent,
			      dvol_struct* selected, Widget set_target,
			      XtCallbackProc button_cb,
			      sList<Widget>* button_list );
  int get_selected_dvol_index( Widget dvol_rowcol );
  void build( Widget neighbor, char* type_name, char* dialog_name, 
	      char* widget_name );
  static void create_cb( Widget w, int *id, unsigned long *reason );
  virtual void create( Widget w, int *id, unsigned long *reason );
  static void expose_cb( Widget w, int *id, unsigned long *reason );
  virtual void expose( Widget w, int *id, unsigned long *reason ) {}
  static void set_edit_color_cb( Widget w, int *id, unsigned long *reason );
  virtual void set_edit_color( Widget w, int *id, unsigned long *reason ) {}
  static void set_cb( Widget w, int *id, unsigned long *reason );
  virtual void set( Widget w, int *id, unsigned long *reason );
  static void reset_cb( Widget w, int *id, unsigned long *reason );
  virtual void reset( Widget w, int *id, unsigned long *reason );
  static void delete_cb( Widget w, int *id, unsigned long *reason );
  static void save_cb( Widget w, int *id, unsigned long *reason );
  virtual void save( Widget w, int *id, unsigned long *reason );
  static void save_file_cb( Widget w, int *id, 
			   XmFileSelectionBoxCallbackStruct *call_data);
  virtual void save_file( Widget w, int *id, 
			 XmFileSelectionBoxCallbackStruct *call_data );
  static void delete_tfun_anyway_cb( Widget w, caddr_t data_in,
				     caddr_t call_data );
  static void add_cb( Widget w, int *id, unsigned long *reason );
  virtual void add( Widget w, int *id, unsigned long *reason );
  static void add_file_cb( Widget w, int *id,
			  XmFileSelectionBoxCallbackStruct *call_data );
  virtual void add_file( Widget w, int *id,
			XmFileSelectionBoxCallbackStruct *call_data );
  static WinSize get_widget_size( Widget w );
  static void set_widget_size( Widget w, WinSize& winsize );
  static baseXTfunHandler* load_tfun_by_number( const int id,
						const GridInfo* grid_in,
						baseXTfunHandler* parent,
						Widget neighbor );
  virtual WinSize recalc_preferred_size();
  virtual void set_preferred_size();
  static int int_from_text_widget( Widget w, const int min, const int max );
  static float float_from_text_widget(Widget w, 
				      const float min, const float max);
  static void ordered_pair_from_text_widgets( int* v1, int* v2,
					      Widget w1, Widget w2, 
					      const int min, const int max );
  static void ordered_pair_from_text_widgets( float* v1, float* v2,
					      Widget w1, Widget w2, 
					      const float min, 
					      const float max );
};

baseXTfunHandler* XTfunHandler_load( baseXTfunHandler *parent_in, 
				    MrmHierarchy mrm_id,
				    FILE* ifile, const GridInfo* grid_in,
				    Widget neighbor=NULL );
				    
typedef void (*XTfunHandler_cb)(baseXTfunHandler*);

baseXTfunHandler* XTfunHandler_load( XTfunHandler_cb update_cb_in,
				     XTfunHandler_cb deletion_cb_in,
				     MrmHierarchy mrm_id,
				     FILE* ifile, const GridInfo* grid_in,
				     Widget neighbor=NULL );
				    
class bboxXTfunHandler: public baseXTfunHandler, public bboxTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
public:
  bboxXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id, 
		    FILE *infile, const GridInfo* grid_in, 
		    Widget neighbor=NULL );
  bboxXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		    void (*deletion_cb_in)(baseXTfunHandler*),
		    MrmHierarchy mrm_id_in,
		    FILE *infile, const GridInfo* grid_in, 
		    Widget neightbor= NULL );
protected:
  void construct();
  static int initialized;
};

class tableXTfunHandler: public baseXTfunHandler, 
			 virtual public tableTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
public:
  tableXTfunHandler( baseXTfunHandler* parent_in, 
		     MrmHierarchy mrm_id, FILE *infile, 
		     const GridInfo* grid_in, 
		     char* dialog_name= NULL, char* widget_name= NULL,
		     Widget neighbor= NULL );
  tableXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		     void (*deletion_cb_in)(baseXTfunHandler*),
		     MrmHierarchy mrm_id, 
		     FILE *infile, const GridInfo* grid_in, 
		     char* dialog_name= NULL, char* widget_name= NULL,
		     Widget neighbor= NULL );
  ~tableXTfunHandler();
  int datavol_update_inform( int datavol_removed );
protected:
  static int initialized;
  static GC r_gc, g_gc, b_gc, a_gc, prev_gc;
  void construct();
  void create( Widget w, int *id, unsigned long *reason );
  void expose( Widget w, int *id, unsigned long *reason );
  void set_edit_color( Widget w, int *id, unsigned long *reason );
  virtual void set( Widget w, int *id, unsigned long *reason );
  virtual void reset( Widget w, int *id, unsigned long *reason );
  static void dvol_button_press_cb( Widget w, XtPointer client, 
				   XtPointer call );
  void dvol_button_press( Widget w, XtPointer client, XtPointer call );
  static void graph_window_input_cb( Widget w, baseXTfunHandler* handler,
				    XEvent *event );
  void graph_window_input( Widget w, XEvent *event );
  virtual gBColor* get_color_table(); // returns pointer, not copy
  virtual void set_color_table();
  virtual void reset_color_table();
  virtual void update_image();
  void update_graph();
  XImageHandler *ihandler;
  rgbImage *image;
  Display* dpy;
  Widget graph_widget;
  XPoint *rpoints;
  XPoint *gpoints;
  XPoint *bpoints;
  XPoint *apoints;
  int edit_which_color;
  int drag_start_valid;
  XPoint drag_start;
  Widget datavol_selection_widget;
  Widget datavol_option_menu;
  sList<Widget> datavol_menu_button_list;
};

class gradtableXTfunHandler: virtual public tableXTfunHandler, 
			     virtual public gradtableTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
public:
  gradtableXTfunHandler( baseXTfunHandler* parent_in, 
			 MrmHierarchy mrm_id, 
			 FILE *infile, const GridInfo* grid_in,
			 Widget neighbor=NULL );
  gradtableXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
			 void (*deletion_cb_in)(baseXTfunHandler*),
			 MrmHierarchy mrm_id, 
			 FILE *infile, const GridInfo* grid_in,
			 Widget neighbor=NULL );
  ~gradtableXTfunHandler();
  int save( FILE* ofile ) { return gradtableTfunHandler::save(ofile); }
protected:
  virtual gBColor* get_color_table(); // returns pointer, not copy
  void set_color_table();
};

class sumXTfunHandler: public baseXTfunHandler, virtual public sumTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
friend class ssumXTfunHandler;
public:
  sumXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id, 
		   FILE *infile, const GridInfo* grid_in, 
		   Widget neighbor=NULL );
  sumXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id,
		   FILE *infile, const GridInfo* grid_in, 
		   Widget neighbor, char* type_name,
		   char* dlog_name, char* widget_name );
  sumXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		   void (*deletion_cb_in)(baseXTfunHandler*),
		   MrmHierarchy mrm_id, 
		   FILE *infile, const GridInfo* grid_in, 
		   Widget neighbor=NULL );
  sumXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		   void (*deletion_cb_in)(baseXTfunHandler*),
		   MrmHierarchy mrm_id, 
		   FILE *infile, const GridInfo* grid_in, 
		   Widget neighbor, char* type_name,
		   char* dlog_name, char* widget_name );
  ~sumXTfunHandler();
  Widget get_enclosing_widget( baseXTfunHandler *asked_by ) const;
  void goodbye( baseXTfunHandler *asked_by );
  void set_weight( baseXTfunHandler *asked_by, const float weight );
  float get_weight( const baseXTfunHandler* asked_by );
  int datavol_update_inform( int datavol_removed );
protected:
  void child_changed();
  FILE *loadfile;
  dList<baseXTfunHandler*> xkids;
  Widget kid_holder_widget;
  Widget kid_scroll_widget;
  void construct( FILE *infile, Widget neighbor, char* type_name,
		  char* dlog_name, char* widget_name );
  void create( Widget w, int *id, unsigned long *reason );
  void set( Widget w, int* id, unsigned long* reason );
  void reset( Widget w, int* id, unsigned long* reason );
  static int initialized;
  void add( Widget w, int *id, unsigned long *reason );
  void add_file( Widget w, int *id,
		 XmFileSelectionBoxCallbackStruct *call_data );
  virtual void update_tfun();
  void connect_child_widgets();
  int being_deleted;
  int child_build_in_progress;
  float weight_of_new_child;
  WinSize kid_area_size;
  WinSize scroll_area_size;
  virtual WinSize recalc_preferred_size();
  virtual void set_preferred_size();
};

class ssumXTfunHandler: virtual public ssumTfunHandler, 
			virtual public sumXTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
public:
  ssumXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id, 
		    FILE *infile, const GridInfo* grid_in, 
		    Widget neighbor=NULL );
  ssumXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		    void (*deletion_cb_in)(baseXTfunHandler*),
		    MrmHierarchy mrm_id, 
		    FILE *infile, const GridInfo* grid_in, 
		    Widget neighbor=NULL );
  ~ssumXTfunHandler();
protected:
  void update_tfun();
  int save( FILE* ofile ) { return ssumTfunHandler::save(ofile); }
  void append_needed_dvols( int *count, sList<dvol_struct*> *needed )
  { ssumTfunHandler::append_needed_dvols(count, needed); }
  void refit_to_volume( const GridInfo* grid_in ) 
  { ssumTfunHandler::refit_to_volume(grid_in); }
};

class blockXTfunHandler: public baseXTfunHandler, public blockTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
public:
  blockXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id, 
		     FILE *infile, const GridInfo* grid_in, 
		     Widget neighbor=NULL );
  blockXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		     void (*deletion_cb_in)(baseXTfunHandler*),
		     MrmHierarchy mrm_id_in,
		     FILE * infile, const GridInfo* grid_in,
		     Widget neightbor= NULL );
  ~blockXTfunHandler();
  void refit_to_volume( const GridInfo* grid_in );
protected:
  void construct();
  static void index_change_cb( Widget w, int *id, unsigned long *reason );
  virtual void index_change( Widget w, int *id, unsigned long *reason );
  void create( Widget w, int *id, unsigned long *reason );
  void set( Widget w, int *id, unsigned long *reason );
  void reset( Widget w, int *id, unsigned long *reason );
  static int initialized;
  Widget llf_x_widget;
  Widget llf_y_widget;
  Widget llf_z_widget;
  Widget trb_x_widget;
  Widget trb_y_widget;
  Widget trb_z_widget;
  Widget rgba_r_widget;
  Widget rgba_g_widget;
  Widget rgba_b_widget;
  Widget rgba_a_widget;
  Widget inside_tb_widget;
  Widget index_tb_widget;
};

class maskXTfunHandler: public baseXTfunHandler, public maskTfunHandler {
friend baseXTfunHandler* XTfunHandler_load( baseXTfunHandler* parent_in, 
					    MrmHierarchy mrm_id, 
					    FILE *infile, 
					    const GridInfo* grid_in,
					    Widget neighbor );
public:
  maskXTfunHandler( baseXTfunHandler* parent_in, MrmHierarchy mrm_id, 
		  FILE *infile, const GridInfo* grid_in, 
		  Widget neighbor=NULL );
  maskXTfunHandler( void (*update_cb_in)(baseXTfunHandler*),
		  void (*deletion_cb_in)(baseXTfunHandler*),
		  MrmHierarchy mrm_id, 
		  FILE *infile, const GridInfo* grid_in, 
		  Widget neighbor=NULL );
  ~maskXTfunHandler();
  Widget get_enclosing_widget( baseXTfunHandler *asked_by ) const;
  void goodbye( baseXTfunHandler *asked_by );
  void set_weight( baseXTfunHandler *asked_by, const float weight );
  float get_weight( const baseXTfunHandler* asked_by );
  int datavol_update_inform( int datavol_removed );
protected:
  static void button_press_cb(Widget w, int* id, unsigned long* reason);
  static int most_recent_button_press;
  void child_changed();
  FILE *loadfile;
  baseXTfunHandler* mask_xhandler;
  baseXTfunHandler* input_xhandler;
  Widget input_holder_widget;
  Widget input_scroll_widget;
  Widget input_select_widget;
  Widget mask_holder_widget;
  Widget mask_scroll_widget;
  Widget mask_select_widget;
  Widget current_build_parent;
  void construct( FILE *infile, Widget neighbor );
  void create( Widget w, int *id, unsigned long *reason );
  void set( Widget w, int* id, unsigned long* reason );
  void reset( Widget w, int* id, unsigned long* reason );
  static int initialized;
  virtual void add( Widget w, int *id, unsigned long *reason );
  virtual void add_file( Widget w, int *id,
			 XmFileSelectionBoxCallbackStruct *call_data );
  void update_tfun();
  int being_deleted;
  WinSize input_area_size;
  WinSize input_scroll_area_size;
  WinSize mask_area_size;
  WinSize mask_scroll_area_size;
  virtual WinSize recalc_preferred_size();
  virtual void set_preferred_size();
};

