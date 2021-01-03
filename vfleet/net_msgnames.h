/****************************************************************************
 * net_msgnames.h
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
// This file can only be included in one place!

// For each message, the string is the name and the number is non-zero if
// an acknowlegement (ACK) of the message is expected.

NetMsgInfo baseNet::MsgInfo[ BASENET_LASTMSG ]= {
  "basenet_ack", 0,
  "basenet_nak", 0,
  "basenet_reqbasenet", 1,
  "basenet_deleted", 0,
  "basenet_debug_on", 0,
  "basenet_debug_off", 0,
  "basenet_reqserver", 1,
  "basenet_ready", 0,
  "vren_reqvren", 1,
  "vren_ready", 0,
  "vren_setoptflags", 0,
  "vren_render", 0,
  "vren_abort", 0,
  "vren_error", 0,
  "vren_fatal", 0,
  "vren_set_camera", 0,
  "vren_set_quality", 0,
  "vren_set_lightinfo", 0,
  "vren_set_geometry", 0,
  "vren_update_and_go", 0,
  "vren_remote_file_readable", 1,
  "datavolume_reqdatavolume", 1,
  "datavolume_finish_init", 0,
  "datavolume_load_x", 0,
  "datavolume_load_y", 0,
  "datavolume_load_z", 0,
  "datavolume_max_grad", 0,
  "datavolume_load_remote", 0,
  "samplevolume_reqsamplevolume", 1,
  "samplevolume_construct", 0,
  "samplevolume_regenerate", 0,
  "samplevolume_set_size_scale", 0,
  "volgob_reqvolgob", 1,
  "volgob_update_trans", 0,
  "tfun_reqtfun", 1,
  "tfun_passtfun", 0,
  "logger_reqlogger", 1,
  "logger_matchnames", 1,
  "logger_message", 0,
  "ihandler_reqihandler", 1,
  "ihandler_display", 0,
  "ihandler_shmem_data", 0,
  "comtest_reqcomtest", 1,
  "comtest_doit", 0,
  "datafile_reqdatafile", 1,
  "datafile_match", 1,
  "datafile_get_x", 1,
  "datafile_get_y", 1,
  "datafile_get_z", 1,
  "datafile_next_dset", 1,
  "datafile_open_file", 1,
  "datafile_close_file", 1,
  "datafile_has_named_value", 1,
  "datafile_get_named_value", 1,
  "datafile_get_named_value_type", 1,
};

