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

#define GPIO_MAX_PAD_COUNT 25

static gpio_callback_fn_t gpio_callback_n;
static int gpio_notfctn_enable = 0;

uint32_t count_set_bit(uint32_t x)
{
	uint32_t count = 0;

	while(x){
		count += (x & 1);
		x >>= 1;
	}

	return count;
}

int mxuvc_gpio_notification_start(
		    gpio_callback_fn_t gpio_callback, 
			uint32_t polling_interval_ms, 
			uint32_t gpio_number)
{
    int ret = 0;
	alert_type type;
    gpio_notification_setting gpion;

    CHECK_ERROR(gpio_callback==NULL, -1, "Invalid gpio_callback() function provided");
    CHECK_ERROR(polling_interval_ms<10, -1, "Invalid polling_interval_ms provided");
    CHECK_ERROR(count_set_bit(gpio_number)>GPIO_MAX_PAD_COUNT, -1, "GC6500 has only 25 GPIO lines");
    CHECK_ERROR(gpio_notfctn_enable, -1, "gpio notification is allready enabled");

    type.d32 = 0;
    type.b.gpio_alert = 1;

    gpion.polling_interval_ms = polling_interval_ms;
    gpion.gpio_number = gpio_number;

    ret = set_alert(type, ALARM_ACTION_ENABLE, 0, NULL, 0, 0, 0, &gpion);

    if(ret==0){
        gpio_notfctn_enable = 1;
        gpio_callback_n = gpio_callback;
    }

    return ret;
}

int mxuvc_gpio_notification_stop(void)
{
    int ret = 0;
    alert_type type;
    gpio_notification_setting gpion;
    CHECK_ERROR(gpio_notfctn_enable<=0, -1, "gpio notification is not enabled");
    gpio_notfctn_enable = 0;
    type.d32 = 0;
    type.b.gpio_alert = 1;

    ret = set_alert(type, ALARM_ACTION_DISABLE, 0, NULL, 0, 0, 0, &gpion);   
    
    return ret;
}

void gpio_notification_process(gpio_alert_info *gpion){
    if(gpio_callback_n)
        gpio_callback_n(gpion->gpio_number, gpion->gpio_value);
}

