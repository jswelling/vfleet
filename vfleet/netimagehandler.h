/****************************************************************************
 * netimagehandler.h
 * Author Joel Welling
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include "lists.h"
#endif

class rgbImage;

class netImageHandler: public baseNet, public baseImageHandler { 
friend class ImageHandlerServer;
private:
  static baseImageHandler *default_ihandler;
public:
  netImageHandler( RemObjInfo *rem_in, 
		   baseImageHandler *ihandler=default_ihandler );
  netImageHandler( char *info_str= "", 
		   RemObjInfo *server= default_server );
  ~netImageHandler();
  int netrcv(NetMsgType msg);
  void display( rgbImage *image, short refresh=1);
  static void initialize( const char *name, baseImageHandler *ihandler );
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  static void shmem_display_in_order();
#endif
private:
  static int initialized;
  static void handle_ihandler_request();
  baseImageHandler *out_ihandler;
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  int partner_pe;
  long shmem_landing_area[7]; // must match size used in netRgbImage
  long* partner_shmem_landing_addr;
  void display_from_shmem();
  static sList<netImageHandler*> ihandler_list;
#endif
};

