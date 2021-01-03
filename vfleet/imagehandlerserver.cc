/****************************************************************************
 * imagehandlerserver.cc
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
#include "imagehandler.h"
#include "imagehandlerserver.h"

/*Need to allocate space for class static variables.*/
int ImageHandlerServer::initialized = 0;

baseImageHandler *ImageHandlerServer::default_ihandler= NULL;

ImageHandlerServer::ImageHandlerServer() 
: baseServer(ImageHandlerServerExeName), 
  baseNet( BASENET_REQSERVER, "", server_info )
{
  // The baseServer constructor fires up the executable, and the
  // baseNet constructor fires a requst to it which causes us to
  // get connected.  Thus, we\'re done.
}

ImageHandlerServer::ImageHandlerServer( RemObjInfo *rem_in )
: baseNet(rem_in), baseServer()
{
}

ImageHandlerServer::~ImageHandlerServer()
{
  // Nothing to be done
}

int ImageHandlerServer::netrcv( NetMsgType msg )
{
  int rcv_ok= 1;
  switch (msg) {

  case IHANDLER_REQIHANDLER: // request for new ImageHandler- purpose of server
    {
      RemObjInfo remote;
      netgetreminfo(&remote);
      fprintf(stderr,
	      "ImageHandlerServer::netrcv: sender= <%s> %d %d\n",
	      remote.component,remote.instance,remote.object);
      // Too many people to talk to to use ACK mechanism
      send_ack_and_break_lock();
      baseImageHandler *ihandler= 
	new netImageHandler( &remote, default_ihandler );
    }
    break;
    
  default: rcv_ok= baseNet::netrcv(msg);
  }
  return( rcv_ok );
}

void ImageHandlerServer::initialize( baseImageHandler *ihandler, char *cmd )
{
  if (!initialized) {
    // Initialize network handling
    baseNet::initialize( cmd );

    default_ihandler= ihandler;

    // Add our message handler
    add_msg_handler( BASENET_REQSERVER, 
		     (void (*)())handle_server_request );

    initialized= 1;
  }
}

void ImageHandlerServer::handle_server_request()
// This routine catches the initial server request from the partner
{
  RemObjInfo rem;
  baseNet::netgetreminfo(&rem);
  ImageHandlerServer *ihandlerserver= new ImageHandlerServer(&rem);
}

void ImageHandlerServer::create_ihandler( RemObjInfo *rem_in )
// Create a partner for the netImageHandler at rem_in
{
  break_ack_lock(); // so we can send to server, with will ack to requestor
  bufsetup();
  netputreminfo(rem_in);
  netsnd(IHANDLER_REQIHANDLER);
}

