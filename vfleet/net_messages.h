/****************************************************************************
 * net_messages.h
 * Author Joel Welling, Adam Beguelin
 * Copyright 1992, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
// Network messages go here.  The assumption is made that they are
// sequential starting from zero.

enum NetMsgType { 
  BASENET_ACK,        // Used only for synchronous communication by basenet
  BASENET_NAK,        // Used only for synchronous communication by basenet
  BASENET_REQBASENET, // Request basenet node
  BASENET_DELETED,    // Tell remote partner that local partner is deleted
  BASENET_DEBUG_ON,   // Tell remote partner to turn debugging on
  BASENET_DEBUG_OFF,  // Tell remote partner to turn debugging off
  BASENET_REQSERVER,  // Request server of baseServer-derived classes
  BASENET_READY,      // Successful spawn by server complete
  VREN_REQVREN,       // Request volume renderer of service manager
  VREN_READY,         // Inform remote partner that renderer network is ready
  VREN_SETOPTFLAGS,   // Set volume renderer option flags
  VREN_RENDER,        // Cause volume renderer to render an image
  VREN_ABORT,         // Abort volume render currently in progress
  VREN_ERROR,         // Error condition in remote volume renderer
  VREN_FATAL,         // Fatal error condition in remote volume renderer
  VREN_SET_CAMERA,    // Set volume renderer camera info
  VREN_SET_QUALITY,   // Set volume renderer quality measure
  VREN_SET_LIGHTINFO, // Set volume renderer light info
  VREN_SET_GEOMETRY,  // Set volume renderer volgob geometry
  VREN_UPDATE_AND_GO, // Update settings and initiate render
  VREN_REMOTE_FILE_READABLE, // Check if given file exists and is readable
  DATAVOLUME_REQDATAVOLUME, // Request datavolume of server
  DATAVOLUME_FINISH_INIT, // Cause data volume to finish initializing
  DATAVOLUME_LOAD_X,  // Cause data volume to load x plane
  DATAVOLUME_LOAD_Y,  // Cause data volume to load y plane
  DATAVOLUME_LOAD_Z,  // Cause data volume to load z plane
  DATAVOLUME_MAX_GRAD, // Cause data volume to update its max gradient value
  DATAVOLUME_LOAD_REMOTE, // Load the given netDataFile
  SAMPLEVOLUME_REQSAMPLEVOLUME, // Request samplevolume of server
  SAMPLEVOLUME_CONSTRUCT, // Initialize a newly constructed samplevolume
  SAMPLEVOLUME_REGENERATE, // Cause sample volume to fire regenerate method
  SAMPLEVOLUME_SET_SIZE_SCALE, // Set samplevolume size scale
  VOLGOB_REQVOLGOB,   // Request volgob of server
  VOLGOB_UPDATE_TRANS, // Set volgob transformation
  TFUN_REQTFUN,       // Request transfer function of server
  TFUN_PASSTFUN,      // Cause transmission of a transfer function
  LOGGER_REQLOGGER,   // Request logger of server
  LOGGER_MATCHNAMES,  // Exchange user and process info
  LOGGER_MESSAGE,     // Tell logger to log a message
  IHANDLER_REQIHANDLER, // Request image handler of service manager
  IHANDLER_DISPLAY,   // Cause image handler to display an image
  IHANDLER_SHMEM_DATA,// Used on T3D to pass buffer info for shmem comm.
  COMTEST_REQCOMTEST, // Request communications tester
  COMTEST_DOIT,       // Fire a communications test message
  DATAFILE_REQDATAFILE, // Request a netDataFile of server
  DATAFILE_MATCH,     // Match data file information
  DATAFILE_GET_X,     // get X plane
  DATAFILE_GET_Y,     // get Y plane
  DATAFILE_GET_Z,     // get Z plane
  DATAFILE_NEXT_DSET, // next dataset
  DATAFILE_OPEN_FILE, // open file
  DATAFILE_CLOSE_FILE, // close file
  DATAFILE_HAS_NAMED_VALUE, // passes datafile->hasNamedValue()
  DATAFILE_GET_NAMED_VALUE, // passes datafile->hasNamedValue()
  DATAFILE_GET_NAMED_VALUE_TYPE, // passes datafile->hasNamedValue()
  BASENET_LASTMSG };  // Insert new messages before this line

