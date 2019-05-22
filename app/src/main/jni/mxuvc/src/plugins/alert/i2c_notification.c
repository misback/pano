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
#include <libusb-1.0/libusb.h>
#include "libusb/handle_events.h"
#include "mxuvc.h"
#include "alert.h"
#include "common.h"

static int i2c_handle = 0;
static i2c_callback_fn_t i2c_callback_fn;
int dinstance = 0;
int dtype = 0;
uint16_t dev_addr =0;

int mxuvc_i2c_open(int i2c_instance, 
		   int i2c_device_type, 
		   uint16_t i2c_device_addr, 
		   uint16_t chip_id_reg, 
		   uint16_t chip_id_reg_value)
{
	int ret = 0;
	alert_type type;
	i2c_notification_setting i2cn;
	CHECK_ERROR(check_alert_state() != 1, -1,
			"alert module is not initialized");
	CHECK_ERROR((i2c_instance != 0) && (i2c_instance != 1), -1 , 
					"Unsupported i2c_instance");	
	CHECK_ERROR(i2c_device_type > 3, -1, "Unsupported i2c_device_type");

	type.d32 = 0;
	type.b.i2c_alert = 1;

	i2cn.setting_type = I2C_OPEN;
	i2cn.i2cmsg.open.i2c_instance = i2c_instance;
	i2cn.i2cmsg.open.i2c_device_type = i2c_device_type;
	i2cn.i2cmsg.open.i2c_device_addr = i2c_device_addr;
	i2cn.i2cmsg.open.chip_id_reg = chip_id_reg;
	i2cn.i2cmsg.open.chip_id_reg_value = chip_id_reg_value;

	ret = set_alert(type, ALARM_ACTION_ADD, 0, &i2cn, 1, 1, 2000, NULL);
	CHECK_ERROR(ret != 0, -1, "set_alert failed");

	i2c_handle++;
	
	dinstance = i2c_instance;
	dtype = i2c_device_type;
	dev_addr = i2c_device_addr;

	return i2c_handle;
}

int mxuvc_i2c_register_notification_callback(int handle, 
			i2c_callback_fn_t i2c_callback, 
			uint32_t polling_interval_ms, 
			uint16_t reg_addr, 
			uint32_t read_len)
{
	int ret = 0;
	alert_type type;
	i2c_notification_setting i2cn;

	CHECK_ERROR(check_alert_state() != 1, -1,
			"alert module is not initialized");
	CHECK_ERROR(handle != i2c_handle, -1, "Invalid handle provided");
	CHECK_ERROR((read_len & 0x3f) > 2, -1, "read_len cannot be more than 2 bytes");
	CHECK_ERROR(polling_interval_ms < 500, -1, "minimum polling interval is 500ms");	
	CHECK_ERROR(i2c_callback==NULL, -1, "Invalid callback function provided");

	i2c_callback_fn = i2c_callback;
	type.d32 = 0;
	type.b.i2c_alert = 1;

	i2cn.setting_type = I2C_REGISTER_ADDR;
	i2cn.i2cmsg.reg.polling_interval_ms = polling_interval_ms;
	i2cn.i2cmsg.reg.reg_addr = reg_addr;
	i2cn.i2cmsg.reg.read_len = read_len;

	ret = set_alert(type, ALARM_ACTION_ADD, 0, &i2cn, 0, 0, 0, NULL);
	CHECK_ERROR(ret != 0, -1, "set_alert failed");

	return 0;
}


int mxuvc_i2c_notification_write_register(int handle, uint16_t reg_addr, uint16_t value, uint32_t write_len)
{
	i2c_payload_t payload;
	i2c_data_t    data;
	int ret = 0;

	CHECK_ERROR(check_alert_state() != 1, -1,
		"alert module is not initialized");
	CHECK_ERROR(handle != i2c_handle, -1, "Invalid handle provided");
	CHECK_ERROR(write_len > 2, -1, "write_len cannot be more than 2 bytes");

	payload.dev_addr = dev_addr;
    	payload.sub_addr = reg_addr;
    	payload.data.len = write_len;
	memcpy(&payload.data.buf[0], &value, write_len);

	ret = i2c_rw(&payload, &data, dinstance, dtype, 1);	

	CHECK_ERROR((data.len < 0) || (ret < 0), -1, "i2c_rw failed");

	return 0;
}

int mxuvc_i2c_notification_read_register(int handle, uint16_t reg_addr, uint16_t *value, uint32_t read_len)
{
	i2c_payload_t payload;
	i2c_data_t    data;
	int ret = 0;

	CHECK_ERROR(check_alert_state() != 1, -1,
		"alert module is not initialized");
	CHECK_ERROR(handle != i2c_handle, -1, "Invalid handle provided");
	CHECK_ERROR(read_len > 2, -1, "read_len cannot be more than 2 bytes");
	
	payload.dev_addr = dev_addr;
    	payload.sub_addr = reg_addr;
    	payload.data.len = read_len;

	ret = i2c_rw(&payload, &data, dinstance, dtype, 0);

	if(ret == 0)
		memcpy(value, &data.buf[0], data.len);

    CHECK_ERROR((ret < 0), -1, "i2c_rw failed");

	return ret;
}

int mxuvc_i2c_notification_add_threshold(int handle, uint16_t low_threshold, uint16_t high_threshold)
{
	int ret = 0;
	alert_type type;
	i2c_notification_setting i2cn;

	CHECK_ERROR(check_alert_state() != 1, -1,
			"alert module is not initialized");
	CHECK_ERROR(handle != i2c_handle, -1, "Invalid handle provided");

	type.d32 = 0;
	type.b.i2c_alert = 1;

	i2cn.setting_type = I2C_ADD_THRESHOLD;
	i2cn.i2cmsg.thr.low_threshold = low_threshold;
	i2cn.i2cmsg.thr.high_threshold = high_threshold;

	ret = set_alert(type, ALARM_ACTION_ENABLE, 0, &i2cn, 0, 0, 0, NULL);
	CHECK_ERROR(ret != 0, -1, "set_alert failed");
	
	return 0;
}

int mxuvc_i2c_notification_remove_threshold(int handle, uint16_t low_threshold, uint16_t high_threshold)
{
	int ret = 0;
	alert_type type;
	i2c_notification_setting i2cn;

	CHECK_ERROR(check_alert_state() != 1, -1,
			"alert module is not initialized");
	CHECK_ERROR(handle != i2c_handle, -1, "Invalid handle provided");

	type.d32 = 0;
	type.b.i2c_alert = 1;

	i2cn.setting_type = I2C_REMOVE_THRESHOLD;
	i2cn.i2cmsg.thr.low_threshold = low_threshold;
	i2cn.i2cmsg.thr.high_threshold = high_threshold;

	ret = set_alert(type, ALARM_ACTION_ADD, 0, &i2cn, 0, 0, 0, NULL);
	CHECK_ERROR(ret != 0, -1, "set_alert failed");

	return 0;
}

int mxuvc_i2c_close(int handle)
{
	int ret = 0;
	alert_type type;
	i2c_notification_setting i2cn;

	CHECK_ERROR(check_alert_state() != 1, -1,
			"alert module is not initialized");
	CHECK_ERROR(handle != i2c_handle, -1, "Invalid handle provided");
	
	i2c_callback_fn = NULL;

	type.d32 = 0;
	type.b.i2c_alert = 1;

	i2cn.setting_type = I2C_CLOSE;

	ret = set_alert(type, ALARM_ACTION_DISABLE, 0, &i2cn, 0, 0, 0, NULL);
	CHECK_ERROR(ret != 0, -1, "set_alert failed");
	
	i2c_handle = 0;

	return 0;
}

int i2c_notification_fn(i2c_alert_info *i2c_data)
{
	CHECK_ERROR(check_alert_state() != 1, -1,
		"alert module is not initialized");
	CHECK_ERROR(i2c_handle == 0, -1, "no registerd handle found");

	if(i2c_callback_fn)
		i2c_callback_fn(i2c_data->value);

	return 0;	
}
