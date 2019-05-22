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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h> /* assert */
#include "common.h"
#include "handle_events.h"

/* Allow registering a maximum a MAX_REMOVE_CB remove callback */
#define MAX_REMOVE_CB 5

static struct libusb_context *libusb_ctx = NULL;
static int event_loop_refcnt = 0;
static int init_refcnt = 0;
static pthread_t event_thread;
static pthread_mutex_t refcnt_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned int removed_cb_counter = 0;
static struct {
	libusb_pollfd_removed_cb cb;
	int fd;
} removed_cbs[MAX_REMOVE_CB];

struct list_head {
        struct list_head *prev, *next;
};
struct libusb_device_handle {
        /* lock protects claimed_interfaces */
        pthread_mutex_t lock;
        unsigned long claimed_interfaces;

        struct list_head list;
        struct libusb_device *dev;
        unsigned char os_priv[0];
};

static void *event_loop(void *ptr)
{
	int ret, refcnt;
	struct timeval tv = {1, 0};

	TRACE("Entering USB event loop\n");
	while(1) {
		/* First check if someone is still listening to libusb events */
		pthread_mutex_lock(&refcnt_mutex);
		refcnt = event_loop_refcnt;
		pthread_mutex_unlock(&refcnt_mutex);
		if(refcnt <= 0)
			break;

		/* Handle pending events */
		ret=libusb_handle_events_timeout(libusb_ctx, &tv);
		if(ret != 0) {
			printf("USB error. Exiting\n");
			break;
		}
	}
	TRACE("Exiting USB event loop\n");
	return 0;
}

static void usb_removed(int fd, void *user_data)
{
	int i;
	for(i=0; i<MAX_REMOVE_CB; i++) {
		if(removed_cbs[i].fd != fd)
			continue;
		(removed_cbs[i].cb)(fd, NULL);
	}
}

int init_libusb2(struct libusb_context **ctx, char *mUsbFs)		//add by Zone
{
	CHECK_ERROR(ctx == NULL, -1, "ctx is NULL in init_libusb()");
	if(libusb_ctx == NULL) {
		assert(init_refcnt == 0);
		//int ret = libusb_init2(&libusb_ctx, mUsbFs);
		int ret = libusb_init(&libusb_ctx);
		if(ret<0){
		    //LOGE("1111111RRGGGG");
		}
		CHECK_ERROR(ret < 0, -1, "Unable to initialize libusb");
		libusb_set_debug(libusb_ctx, 3);
	}

	*ctx = libusb_ctx;
	pthread_mutex_lock(&refcnt_mutex);
	init_refcnt++;
	pthread_mutex_unlock(&refcnt_mutex);

	return 0;
}

/* If nobody has called this function before, the event_loop thread handling
 * libusb events will be created.
 * If someone has already called start_libusb_events(), just increase the
 * reference counter.
 * Returns:
 *  -1: error: could not start the event thread
 *   0: success: event thread started */



int init_libusb(struct libusb_context **ctx)
{
	CHECK_ERROR(ctx == NULL, -1, "ctx is NULL in init_libusb()");

	if(libusb_ctx == NULL) {
		assert(init_refcnt == 0);
		int ret = libusb_init(&libusb_ctx);
		CHECK_ERROR(ret < 0, -1, "Unable to initialize libusb");
		libusb_set_debug(libusb_ctx, 3);
	}

	*ctx = libusb_ctx;
	pthread_mutex_lock(&refcnt_mutex);
	init_refcnt++;
	pthread_mutex_unlock(&refcnt_mutex);

	return 0;
}

int exit_libusb(struct libusb_context **ctx)
{
	CHECK_ERROR(ctx == NULL, -1, "ctx is NULL in exit_libusb()");
	CHECK_ERROR(*ctx != libusb_ctx, -1, "*ctx != libusb_ctx in exit_libusb()");

	pthread_mutex_lock(&refcnt_mutex);
	if(init_refcnt <=0) {
		WARNING("Libusb init reference counter is < 0 (%i)!"
					"exit_libusb() must have been called "
					"too many times.", init_refcnt);
			init_refcnt=0;
			pthread_mutex_unlock(&refcnt_mutex);
	}
	init_refcnt--;
	pthread_mutex_unlock(&refcnt_mutex);
	if(init_refcnt == 0){
		libusb_exit(libusb_ctx);
		libusb_ctx = NULL;
	}

	*ctx = NULL;
	return 0;
}

int register_libusb_removal_cb(libusb_pollfd_removed_cb removed_cb,
		libusb_device_handle *hdl)
{
	unsigned int i;

	if(removed_cb_counter > MAX_REMOVE_CB || hdl == NULL) {
		return -1;
	}

	for(i=0; i<MAX_REMOVE_CB; i++) {
		if(removed_cbs[i].cb != NULL)
			continue;
		removed_cbs[i].cb = removed_cb;
		removed_cbs[i].fd = (int) (hdl->os_priv[0]);
		removed_cb_counter++;
		TRACE2("#%u: Register callback %p for fd = %i\n", removed_cb_counter,
				removed_cbs[i].cb,
				removed_cbs[i].fd);
		break;
	}

	return 0;
}

int deregister_libusb_removal_cb(libusb_pollfd_removed_cb removed_cb)
{
	unsigned int i;
	int ret = -1;

	for(i=0; i<MAX_REMOVE_CB; i++) {
		if(removed_cbs[i].cb == removed_cb) {
			removed_cbs[i].cb = NULL;
			removed_cbs[i].fd = 0;
			removed_cb_counter--;
			ret = 0;
		}
	}
	return ret;
}

int start_libusb_events()
{
	pthread_mutex_lock(&refcnt_mutex);
	event_loop_refcnt++;
	pthread_mutex_unlock(&refcnt_mutex);

	if(event_loop_refcnt == 1) {
		libusb_set_pollfd_notifiers(libusb_ctx, NULL,
			(libusb_pollfd_removed_cb) usb_removed, NULL);

		int ret = pthread_create(&event_thread, NULL, &event_loop, NULL);
		if(ret != 0) {
			ERROR_NORET("Unable to create USB events thread!");
			pthread_mutex_lock(&refcnt_mutex);
			event_loop_refcnt=0;
			pthread_mutex_unlock(&refcnt_mutex);
			return -1;
		}
	}

	return 0;
}

/* Really stops when ALL 'listeners' have called this function.
 * Returns:
 *  -1: error
 *   0: ALL 'listeners' have called stop_libusb_events(),
 *      hence libusb callbacks won't be called anymore
 *   X: Number of listeners still active. callback functions
 *      call still be called */
int stop_libusb_events()
{
	pthread_mutex_lock(&refcnt_mutex);
	if(event_loop_refcnt<=0) {
		WARNING("Libusb event loop reference counter is < 0 (%i)!"
				"stop_libusb_events() must have been called "
				"too many times.", event_loop_refcnt);
		event_loop_refcnt=0;
		pthread_mutex_unlock(&refcnt_mutex);
		return -1;
	}
	event_loop_refcnt--;
	pthread_mutex_unlock(&refcnt_mutex);

	/* If there is no longer any listener, wait for the thread to exit */
	if(event_loop_refcnt == 0)
		pthread_join(event_thread, NULL);

	return event_loop_refcnt;
}
