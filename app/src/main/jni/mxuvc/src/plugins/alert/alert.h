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

#ifndef __ALERT_H__
#define __ALERT_H__

#include <mxuvc.h>

typedef union {
	uint32_t	d32;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	struct {
    unsigned int gpio_alert: 1;
    unsigned int audio_alert : 1;
	unsigned int i2c_alert: 1;
	unsigned int reserved : 29;
	} b;
#elif __BYTE_ORDER == __BIG_ENDIAN
	struct {
	unsigned int reserved : 29;
	unsigned int i2c_alert: 1;
    unsigned int audio_alert : 1;
    unsigned int gpio_alert: 1;
	} b;
#endif
}alert_type;

typedef enum{
	ALARM_ACTION_ENABLE = 1, //enable alarm
	ALARM_ACTION_DISABLE,      //disable alarm
	ALARM_ACTION_ADD,              //add a new value to the alarm control
	ALARM_ACTION_REMOVE,     //remove a  value to the alarm control
	NO_ACTION, //TBD
}alert_action;

typedef enum{
	I2C_OPEN = 1,
	I2C_REGISTER_ADDR,
	I2C_ADD_THRESHOLD,
	I2C_REMOVE_THRESHOLD,
	I2C_CLOSE
}i2c_notify_setting_type;

typedef struct {	
	int i2c_instance;
	int i2c_device_type;
	uint16_t i2c_device_addr;
	uint16_t chip_id_reg;
	uint16_t chip_id_reg_value;
}__attribute__((__packed__))i2c_open;
typedef struct {	
	uint32_t polling_interval_ms;
	uint32_t read_len;
	uint16_t reg_addr;
}__attribute__((__packed__))i2c_set_reg;
typedef struct {	
	uint16_t low_threshold;	
	uint16_t high_threshold;	
}i2c_set_thr;

typedef union {
	i2c_open 	open;
	i2c_set_reg 	reg;
	i2c_set_thr	thr;
}i2cmsg_t;

typedef struct{
	i2c_notify_setting_type setting_type;
	i2cmsg_t i2cmsg;
}__attribute__((__packed__))i2c_notification_setting;

typedef struct {
    uint32_t polling_interval_ms;
    uint32_t gpio_number;
}gpio_notification_setting;

typedef struct {
    uint32_t gpio_number;
    uint32_t gpio_value;
}gpio_alert_info;

typedef struct{
	int value;
}i2c_alert_info;

typedef struct {
	char len; // dual purpose for status and buf len
	unsigned char buf[2];
} i2c_data_t;
typedef struct {
	unsigned short dev_addr;
	unsigned short sub_addr;
	i2c_data_t     data;
} i2c_payload_t;

//functions
int get_audio_intensity(uint32_t *audioIntensityDB);

int set_alert(alert_type type,
		alert_action	action,
		unsigned int audioThresholdDB,
		i2c_notification_setting *i2cn,
		int check_status,
		uint32_t expected_status_value,
		uint32_t status_wait_time_ms,
        gpio_notification_setting *gpion);

int proces_audio_alert(audio_alert_info *aalert);
int i2c_notification_fn(i2c_alert_info *i2c_data);

int check_alert_state(void);

int i2c_rw(i2c_payload_t *payload, i2c_data_t *data, 
		uint16_t inst, uint16_t type, bool set_req);

void gpio_notification_process(gpio_alert_info *gpion);
#endif
