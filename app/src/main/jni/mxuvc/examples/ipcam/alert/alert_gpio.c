/*******************************************************************************
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Maxim Integrated Products.  It is subject to the terms of a
 * License Agreement between Licensee and Maxim Integrated Products.
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and
 * any derivative work shall retain this copyright notice.
 *
 * Copyright (c) 2012 Maxim Integrated Products.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <pthread.h>
#include <math.h>

#include "mxuvc.h"

#define MAX_GPIO 25

void gpio_cb(uint32_t gpio_num, uint32_t gpio_value)
{
    int i=0;
    //printf("gpio_cb %x %x\n",gpio_num,gpio_value);
    for(i=0; i<MAX_GPIO; i++){
        if((gpio_num >>i)&1)
            printf("\ngpio_cb gpio[%d]->%d\n",i,(gpio_value>>i)&1);
    }
}

int main(int argc, char **argv)
{
   	int ret, counter;
    int number = 0;
    int gpio_num1 = 1;
    int gpio_num2 = 2;
    	
    ret = mxuvc_alert_init();
	if(ret < 0)
		goto error;
    printf("track gpio %d %d\n",gpio_num1,gpio_num2);
    ret = mxuvc_gpio_notification_start(gpio_cb, 60, ((number | (1<<gpio_num1)) | (1<<gpio_num2)));
    	/* Main 'loop' */
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 15;

    while(counter--) {
        printf("\r%i secs left", counter);

        fflush(stdout);
        sleep(1);
    }

    mxuvc_gpio_notification_stop();
    mxuvc_alert_deinit();

    return 0;
error:
    mxuvc_alert_deinit();

    return 1;
}
