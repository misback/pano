/*******************************************************************************
* 
* The content of this file or document is CONFIDENTIAL and PROPRIETARY
* to GEO Semiconductor.  It is subject to the terms of a License Agreement 
* between Licensee and GEO Semiconductor, restricting among other things,
* the use, reproduction, distribution and transfer.  Each of the embodiments,
* including this information and any derivative work shall retain this 
* copyright notice.
* 
* Copyright 2013-2016 GEO Semiconductor, Inc.
* All rights reserved.
*
* 
*******************************************************************************/

#include <libusb/libusb.h>

/* The following functions must be used in order to share libusb event
 * handling with the different backends */

/* Call instead of libusb_init() */
int init_libusb(struct libusb_context **ctx);
/* add by Zone */
int init_libusb2(struct libusb_context **ctx, char *mUsbFs);
/* Call instead of libusb_exit() */
int exit_libusb(struct libusb_context **ctx);

int register_libusb_removal_cb(libusb_pollfd_removed_cb removed_cb,
		libusb_device_handle *hdl);
int deregister_libusb_removal_cb(libusb_pollfd_removed_cb removed_cb);

/* Call those functions in order to start/stop libusb event handling */
int start_libusb_events();
int stop_libusb_events();
