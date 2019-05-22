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
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <pthread.h>
#include <math.h>

#include "mxuvc.h"

// Use this example for OPT3001 ALS

void i2c_notify_cb(int value)
{
	printf("\ni2c_notify_cb 0x%x\n",(uint16_t)value);
}

int main(int argc, char **argv)
{
	int ret;
	int counter=0;
	char data;
	int i2c_instance = 0; 
	int i2c_device_type = 0; 
	//while covering the lense and uncovering it i2c alert will come.
	uint16_t i2c_device_addr = 0x89;
	uint16_t chip_id_reg = 0;   //Not using detection
	uint16_t chip_id_reg_value = 0;  //Not using detection
	int handle;
	uint32_t read_len = 0;
	uint16_t regval = 0;

	ret = mxuvc_alert_init();
	if(ret < 0)
		goto error;
	
	printf("\n");

	//sleep
	usleep(5000);

	//start i2c notification
	handle = mxuvc_i2c_open(i2c_instance, i2c_device_type, i2c_device_addr, 
			chip_id_reg, chip_id_reg_value);
	if(handle <= 0){
		printf("err: mxuvc_i2c_open failed\n");
		goto error;	
	}
	//Check device ID
	ret = mxuvc_i2c_notification_read_register(handle, 0x7f, &regval, 2);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_read_register failed\n");
		goto error;	
	}
	printf("reg 0x7f val 0x%x\n",regval);
	if(regval != 0x3001) {
		printf("err: OPT3001 ALS not found\n"); 
		goto error;	
	}
	//Configuration for continuous reading
	ret = mxuvc_i2c_notification_write_register(handle, 0x01, 0xce10, 2);
	//Configuration for single reading - don't use it for this application
	//ret = mxuvc_i2c_notification_write_register(handle, 0x01, 0xca10, 2);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_write_register failed\n");
		goto error;	
	}
	ret = mxuvc_i2c_notification_read_register(handle, 0x01, &regval, 2);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_read_register failed\n");
		goto error;	
	}
	printf("reg 0x01 val %x\n",regval);

	//sleep(1);
        //I2C 16 bit register read is big endian so specify the endianness flag
	read_len = 2 | 0x80; //max 2 bytes can be read together
	ret = mxuvc_i2c_register_notification_callback(
					handle,
					i2c_notify_cb,
					500, //polling time
					0x00, //ALS value read
					read_len);
	if(ret < 0){
		printf("err: mxuvc_i2c_register_notification_callback failed\n");
		goto error;	
	}
	ret = mxuvc_i2c_notification_add_threshold(
					handle,
					0x10,
					0x20);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_add_threshold failed\n");
		goto error;	
	}
	//Add more thresholds as needed       
#if 0
	ret = mxuvc_i2c_notification_add_threshold(
					handle,
					0x4,
					0x6);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_add_threshold failed\n");
		goto error;	
	}
#endif
	/* Main 'loop' */
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 60;

	while(counter--) {
#if 0 
		if(counter==2){
			ret = mxuvc_i2c_notification_remove_threshold(
				handle,
				0x10,
				0x20);
			if(ret < 0){
				printf("err: mxuvc_i2c_notification_remove_threshold failed\n");
				goto error;	
			}	
            ret = mxuvc_i2c_notification_remove_threshold(
				handle,
				0x4,
				0x6);
            if(ret < 0){
				printf("err: mxuvc_i2c_notification_remove_threshold failed\n");
				goto error;	
			}
		}
#endif

		fflush(stdout);
		sleep(1);
	}
	
	ret = mxuvc_i2c_close(handle);	

	mxuvc_alert_deinit();

	mxuvc_audio_deinit();
	/* Deinitialize and exit */
	mxuvc_video_deinit();
	
	return 0;

error:
	mxuvc_alert_deinit();
#if 0
	mxuvc_audio_deinit();
	mxuvc_video_deinit();
#endif
	mxuvc_custom_control_deinit();

	printf("Failed\n");
	return 1;
}
