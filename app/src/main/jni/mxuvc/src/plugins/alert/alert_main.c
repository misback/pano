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
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <pthread.h>
#include <unistd.h>
#include "libusb/handle_events.h"
#include "mxuvc.h"
#include "alert.h"
#include "common.h"

struct libusb_device_handle *camera = NULL;
struct libusb_context *ctxt = NULL;
const struct libusb_interface *intf = NULL;

pthread_mutex_t count_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;

int disable_alarm = 0;
int kill_thread =0;
pthread_t alarm_thread_id = 0;

//state machine
typedef enum 
{
	STOP = 0,
	RUNNING,
	UNDEFINED
}ALERT_THRD_STATE;

typedef struct {
	unsigned int audioThresholdDB;		
}audio_alert_setting;

typedef struct {
	int reg_id;	/* region number; used when alert is generated */
	int x_offt;	/* starting x offset */
	int y_offt;	/* starting y offset */
	int width;	/* width of region */
	int height;	/* height of region */
	int sensitivity;	/* threshold for motion detection for a macroblock */
	int trigger_percentage;
}motion_alert_setting;

typedef struct {
	alert_type  type;
	alert_action  action;
	audio_alert_setting audio;
	i2c_notification_setting i2c_setting;
    gpio_notification_setting gpio_setting;
}alert_setting;

typedef struct {
	alert_type  type;
	audio_alert_info aalert;
	i2c_alert_info i2c_data;
    gpio_alert_info gpio_data;
}alert_info;

static int32_t alert_thread_state = STOP;

//alert count
static int alert_state = 0;

//local functions
static void *mxuvc_alert_thread();
static int start_alert_thread(void);


/* initialize alert plugin */
int mxuvc_alert_init(void)
{
	int ret=0, count, i=0;
	struct libusb_config_descriptor *conf_desc;
	int scan_result = 0;
	struct libusb_device **devs=NULL;
	struct libusb_device *dev;
	struct libusb_device_descriptor desc;
	struct libusb_device_handle *devhandle = NULL;
	const struct libusb_interface *dev_interface;
	const struct libusb_interface_descriptor *altsetting;

	TRACE("alert plugin init\n");

	alarm_thread_id = 0;
	kill_thread = 0;	
	disable_alarm = 0;

	if (camera==NULL){	
		ret = init_libusb(&ctxt);
		//ret = libusb_init(&ctxt);
		if (ret) {
			TRACE("libusb_init failed\n");
			return -1;
		}
		if (libusb_get_device_list(ctxt, &devs) < 0)
		{
			TRACE("libusb_get_device_list error\n");
			return -1;	
		}
		while ((dev = devs[i++]) != NULL) {

			int data[2] = {-1, -1};

			ret = libusb_get_device_descriptor(dev, &desc);
			if (ret < 0)
				continue;
			ret = libusb_get_config_descriptor_by_value(dev, 1, &conf_desc);
			if(ret < 0)
				continue;

			dev_interface = conf_desc->interface;		
			altsetting = dev_interface->altsetting;
			/* We are only interested in devices whose first USB class is
			 *  - a Vendor specific class
			 *  - a UVC class
			 * */
			if (altsetting->bInterfaceClass != VENDOR_SPECIFIC
					&& altsetting->bInterfaceClass != CC_VIDEO) {
				libusb_free_config_descriptor(conf_desc);
				continue;
			}

			/* Open the device to communicate with it */
			ret = libusb_open(dev, &devhandle);
			if (ret < 0) {
				continue;
			}
			ret = libusb_control_transfer(devhandle,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ CMD_WHO_R_U,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data,
				/* wLength       */ 8,
				/* timeout*/  LIBUSB_CMD_TIMEOUT
				);

			switch(data[0]) {
				case MAX64380:
				case MAX64480:
				case MAX64580:
					//got the Geo camera
					scan_result = 1;
				break;
				default:
				break;	
			}
			if(scan_result == 1){
				if(data[1] != 0)
					scan_result = 0;
			}
			if(scan_result == 1)
				break;
			else
				libusb_close(devhandle);
					
		}
		if(scan_result == 1)	
			camera = devhandle;	
		else {
			TRACE("ERR: Opening camera failed\n");
			return -1;
		}
		ret = libusb_get_active_config_descriptor(dev, &conf_desc);
		if (ret < 0) {
			TRACE("Cannot retrive active config descriptor\n");
			return -1;
		}
		//array = conf_desc->interface;
		for (count = 0; count < conf_desc->bNumInterfaces; count++) {
			intf = (conf_desc->interface + count);
			/* FIXME TODO not the best check; but sufficient for now */
			if (intf->altsetting->bInterfaceClass == 
						LIBUSB_CLASS_VENDOR_SPEC)
				break;
		}
		if (count < conf_desc->bNumInterfaces) {
			ret = libusb_claim_interface(camera, 
						intf->altsetting->bInterfaceNumber);
			if (ret < 0) {
				TRACE("Cannot claim interface %d\n", 
						intf->altsetting->bInterfaceNumber);
				return -1;
			}
			ret = libusb_set_interface_alt_setting(camera, 
					intf->altsetting->bInterfaceNumber, 1);
			if (ret < 0) {
				TRACE("Cannot set interface\n");
				return -1;
			}
		} else {
			TRACE("Cannot find a suitable interface\n");
			return -1;
		}
	} else
		return -1;

	return 0;	
}

int mxuvc_alert_deinit(void)
{
	int ret = 0;
	TRACE("alert plugin deinit\n");
	alert_thread_state = STOP;
	disable_alarm = 1;
	if(alarm_thread_id){
		pthread_mutex_lock( &count_mutex );
		pthread_cond_signal( &condition_var );
		pthread_mutex_unlock( &count_mutex );

		(void) pthread_join(alarm_thread_id, NULL);
	}
	alert_state = 0;
	/*CHECK_ERROR(alert_state != 0, -1,
			"Unable to deinit alert module alert_state is not zero");*/

	if(intf && camera)
		ret = libusb_set_interface_alt_setting(camera, 
				intf->altsetting->bInterfaceNumber,
				0);

	if(ctxt && camera){
		libusb_release_interface (camera, 
					intf->altsetting->bInterfaceNumber);
		libusb_close (camera);
		exit_libusb(&ctxt);

		ctxt = NULL;
		intf = NULL;
		camera = NULL;
	}
		
	return ret;
}

/* this function sends all the alert related control events to host */
int set_alert(alert_type type,
		alert_action	action,
		unsigned int audioThresholdDB,
		i2c_notification_setting *i2cn,
		int check_status,
		uint32_t expected_status_value,
		uint32_t status_wait_time_ms,
        gpio_notification_setting *gpion)
{
	alert_setting setting;
	int ret = 0;
	CHECK_ERROR(camera == NULL, -1, "Alert module is not enabled");

    setting.type.d32 = 0;

	if(type.b.audio_alert == 1){
		setting.type.b.audio_alert = 1;
		setting.action = action;
		setting.audio.audioThresholdDB = audioThresholdDB;
		//TRACE("Setting Audio Alert\n");
	}else if(type.b.i2c_alert == 1){
		setting.type.b.i2c_alert = 1;
		setting.action = action;
		setting.i2c_setting = *i2cn;
		//TRACE("Setting i2c notification\n");
	}else if(type.b.gpio_alert == 1){
        setting.type.b.gpio_alert = 1;
        setting.action = action;
        setting.gpio_setting = *gpion;
    }else{
		TRACE("ERROR: Unsupported Alert type");
		return -1;
	}

	if(camera){
		//enable/disable alert
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ 0x29, //AV_ALARM
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&setting,
				/* wLength       */ sizeof(alert_setting),
				/* timeout*/    LIBUSB_CMD_TIMEOUT
				);
		CHECK_ERROR(ret < 0, -1, "setting alert failed");

		if(check_status == 1){
			uint32_t status;
			uint32_t timeout = status_wait_time_ms;
			//check if status is required
			usleep((timeout*1000)/2);
			timeout = timeout/2;
			while(timeout > 0){
				ret = libusb_control_transfer(camera,
					/* bmRequestType */
					(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
					 LIBUSB_RECIPIENT_INTERFACE),
					/* bRequest      */ 0x29, //AV_ALARM
					/* wValue        */ 0,
					/* MSB 4 bytes   */
					/* wIndex        */ 0,
					/* Data          */ (unsigned char *)&status,
					/* wLength       */ sizeof(uint32_t),
					/* timeout*/    (LIBUSB_CMD_TIMEOUT*2)
				);
				CHECK_ERROR(ret < 0, -1, "status read failed");
				if(status == expected_status_value)
					break;
				timeout = timeout - (LIBUSB_CMD_TIMEOUT*2);
			}
			if(status != expected_status_value){
				printf("ERR: read status failed %d\n",status);
				return -1;
			}
		}
	}

	if((alert_state == 0) && (action == ALARM_ACTION_ENABLE)){
		//start the alert thread
		ret = start_alert_thread();
		CHECK_ERROR(ret < 0, -1, "alert_thread_state failed");
		sleep(1);
		pthread_mutex_lock( &count_mutex );
		pthread_cond_signal( &condition_var );
		pthread_mutex_unlock( &count_mutex );	
	}
	
	if(action == ALARM_ACTION_ENABLE)	
		alert_state++;
	else if(action == ALARM_ACTION_DISABLE)
		alert_state--;

	return 0;
}
	
int start_alert_thread(void)
{
	int ret;

	if(camera && intf && (alert_thread_state == STOP))
	{
		alert_thread_state = RUNNING;
		pthread_attr_t thread_att;
		pthread_attr_init(&thread_att);
		pthread_attr_setdetachstate(&thread_att, PTHREAD_CREATE_JOINABLE);

		ret = pthread_create( &alarm_thread_id, &thread_att, 
			&mxuvc_alert_thread, NULL);
		if(ret){
			TRACE("%s:pthread_create failed, ret = %d",__func__,ret);			
			alert_thread_state = STOP;
			return -1;
		}
		pthread_attr_destroy(&thread_att);
	}
	
	return 0;
}

int check_alert_state(void)
{
	if(camera && intf)
		return 1; //enabled

	return 0;	
}

void *mxuvc_alert_thread()
{
	int rx_size;
	alert_info alert;
	int ret;

	while(alert_thread_state == RUNNING) {
		//wait for alarm enable
		pthread_mutex_lock( &count_mutex );
		pthread_cond_wait( &condition_var, &count_mutex );
		pthread_mutex_unlock( &count_mutex );

		TRACE("Alert Enabled or Exiting...\n");

		while((alert_thread_state == RUNNING) && (disable_alarm == 0)){
			rx_size = 0;
			memset(&alert, 0, sizeof(alert_info));
			ret = libusb_interrupt_transfer(camera,
							0x86,
							(unsigned char *)&alert,
							sizeof(alert_info),
							&rx_size, LIBUSB_CMD_TIMEOUT*2);
			if((ret == LIBUSB_ERROR_TIMEOUT) && (!disable_alarm)){
				//TRACE("ERROR: Interrupt transfer failed %d\n", ret);
				continue;
			}
			if ((rx_size > 0) && (!disable_alarm)){
				if(alert.type.b.audio_alert){
					proces_audio_alert(&alert.aalert);
				}
				if(alert.type.b.i2c_alert)
					i2c_notification_fn(&alert.i2c_data);
                if(alert.type.b.gpio_alert)
                    gpio_notification_process(&alert.gpio_data);
			}
		}
	}
	
	return NULL;

}

int get_audio_intensity(uint32_t *audioIntensityDB)
{
	int ret=0;

	if(camera){
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest     */  A_INTENSITY,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)audioIntensityDB,
				/* wLength       */ sizeof(int),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
			if (ret < 0) {
				TRACE("ERROR: Audio Intensity request failed\n");
				return -1;
			}
	} else {
		TRACE("%s:ERROR-> Alert module is not enabled",__func__);
		return -1;
	}

	return 0;
}

int i2c_rw(i2c_payload_t *payload, i2c_data_t *data, uint16_t inst, uint16_t type, bool set_req)
{
	uvc_vendor_req_id_t id;
	i2c_data_t i2c_stat;
	int retry = 0;
	int r;

	CHECK_ERROR(camera == NULL, -1, "Alert module is not enabled");

	id = (set_req == 1) ? I2C_WRITE : I2C_READ;

	r = libusb_control_transfer(camera,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ id,
                /* wValue        */ (uint16_t)inst,
                /* MSB 4 bytes   */
                /* wIndex        */ (uint16_t)type,
                /* Data          */ (unsigned char *)payload,
                /* wLength       */ (uint16_t)sizeof(i2c_payload_t),
                /* timeout*/   LIBUSB_CMD_TIMEOUT
                );
	CHECK_ERROR(r<0, -1, "Failed I2C_READ %d\n", r);	

	id = (set_req == 1) ? I2C_WRITE_STATUS : I2C_READ_STATUS;

	while(retry < 10){
		usleep(1000*10);

        r = libusb_control_transfer(camera,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                     LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ id,
                    /* wValue        */ 0,
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char *)&i2c_stat,
                    /* wLength       */ sizeof(i2c_data_t),
                    /* timeout*/   LIBUSB_CMD_TIMEOUT
                    );
		CHECK_ERROR(r<0, -1, "I2C_READ/WRITE staus Failed %d\n", r);

		CHECK_ERROR(i2c_stat.len < 0, -1 , "I2C_READ/WRITE staus Failed");

		if(i2c_stat.len == 0xF){
			retry++;
		}else
			break;
	}
    CHECK_ERROR(((i2c_stat.len == 0xF) || (i2c_stat.len > 2)), -1 , 
                                    "I2C_READ/WRITE staus Timeout");

	*data = i2c_stat;

	return 0;
}
