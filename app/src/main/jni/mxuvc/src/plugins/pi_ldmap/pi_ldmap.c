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
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include "libusb/handle_events.h"
#include "mxuvc.h"
#include "common.h"
#include <sys/stat.h>

#define PRINTF(str...)  

static struct libusb_device_handle *camera = NULL;
static struct libusb_context *ctxt = NULL;

static unsigned int choidx[NUM_MUX_VID_CHANNELS][NUM_OVERLAY_IMAGE_IDX];
static unsigned int chbidx[NUM_MUX_VID_CHANNELS][NUM_OVERLAY_TEXT_IDX];

#define IDX_HANDLE			(0xFF000000)
#define IDX_TEXT_ID         (0x00AA0000)
#define IDX_IMAGE_ID        (0x00BB0000)

#define MAX_TEXT_LENGTH   	(12)
#define EP0TIMEOUT   		(0)
#define FWPACKETSIZE 		4088
#define QCC_BID_PMU         0x21      /* Partition Management Unit */
#define QCC_BID_PMC         0x22      /* Physical Memory Ctl. (SDRAM) */	




static int ldm_qcc_write(uint16_t bid, uint16_t addr, uint16_t length, uint32_t value)
{
	int r;
	int cmd_sta;
	PRINTF("%s (IN)\n",__func__);

	switch (length) {
		case 1:
		value &= 0xFF;
		break;
		case 2:
		value &= 0xFFFF;
		break;
		case 4:
		break;
		default:
		return -1;
	}

	r = libusb_control_transfer(camera,
			/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
			/* bRequest      */ QCC_WRITE,
			/* wValue        */ bid,
			/* MSB 4 bytes   */
			/* wIndex        */ addr,
			/* Data          */ (unsigned char *)&value,
			/* wLength       */ length,
			/* timeout       */ EP0TIMEOUT
		);
	if (r < 0) {
		printf("Failed QCC_WRITE %d\n", r);
		return r;
	}

	r = libusb_control_transfer(camera,
			/* bmRequestType */
		(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
			/* bRequest      */ QCC_WRITE_STATUS,
			/* wValue        */ 0,
			/* wIndex        */ 0,
			/* Data          */ (unsigned char *)&cmd_sta,
			/* wLength       */ sizeof(int),
			/* timeout       */ EP0TIMEOUT
		);

	if (r < 0) {
		printf("Failed QCC_WRITE_STATUS %d\n", r);
		return r;
	}

	PRINTF("%s (OUT)\n",__func__);

	return 0;
}


static int ldm_qcc_read(uint16_t bid, uint16_t addr, uint16_t length, uint32_t *value)
{
	int r;
	int mask;

	PRINTF("%s (IN)\n",__func__);

	switch (length) {
		case 1:
		mask = 0xFF;
		break;
		case 2:
		mask = 0xFFFF;
		break;
		case 4:
		mask = 0xFFFFFFFF;
		break;
		default:
		return -1;
	}

	r = libusb_control_transfer(camera,
			/* bmRequestType */
		(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
			/* bRequest      */ QCC_READ,
			/* wValue        */ bid,
			/* wIndex        */ addr,
			/* Data          */ (unsigned char*)value,
			/* wLength       */ length,
			/* timeout       */ EP0TIMEOUT
		);
	if (r < 0) {
		printf("Failed QCC_READ %d\n", r);
		return r;
	}

	*value &= mask;
	return 0;
}


static int ldm_usb_send_buffer(struct libusb_device_handle *dhandle, 
	unsigned char *buffer, int buffersize, int fwpactsize,unsigned char brequest)
{
	int r, ret, offset = 0;
	int total_size;
	
	total_size = buffersize;

    PRINTF("Sending buffer of size %d\n",total_size);

	while(total_size > 0){
		int readl = 0;
		if(fwpactsize > total_size)
			readl = total_size;
		else
			readl = fwpactsize;

		r = libusb_control_transfer(dhandle,
				/* bmRequestType*/
			LIBUSB_ENDPOINT_OUT |LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE,
				/* bRequest     */brequest,
				/* wValue       */0,
				/* wIndex       */0,
				/* Data         */
			(unsigned char *)&buffer[offset],
				/* wLength       */ readl,
			0); 
		if(r<0){
			printf("ERR: Req 0x%x failed\n",brequest);	
			ret = -1;
			break;
		}
		offset += readl;
		total_size -= readl;
	}

	return ret;
}

int ldm_memw_1k(unsigned int codecAddr, unsigned char *buffer, int buffersize)
{
    int ret;
    uint16_t addr_hi;
    uint16_t addr_lo;
    unsigned char status[64];

    if (buffer == NULL)
        return 0;

    addr_hi = (uint16_t)(codecAddr >> 16);
    addr_lo = (uint16_t)(codecAddr & 0xFFFF);

    ret = libusb_control_transfer(camera,
				  /* bmRequestType */
				  (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				   LIBUSB_RECIPIENT_INTERFACE),
				  /* bRequest      */ MEM_WRITE,
				  /* wValue        */ (uint16_t)addr_hi,
				  /* MSB 4 bytes   */
				  /* wIndex        */ (uint16_t)addr_lo,
				  /* Data          */ (unsigned char *)buffer,
				  /* wLength       */ buffersize,
				  /* timeout       */ 0   
				  );
    if (ret < 0) {
        return 0;
    }

    ret = libusb_control_transfer(camera,
				  /* bmRequestType */
				  (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				   LIBUSB_RECIPIENT_INTERFACE),
				  /* bRequest      */ MEM_WRITE_STATUS,
				  /* wValue        */ 0,
				  /* wIndex        */ 0,
				  /* Data          */ status,
				  /* wLength       */ sizeof(status),
				  /* timeout       */ 0
				  );
    
    if (ret < 0)
        return 0;
    else
        return buffersize;

}

int ldm_memr_1k(unsigned int codecAddr, unsigned char *buffer, int buffersize)
{
    int ret;
    uint16_t addr_hi;
    uint16_t addr_lo;

    if (buffer == NULL)
        return 0;


    addr_hi = (uint16_t)(codecAddr >> 16);
    addr_lo = (uint16_t)(codecAddr & 0xFFFF);

    ret = libusb_control_transfer(camera,
				  /* bmRequestType */
				  (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				   LIBUSB_RECIPIENT_INTERFACE),
				  /* bRequest      */ MEM_READ,
				  /* wValue        */ (uint16_t)addr_hi,
				  /* MSB 4 bytes   */
				  /* wIndex        */ (uint16_t)addr_lo,
				  /* Data          */ (unsigned char *)&buffersize,
				  /* wLength       */ sizeof(int),
				  /* timeout       */ 0   
				  );
    if (ret < 0) {
	printf("qhalhost_read_block error: vendor req MEM_READ. %d\n", ret);
	return ret;
    }

    ret = libusb_control_transfer(camera,
				  /* bmRequestType */
				  (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				   LIBUSB_RECIPIENT_INTERFACE),
				  /* bRequest      */ MEM_READ_STATUS,
				  /* wValue        */ 0,
				  /* wIndex        */ 0,
				  /* Data          */ (unsigned char*)buffer,
				  /* wLength       */ buffersize,
				  /* timeout       */ 0
				  );
    
    if (ret < 0)
        return 0;
    else
        return buffersize;
}

int ldmap_wait_dewarp_done(void)
{
    int ret=0;
    int cmd_sta = 0, retry = 10;

    while(retry--) { 
        ret = libusb_control_transfer(camera,
                                /* bmRequestType */
                                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                                 LIBUSB_RECIPIENT_INTERFACE),
                                /* bRequest      */ DEWARP_STATUS,
                                /* wValue        */ 0,
                                /* wIndex        */ 0,
                                /* Data          */ (unsigned char *)&cmd_sta,
                                /* wLength       */ sizeof(int),
                                /* timeout       */ LIBUSB_CMD_TIMEOUT
                                );

        if(ret < 0)
            break;
        if (cmd_sta == 0) {
            break;
        }
        else
            usleep(10*1000);
    }
    if(ret < 0)
        return ret;
    else
        return cmd_sta;         //Can be 0 or 1
}

int ldm_memw(unsigned int codecAddr, unsigned char *buffer, int buffersize)
{
    int i = 0;
    int ret;
    int len;
    
    while(i < buffersize)
    {
        len = (buffersize - i) > 1024 ? 1024 : (buffersize - i);
        ret = ldm_memw_1k(codecAddr+i, buffer+i, len);
        if(ret != len) {
            break;
        }
        i += len;
    }

    return(i);
}

int ldm_memr(unsigned int codecAddr, unsigned char *buffer, int buffersize)
{
    int i = 0;
    int ret;
    int len;
    
    while(i < buffersize)
    {
        len = (buffersize - i) > 1024 ? 1024 : (buffersize - i);
        ret = ldm_memr_1k(codecAddr+i, buffer+i, len);
        if(ret != len) {
            break;
        }
        i += len;
    }

    return(i);
}

int ldm_init()
{
	int ret=0, i=0;
	struct libusb_device **devs=NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *devhandle = NULL;

	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *conf_desc;
	const struct libusb_interface *dev_interface;
	const struct libusb_interface_descriptor *altsetting;
	int scan_result = 0;

	if (camera == NULL){
		ret = init_libusb(&ctxt);
		if (ret) {
			TRACE("libusb_init failed\n");
			return -1;
		}
		//scan and detect GEO devices
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
		//camera = libusb_open_device_with_vid_pid(ctxt, 0x0b6a, 0x4d52);
		if (camera == NULL) {
			TRACE("ERR: Opening camera failed\n");
			return -1;
		}
		TRACE("ldm init done\n");
	}

	unsigned int data[4] = {0};
	// read csr registers    
	ret = ldm_qcc_read(QCC_BID_PMU, 0x08, 2, &data[0]);
	ret = ldm_qcc_read(QCC_BID_PMC, 0x14, 1, &data[1]);
	ret = ldm_qcc_read(QCC_BID_PMC, 0x15, 1, &data[2]);
	ret = ldm_qcc_read(QCC_BID_PMC, 0x16, 1, &data[3]);

	YUVUtil_Init2(data[0], data[1], data[2], data[3]);

	int k, l, m;
	for(k=0; k < NUM_MUX_VID_CHANNELS; k++)
	{
		for(l=0; l < NUM_OVERLAY_TEXT_IDX; l++)
		{
			chbidx[k][l] = 0;
		}

		for(m=0; m < NUM_OVERLAY_IMAGE_IDX; m++)
		{
			choidx[k][m] = 0;
		}

	}


	return ret;
}

int ldm_deinit(void){
	int ret = 0;

	if(camera){
		libusb_close (camera);
		exit_libusb(&ctxt);

		ctxt = NULL;
		camera = NULL;
	}

	return ret;
}


