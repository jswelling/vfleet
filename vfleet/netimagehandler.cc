/****************************************************************************
 * netimagehandler.cc
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
#include <stdio.h>
#include <stdlib.h>
#include "basenet.h"
#include "rgbimage.h"
#include "netrgbimage.h"
#include "imagehandler.h"
#include "netimagehandler.h"

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
#include <mpp/shmem.h>
#endif

/*Need to allocate space for class static variables.*/
int netImageHandler::initialized = 0;

baseImageHandler *netImageHandler::default_ihandler= NULL;

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
sList<netImageHandler*> netImageHandler::ihandler_list;
#endif

netImageHandler::netImageHandler( RemObjInfo *rem_in,
				  baseImageHandler *ihandler )
: baseNet(rem_in), baseImageHandler()
{
  if (!initialized) initialize("NoNameGiven", NULL);

  out_ihandler= ihandler;

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  if (same_host_as_partner()) {
    partner_pe= pvm_get_PE( partner_rem()->tid );
    shmem_landing_area[0]= 0; // flag to show message arrival
    partner_shmem_landing_addr= NULL; // we only get images, never send 'em

    // Send shmem info to partner
    bufsetup();
    long landing_area_addr= (long)&shmem_landing_area[0];
    netputnlong(&landing_area_addr, 1);
    (void)netsnd( IHANDLER_SHMEM_DATA );

    // Insert self for later ordered display of shmem data.
    // Insert vs. append controls order when walking the list to
    // display data.
    netImageHandler* fool_the_compiler= this;
    ihandler_list.insert(fool_the_compiler);
  }
  else {
    partner_pe= 0;
    shmem_landing_area[0]= 0;
    partner_shmem_landing_addr= NULL;
  }
#endif
}

netImageHandler::netImageHandler( char *info_str, RemObjInfo *server )
: baseNet(IHANDLER_REQIHANDLER, info_str, server), 
  baseImageHandler()
{
  if (!initialized) initialize("NoNameGiven", NULL);
  out_ihandler= NULL;

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
  if (same_host_as_partner()) {
    partner_pe= pvm_get_PE( partner_rem()->tid );
    shmem_landing_area[0]= 0; // we only send images, never get 'em
    partner_shmem_landing_addr= NULL; // until partner sends address
    // Don't add to ihandler_list, since we never receive an image.
  }
  else {
    partner_pe= 0;
    shmem_landing_area[0]= 0;
    partner_shmem_landing_addr= NULL;
  }
#endif
}

netImageHandler::~netImageHandler()
{
  // The only way we can get a current_image is by receiving it over
  // the net, so we are responsible to deleting it.
  delete current_image;
}

int netImageHandler::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {
  case IHANDLER_DISPLAY:
    {
      // Get the image from the partner     
      delete current_image;
      short refresh;
      netgetnshort( &refresh, 1 );
      current_image= netRgbImage::netget();
      if (out_ihandler) out_ihandler->display( current_image, refresh );
      else fprintf(stderr,
	    "netImageHandler: not properly initialized; can't display!\n");
    }
    break;

  case IHANDLER_SHMEM_DATA:
    {
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
      long addr_tmp;
      netgetnlong( &addr_tmp, 1 );
      partner_shmem_landing_addr= (long*)addr_tmp;
#else
      fprintf(stderr,
	      "netImageHandler::netrcv: got IHANDLER_SHMEM_DATA in error!\n");
#endif
    }
    break;

  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void netImageHandler::initialize( const char *name, 
				  baseImageHandler *ihandler )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( name );

    // Use the image handler as the default output handler
    delete default_ihandler;
    if (ihandler) default_ihandler= ihandler;

    // Add our message handler
    add_msg_handler( IHANDLER_REQIHANDLER, 
		     (void (*)())handle_ihandler_request );

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    shmem_set_cache_inv();
#endif
    initialized= 1;
  }
}

void netImageHandler::handle_ihandler_request()
{
  // Handle incoming request for a logger here
  RemObjInfo rem;
  netgetreminfo(&rem);
  baseImageHandler *ihandler= new netImageHandler( &rem, default_ihandler );
}

void netImageHandler::display( rgbImage *image, short refresh)
{
  if (out_ihandler) out_ihandler->display(image,refresh);
  else {
    netRgbImage tmp_image( *image );
#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
    // May be time for PE 0 to start listening to the world again
    extern int _MPP_MY_PE;
    if ((!_MPP_MY_PE) && (!same_host_as_partner())) {
      listen_to_everyone();
    }
    if (same_host_as_partner()) {
      // Shove the image information to the remote partner, which will
      // pick it up when it is ready.  The image has to remain valid
      // in memory until the partner pulls the pixel data across.
      tmp_image.shmem_send_self_info( partner_shmem_landing_addr, 
				     partner_pe );
    }
    else {
      // Transmit the image to the partner the old fashioned way
      bufsetup();
      netputnshort(&refresh, 1);
      tmp_image.netputself();
      (void)netsnd( IHANDLER_DISPLAY );
    }
#else
    // Send the image to the partner
    bufsetup();
    netputnshort(&refresh, 1);
    tmp_image.netputself();
    (void)netsnd( IHANDLER_DISPLAY );
#endif
  }
}

#if ( CRAY_ARCH_T3D || CRAY_ARCH_T3E )
void netImageHandler::display_from_shmem()
{
  // Await arrival of image
  shmem_wait( shmem_landing_area, 0 );
  // Reset flag, so we can tell when next message has arrived
  shmem_landing_area[0]= 0;

  // Display the result
  delete current_image;
  current_image= netRgbImage::get_from_shmem( shmem_landing_area );
  if (out_ihandler) out_ihandler->display( current_image, 1 );
  else fprintf(stderr,
	       "netImageHandler: not properly initialized; can't display!\n");
}

void netImageHandler::shmem_display_in_order()
{
  sList_iter<netImageHandler*> iter( ihandler_list );
  netImageHandler** this_ihandler;
  while (this_ihandler= iter.next()) {
    (*this_ihandler)->display_from_shmem();
  }
}
#endif
