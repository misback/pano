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
static struct libusb_device_handle *camera = NULL;
static struct libusb_context *ctxt = NULL;

#define EP0TIMEOUT          (0)
#define FWPACKETSIZE        4088


static int usb_send_buffer(struct libusb_device_handle *dhandle, 
    unsigned char *buffer, int buffersize, int fwpactsize,unsigned char brequest)
{
    int r, ret, offset = 0;
    int total_size;
    
    total_size = buffersize;

    printf("Sending buffer of size %d\n",total_size);

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

static int usb_send_file(struct libusb_device_handle *dhandle, 
    const char *filename, int fwpactsize,unsigned char brequest)
{
    int r, ret;
    int total_size;
    struct stat stfile;
    FILE *fd; 
    char *buffer;

    if(stat(filename,&stfile))
        return -1;
    if(stfile.st_size <= 0){
        printf("ERR: Invalid file provided\n");
        return -1;
    }

#if !defined(_WIN32)
    fd = fopen(filename, "rb");
#else
    ret = fopen_s(&fd,filename, "rb");
#endif
    
    total_size = stfile.st_size;
    buffer = malloc(fwpactsize);

    printf("Sending file of size %d\n",total_size);

    while(total_size > 0){
        int readl = 0;
        if(fwpactsize > total_size)
            readl = total_size;
        else
            readl = fwpactsize;

        ret = (int)fread(buffer, readl, 1, fd);

        r = libusb_control_transfer(dhandle,
                /* bmRequestType*/
            LIBUSB_ENDPOINT_OUT |LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE,
                /* bRequest     */brequest,
                /* wValue       */0,
                /* wIndex       */0,
                /* Data         */
            (unsigned char *)buffer,
                /* wLength       */ readl,
            0); 
        if(r<0){
            printf("ERR: Req 0x%x failed\n",brequest);  
            ret = -1;
            break;
        }
        if(ret == EOF){
            ret = 0;
            break;
        }

        total_size = total_size - readl;
    }

    if(fd)fclose(fd);
    if(buffer)free(buffer);

    return ret;
}

int mxuvc_ispcfg_init()
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
        TRACE("ispcfg init done\n");
    }

    return ret;
}

int mxuvc_ispcfg_load_file(char* file_name)
{
    int ret=0;
    int data[2]; //Dummy data - has to be sent

    if (camera == NULL){
        TRACE("camera handle is not initialised\n");
        return -1;
    }


    ret = libusb_control_transfer(camera,
                /* bmRequestType */
        (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ UPDATE_ISPCFG,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)data,
                /* wLength       */ sizeof(data),
                /* timeout*/     0 
        );
    CHECK_ERROR(ret < 0, -1, "UPDATE_ISPCFG failed");
    
    ret = usb_send_file(camera, file_name, FWPACKETSIZE, SEND_ISPCFG_FILE);
    CHECK_ERROR(ret < 0, -1, "UPDATE_ISPCFG failed");

    ret = libusb_control_transfer(camera,
                /* bmRequestType */
        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ ISPCFG_FILE_DONE,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ NULL,
                /* wLength       */ 0,
                /* timeout*/     0 
        );
    CHECK_ERROR(ret < 0, -1, "ISPCFG_FILE_DONE failed");
    usleep(100000);

    return ret;
}


int mxuvc_ispcfg_deinit(void){
    int ret = 0;

    if(camera){
        libusb_close (camera);
        exit_libusb(&ctxt);

        ctxt = NULL;
        camera = NULL;
    }

    return ret;
}

