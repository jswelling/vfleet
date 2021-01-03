/****************************************************************************
 * netdatafile.h
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

#include "basedatafile.h"

class netDataFile : public baseNet, public baseDataFile {
friend class netVRen;
friend class netDataVolume;
public:
  ~netDataFile(); // deletes real_dfile passed to constructor!
  DataFileType type() const;
  void *get_xplane( int which_x );
  void *get_yplane( int which_y );
  void *get_zplane( int which_z );
  int next_dataset();
  static void initialize( const char* name );
  int netrcv( NetMsgType msg );
  static char* pack_param_info( const char* fname_in, 
				const DataFileType type_in );
  static void unpack_param_info( char* info, char** fname_out, 
				 DataFileType& type_out );
  baseDataFile* get_real_datafile() const { return dfile; }
  int hasNamedValue( const char* key );
  union type_union getNamedValue( const char* key );
  DataElementType getNamedValueType( const char* key );
protected:
  netDataFile( RemObjInfo* rem_in, baseDataFile* real_dfile );
  netDataFile( const char *fname_in, DataFileType type_in, 
	       RemObjInfo* server= default_server );
  void add_ack_payload( NetMsgType msg, const int netrcv_retcode );
  static void handle_datafile_request();
  int open_file();
  int close_file();
  baseDataFile* dfile;
  int x_plane_to_fetch;
  int y_plane_to_fetch;
  int z_plane_to_fetch;
  char* keyToPassToChild;
  static int initialized;
  static char* param_info_buf;
  static int param_buf_size;
};

