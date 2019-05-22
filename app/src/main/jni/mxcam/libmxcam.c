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

/**
 * \mainpage libmxcam API Reference
 * \section intro Introduction
 * libmxcam library  allows you to manage the maxim camera over USB with vendor
 * specific commands.<br>This documentation is aimed at application developers
 * wishing to manage maxim camera over usb.
 * \section features Library features
 * - boot the camera over usb
 * - upgrade the camera firmware
 * - get the camera information
 * - reboot the camera
 * - camera configuration records management
 * \section errorhandling Error handling
 * libmxcam functions typically return  \ref MXCAM_OK on success or a non zero value
 * for error code.<br> a negative value indicates it is a libusb error.<br>
 * The error codes defined in \ref MXCAM_STAT_ERR as enum constants.<br>
 * \ref MXCAM_STAT_ERR codes contains error as well as status codes,
 * which are listed on the \ref misc "miscellaneous" documentation page.
 *<br>\ref mxcam_error_msg "mxcam_error_msg" can be used to convert error
 * and status code to human readable string
 * \section Dependent Dependent library
 *  - libusb 1.0
 */
/** @defgroup library Library initialization/deinitialization */
/** @defgroup boot Boot firmware */
/** @defgroup upgrade Upgrade firmware */
/** @defgroup camerainfo Camera information */
/** @defgroup configuration Camera configuration record management  */
/** @defgroup misc Miscellaneous */
/** \file
 * libmxcam implementation
 */
#ifndef API_EXPORTED

#if !defined(_WIN32)
    #include <sys/types.h>
    #include <libusb-1.0/libusb.h>
    #include <unistd.h>
    #include <endian.h>
    #include <sys/mman.h>
    #include <arpa/inet.h>
    #include "xmodem.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <math.h>
#include "libmxcam.h"
#include "ldmap.h"
#include "cJSON/cJSON.h"

#define FWPACKETSIZE 4088   /*3072  2048 4096 */
#define EP0TIMEOUT   (0)    /* unlimited timeout */
#define MAX_JSON_FILE_SIZE 200*1024
#define MAX_ISPCFG_FILE_SIZE 32*1024
#define MAX_BOOTLOADER_SIZE     64*1024

#define LIBMXDEBUG
#undef LIBMXDEBUG

#if !defined(_WIN32)
    #ifdef LIBMXDEBUG
    #define MXPRINT(args...) printf("[libmxcam] " args)
    #else
    #define MXPRINT(args...)
    #endif
#else
    #ifdef LIBMXDEBUG
    #define MXPRINT(...) { printf("[libmxcam] " __VA_ARGS__); }
    #else
    #define MXPRINT(...)
    #endif
    #define le32toh(x) (x)
#endif

#define DONT_CHK_DEV_ID
#undef DONT_CHK_DEV_ID

static struct libusb_device_handle *devhandle=NULL;
static struct libusb_context *dcontext=NULL;
static struct mxcam_devlist *devlist_cache=NULL;
static struct mxcam_devlist *cur_mxdev=NULL;
static struct libusb_device **devs=NULL;
static int json_file_size = 0;
static int fast_boot_enable = 0;

typedef enum
{
    STREAM_STATE_UNDEFINED,
    STREAM_STATE_RUN,
    STREAM_STATE_STOP
} stream_state_t;

static int mxcam_get_av_stream_status(void);
static void json_deminify(char *json, char *out, int *length);
/*
 * is uvc device or not ?
 */
static int is_uvcdevice(libusb_device_handle *dev)
{
    return cur_mxdev->type == DEVTYPE_UVC;
}

/*
 * is mboot device or not ?
 */
static int is_bootdevice(libusb_device_handle *dev)
{
    return cur_mxdev->type == DEVTYPE_BOOT;
}

static int is_max64180(libusb_device_handle *dev)
{
    return cur_mxdev->soc == MAX64180;
}

static int is_max64380(libusb_device_handle *dev)
{
    return cur_mxdev->soc == MAX64380;
}

static int is_max64480(libusb_device_handle *dev)
{
    return cur_mxdev->soc == MAX64480;
}

static int is_max64580(libusb_device_handle *dev)
{
    return cur_mxdev->soc == MAX64580;
}

static void *grab_file(const char *filename, int blksize, unsigned int *size,
        unsigned int *totblks)
{
    unsigned int max = blksize;
    int ret, err_save;
    FILE *fd;

    char *buffer = malloc(max);
    if (!buffer)
        return NULL;

#if !defined(_WIN32)
    fd = fopen(filename, "rb");
#else
    ret = fopen_s(&fd,filename, "rb");
#endif

    if (fd == NULL)
        return NULL;
    *size = 0;
    *totblks = 1;

    while ( (ret = (int)fread((char*)(buffer + (*size)), sizeof(char),max-(*size),fd)) > 0 ){
        *size += ret;
        if (*size == max) {
            void *p;
            p = realloc(buffer, max *= 2);
            if (!p)
                goto out_error;
            buffer = p;
            memset((buffer + *size), 0, max - *size);
        }
    }
    if (ret < 0)
        goto out_error;

    fclose(fd);

    *totblks = (*size + blksize - 1) / blksize;
    return buffer;

out_error:
    err_save = errno;
    free(buffer);
    fclose(fd);
    errno = err_save;
    *totblks = 0;
    return NULL;
}
/*
 *   all data in network byte order (aka natural aka bigendian)
 */
static unsigned int get_loadaddr(image_header_t *img)
{
    return ntohl(img->ih_load);
}

static int isvalidimage(image_header_t* img)
{
    return (ntohl(img->ih_magic) != IH_MAGIC) ? 1 : 0 ;
}
/*
static unsigned int get_loadaddr_64580(image_header_t *img)
{
    return ntohl(img->ih_load);
}

static int isvalidimage_64580(image_header_t* img)
{
    return (ntohl(img->ih_magic) != IH_MAGIC) ? 1 : 0 ;
}*/

static int usb_send_file(struct libusb_device_handle *dhandle,
        const char *filename, unsigned char brequest,
        int fwpactsize, int fwupgrd,int isbootld, unsigned int *fsz)
{
    int r;
    unsigned int fsize = 0, wblkcount = 0;
    int retryc = 0;
    unsigned char *trabuffer;
    unsigned int count;
    unsigned int imageaddr=0;
    struct stat stfile;
    image_header_t img_hd;
    int total_remain = 0;
    int tx_byte = 0;

    if(stat(filename,&stfile))
        return MXCAM_ERR_FILE_NOT_FOUND;
    trabuffer =
        (unsigned char *)grab_file(filename, fwpactsize, &fsize,
                &wblkcount);
    if (!trabuffer)
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;

    /* Check that the image is bigger than the header size */
    if(fsize < sizeof(image_header_t)) {
        free(trabuffer);
        return MXCAM_ERR_NOT_VALID_IMAGE;
    }
    memcpy(&img_hd, trabuffer, sizeof(image_header_t));

    /* Should not check bootloader header */
    if(!isbootld) {
        if (isvalidimage(&img_hd)) {
            free(trabuffer);
            return MXCAM_ERR_NOT_VALID_IMAGE;
        }
        /* check if image size is proper */
        if((uint32_t)(stfile.st_size) != 
                (uint32_t)(ntohl(img_hd.ih_size) + sizeof(image_header_t))){    
            free(trabuffer);
            return MXCAM_ERR_NOT_VALID_IMAGE;
        }
    }

    imageaddr=get_loadaddr(&img_hd);
    total_remain = fsize;
    MXPRINT("File %s Size %d \n", filename, fsize);
    MXPRINT("Blk count %d\n", wblkcount);
    MXPRINT("Image load addr %0x\n",imageaddr);

    if( fwupgrd == 0 ){
        /*Send image download address in <wValue wIndex> */
        r = libusb_control_transfer(dhandle,
            /* bmRequestType */
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE,
            /* bRequest      */ brequest-1 ,
            /* wValue        */ (uint16_t) (imageaddr >> 16),
            /* wIndex        */ (uint16_t) (imageaddr & 0x0000ffff),
            /* Data          */ NULL,
            /* wLength       */ 0,
                        0);//No time out
        if (r < 0) {
            free(trabuffer);
            return MXCAM_ERR_IMAGE_SEND_FAILED;
        }
    }
    for (count = 0; count < wblkcount; count++) {
        tx_byte = fwpactsize;
        if(total_remain < fwpactsize)
            tx_byte = total_remain;
        total_remain = total_remain - tx_byte;

retry:
        r = libusb_control_transfer(dhandle,
                /* bmRequestType*/
                LIBUSB_ENDPOINT_OUT |LIBUSB_REQUEST_TYPE_VENDOR |
                LIBUSB_RECIPIENT_INTERFACE,
                /* bRequest     */brequest,
                /* wValue       */(uint16_t)(count >> 16),
                /* wIndex       */(uint16_t)(count & 0x0000ffff),
                /* Data         */
                trabuffer + (count * fwpactsize),
                /* wLength       */ tx_byte,
                0); // No timeout
        if (r != tx_byte) {
            MXPRINT("Retry:libusb_control_transfer cnt %d\n",count);
            if (retryc <= 3) {
                retryc++;
                goto retry;
            }
            free(trabuffer);
            return MXCAM_ERR_IMAGE_SEND_FAILED;
        }
    }
    free(trabuffer);
    *fsz = fsize;
    return MXCAM_OK;
}

static int tx_libusb_ctrl_cmd(VEND_CMD_LIST req, uint16_t wValue)
{
    int r;
    char data[4] = {0};
    uint16_t wLength=4;

    r = libusb_control_transfer(devhandle,
                        /* bmRequestType */
            LIBUSB_ENDPOINT_OUT| LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE,
            req,            /* bRequest      */
            wValue,         /* wValue */
            0,          /* wIndex */
            (unsigned char *)data,  /* Data          */
            wLength,
            EP0TIMEOUT);
    if (r < 0) {
        return r;
    }
    return MXCAM_OK;
}
#endif /*API_EXPORTED*/

const char * get_status(const int status)
{
    switch (status) {
        case MXCAM_OK:
            return "No error - operation complete";
        // Staus    
        case MXCAM_STAT_EEPROM_FW_UPGRADE_START:
            return "Started EEPROM FW Upgrade";
        case MXCAM_STAT_EEPROM_FW_UPGRADE_COMPLETE:
            return "Completed  EEPROM FW Upgrade";
        case MXCAM_STAT_SNOR_FW_UPGRADE_START:
            return "Started SNOR FW Upgrade";
        case MXCAM_STAT_SNOR_FW_UPGRADE_COMPLETE:
            return "Completed SNOR FW Upgrade";
        case MXCAM_STAT_FW_UPGRADE_COMPLETE:
            return "Completed FW Upgrade";
        case MXCAM_STAT_EEPROM_ERASE_IN_PROG:
            return "EEPROM Erase in progress";
        case MXCAM_STAT_EEPROM_SAVE_IN_PROG:
            return "EEPROM config save in progress";
        //Errors
        case MXCAM_ERR_FW_IMAGE_CORRUPTED:
            return "FW Image is corrupted";
        case MXCAM_ERR_FW_SNOR_FAILED:
            return "SNOR FW upgrade failed";
        case MXCAM_ERR_FW_UNSUPPORTED_FLASH_MEMORY:
            return "Unsupported Flash memory";
        case MXCAM_ERR_ERASE_SIZE:
            return "Erase size execeds MAX_MXCAM_SIZE"; 
        case MXCAM_ERR_ERASE_UNKNOWN_AREA:
            return "Unknown area to erase";
        case MXCAM_ERR_SAVE_UNKNOWN_AREA:
            return "Unknown area to save";
        case MXCAM_ERR_SETKEY_OVER_FLOW_NO_MEM:
            return "Not enough memory to save new key on memory";
        case MXCAM_ERR_SETKEY_UNKNOWN_AREA:
            return "Unkown area to set key";
        case MXCAM_ERR_REMOVE_KEY_UNKNOWN_AREA:
            return "Unkown area to remove key";
        case MXCAM_ERR_GETVALUE_UNKNOWN_AREA:
            return "Unkown area to get key";
        case MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND:
            return "Key not found";
        case MXCAM_ERR_TCW_FLASH_READ:
            return "Failed to read TCW from camera";
        case MXCAM_ERR_TCW_FLASH_WRITE:
            return "Failed to write TCW to camera";
        case MXCAM_ERR_MEMORY_ALLOC:
            return "Failed to allocate memory on camera";
        //case MXCAM_ERR_MXCAM_AREA_NOT_INIT:
        //  return "Vendor area is not initialized";
        case MXCAM_ERR_VEND_ERR_JSON_SYNTAX_ERR:
            return "Configuration file syntax is wrong";
        case MXCAM_ERR_VEND_ERR_ISPCFG_SYNTAX_ERR:
            return "ISP configuration file syntax is wrong";
        case MXCAM_ERR_SETKEY_UNSUPPORTED:
            return "json is provided while booting so set/remove key is not supported";
        default:
            return "Unknown error";
    }
}

/* API's Implementation */

/**
* \ingroup misc
* \brief mxcam_error_msg:
*   To turn an error or status code into a human readable string
* \param err  : error or status value
*
* \retval  covertedstring
*
* \remark
*    This function can be used to turn an error code into a human
* readable string.
*/
const char* mxcam_error_msg(const int err)
{
    switch (err) {
        case MXCAM_OK:
            return "No error - operation complete";
        // Staus
        case MXCAM_STAT_EEPROM_FW_UPGRADE_START:
            return "Started EEPROM FW Upgrade";
        case MXCAM_STAT_EEPROM_FW_UPGRADE_COMPLETE:
            return "Completed  EEPROM FW Upgrade";
        case MXCAM_STAT_SNOR_FW_UPGRADE_START:
            return "Started SNOR FW Upgrade";
        case MXCAM_STAT_SNOR_FW_UPGRADE_COMPLETE:
            return "Completed SNOR FW Upgrade";
        case MXCAM_STAT_FW_UPGRADE_COMPLETE:
            return "Completed FW Upgrade";
        case MXCAM_STAT_EEPROM_ERASE_IN_PROG:
            return "EEPROM Erase in progress";
        case MXCAM_STAT_EEPROM_SAVE_IN_PROG:
            return "EEPROM config save in progress";
        //Errors
        case MXCAM_ERR_FW_IMAGE_CORRUPTED:
            return "FW Image is corrupted";
        case MXCAM_ERR_FW_SNOR_FAILED:
            return "SNOR FW upgrade failed";
        case MXCAM_ERR_FW_UNSUPPORTED_FLASH_MEMORY:
            return "Unsupported Flash memory";
        case MXCAM_ERR_ERASE_SIZE:
            return "Image size exceeds available Flash size";
        case MXCAM_ERR_ERASE_UNKNOWN_AREA:
            return "Unknown area to erase";
        case MXCAM_ERR_SAVE_UNKNOWN_AREA:
            return "Unknown area to save";
        case MXCAM_ERR_SETKEY_OVER_FLOW_NO_MEM:
            return "Not enough memory to save new key on memory";
        case MXCAM_ERR_SETKEY_UNKNOWN_AREA:
            return "Unknown area to set key";
        case MXCAM_ERR_REMOVE_KEY_UNKNOWN_AREA:
            return "Unknown area to remove key";
        case MXCAM_ERR_GETVALUE_UNKNOWN_AREA:
            return "Unknown area to get key";
        case MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND:
            return "Value not found for given key";
        case MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA:
            return "Unknown area to get config size";
        case MXCAM_ERR_TCW_FLASH_READ:
            return "Failed to read TCW from camera";
        case MXCAM_ERR_TCW_FLASH_WRITE:
            return "Failed to write TCW to camera";
        case MXCAM_ERR_MEMORY_ALLOC:
            return "Failed to allocate memory on camera";
        case MXCAM_ERR_VEND_AREA_NOT_INIT:
            return "Vendor area is not initialized";
        case MXCAM_ERR_INVALID_PARAM:
            return "Invalid parameter(s)";
        case MXCAM_ERR_INVALID_DEVICE:
            return "Not a valid device";
        case MXCAM_ERR_IMAGE_SEND_FAILED:
            return "Failed to send image";
        case MXCAM_ERR_FILE_NOT_FOUND:
            return "File not found";
        case MXCAM_ERR_NOT_ENOUGH_MEMORY:
            return "Not enough memory";
        case MXCAM_ERR_NOT_VALID_IMAGE:
            return "Not a valid image";
        case MXCAM_ERR_VID_PID_ALREADY_REGISTERED:
            return "Already registered vid and Pid";
        case MXCAM_ERR_DEVICE_NOT_FOUND:
            return "Device not found";
        case MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY:
            return "Vendor area not initialized";
        case MXCAM_ERR_FEATURE_NOT_SUPPORTED:
            return "Feature not supported on this device";
        case MXCAM_ERR_I2C_READ:
            return "I2C read failed";
        case MXCAM_ERR_I2C_WRITE:
            return "I2C write failed";
        case MXCAM_ERR_VEND_ERR_JSON_SYNTAX_ERR:
            return "Configuration file syntax is wrong";
        case MXCAM_ERR_VEND_ERR_ISPCFG_SYNTAX_ERR:
            return "ISP configuration file syntax is wrong";
        default:
            return "libusb error";
    }
}

/**
* \ingroup upgrade
* \brief mxcam_upgrade_firmware:
*   upgrade the camera firmware
*
* \param *fw  :pointer to a structure that encapsulate fw upgrade information
* \param *callbk  :register the callbk function pointer here, _callbk_ will be
* invoked before and after the file transfer over usb
* \param is_rom_img  : true if the image is a rom image
*
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - fw is NULL
* - fw->image anf fw->bootldr are NULL
* - fw->img_media >= LAST_MEDIA
* - fw->bootldr_media >= LAST_MEDIA
* - callbk is NULL
* \retval MXCAM_ERR_IMAGE_SEND_FAILED  - if send fails
* \retval MXCAM_ERR_FW_IMAGE_CORRUPTED - found an corrupted image at camera
* \retval MXCAM_ERR_FW_UNSUPPORTED_FLASH_MEMORY - found unsupported flash memory
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   upgrade the camera with the  image information given in fw_info
*structure.<br> using this function you can upgrade  application image
*and/or bootloader  image on given program media provied in fw_info.
*<br> as a status, callbk will invoked before and after the
*file transfer over usb with FW_STATE.
*/

int mxcam_upgrade_firmware(fw_info *fw,
        void (*callbk)(FW_STATE st, const char *filename), int is_rom_img, char **cur_bootmode)
{
    int r;
    image_header_t header;
    FILE *fin;
    char *hdr;
    unsigned int fsize=0;
    char *bootmode = NULL;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !fw || (!fw->image && !fw->bootldr)
          ||fw->img_media >= LAST_MEDIA ||
          fw->bootldr_media >= LAST_MEDIA || !callbk){
        return MXCAM_ERR_INVALID_PARAM;
    }

    if(mxcam_get_av_stream_status() == STREAM_STATE_RUN){
        printf("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;
    r = tx_libusb_ctrl_cmd(FW_UPGRADE_START, FWPACKETSIZE);
    if(r)
        return r;
    /* Change the bootmode to 'usb' to recover if in case upgrade fails */
    r = mxcam_get_value("BOOTMODE", &bootmode);
    /* If bootmode is set and bootmode != usb, set it to usb temporary */
    if (!r && strcmp(bootmode, "usb") != 0) {
        r = mxcam_set_key("BOOTMODE", "usb");
        if(r){
            MXPRINT("Failed setting bootmode key: '%s'\n",mxcam_error_msg(r));
            mxcam_free_get_value_mem(bootmode);
            return r;
        }
    }else{
        mxcam_free_get_value_mem(bootmode);
        bootmode = NULL;
    }

    if(bootmode != NULL){
        *cur_bootmode = bootmode;
    }else
        *cur_bootmode = NULL;

    if (fw->image) {
        callbk(FW_STARTED,fw->image);
        /* Tx START_TRANSFER command */
        r = tx_libusb_ctrl_cmd(START_TRANSFER, \
                    (uint16_t)(fw->img_media));
        if(r)
            return r;
        /* Start sending the image */
        r = usb_send_file(devhandle,fw->image,TX_IMAGE,
                  FWPACKETSIZE,1,0,&fsize);
        if(r)
            return r;

        r = tx_libusb_ctrl_cmd(TRANSFER_COMPLETE, 0);
        if(r)
            return r;
        callbk(FW_COMPLETED,fw->image);

    }
    if (fw->bootldr) {
        callbk(FW_STARTED,fw->bootldr);
        /* Tx START_TRANSFER command */
        r = tx_libusb_ctrl_cmd(START_TRANSFER,
                       (uint16_t)(fw->bootldr_media));
        if(r)
            return r;
        /* Tx mboot header */
        header.ih_magic = ntohl(IH_MAGIC);

#if !defined(_WIN32)
        fin = fopen(fw->bootldr, "r" );
#else
        r = fopen_s(&fin, fw->bootldr, "r" );
#endif
        if (fin == (FILE *)NULL){
            return MXCAM_ERR_FILE_NOT_FOUND;
        }
        fseek(fin, 0L, SEEK_END);
        header.ih_size = ftell(fin);

        header.ih_size  = ntohl( header.ih_size );

        fclose(fin);
        hdr = (char *)&header;
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            LIBUSB_ENDPOINT_OUT| LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE,
            /* bRequest      */ TX_IMAGE,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */
            (unsigned char *)hdr,
            /* wLength     */ (uint16_t)sizeof(image_header_t),
            EP0TIMEOUT);
        if ( r < 0 )
            return r;
        /* Start sending the image */
        r = usb_send_file(devhandle,fw->bootldr,TX_IMAGE,
                      FWPACKETSIZE,1,1,&fsize);
        if (r)
            return r;
        //check if its a full rom image and not just a bootloader image
        if(fsize > MAX_BOOTLOADER_SIZE){
            //for a full rom image BOOTMODE need not to be set at the end
            if(*cur_bootmode != NULL)
                *cur_bootmode = NULL;
            mxcam_free_get_value_mem(bootmode);
        }

        r = tx_libusb_ctrl_cmd(TRANSFER_COMPLETE,0);
        if(r)
            return r;
        callbk(FW_COMPLETED,fw->bootldr);
    }
    r = tx_libusb_ctrl_cmd(FW_UPGRADE_COMPLETE, (uint16_t)(fw->mode));
    if(r)
        return r;
    
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}
/**
* \ingroup boot
* \brief mxcam_boot_firmware:
*   boot the camera firmware over usb
*
* \param *image  : eCos/linux kernel image name
* \param *opt_image  :initrd image in case of linux, NULL for eCos boot
* \param *callbk  :register the callbk function pointer here, _callbk_ will be
* invoked before and after the file transfer over usb
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - image is NULL
* - callbk is NULL
* \retval MXCAM_ERR_IMAGE_SEND_FAILED  - if send fails
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   boot the camera with image and/or opt_image.
*<br> as a status, function callbk will invoked before and after the
*file transfer over usb with FW_STATE.In case of eCos boot,opt_image will be NULL.
*In case of linux, opt_image will be initrd image.
*/

int mxcam_boot_firmware(const char *image, const char *opt_image,
           void (*callbk)(FW_STATE st, const char *filename))
{
    int r;
    unsigned int fsize = 0;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !image || !callbk)
        return MXCAM_ERR_INVALID_PARAM;

    if(!is_bootdevice(devhandle))
        return MXCAM_ERR_INVALID_DEVICE;

    callbk(FW_STARTED,image);
    r = usb_send_file(devhandle,image, 0xde,FWPACKETSIZE,0,0,&fsize);
        if (r)
        return r;
    callbk(FW_COMPLETED,image);

        fsize = 0;
    if (opt_image) {
        callbk(FW_STARTED,opt_image);
        r = usb_send_file(devhandle,opt_image,0xed,
                      FWPACKETSIZE,0,0,&fsize);
        if (r)
            return r;
        callbk(FW_COMPLETED,opt_image);
        }
    if(is_max64180(devhandle)) {
        /* Send a Download complete command with Initrd Image size
         * in <wValue wIndex>.
         * Image size would be zero if no Initrd image*/
            r = libusb_control_transfer(devhandle,
                        /* bmRequestType */
                        (LIBUSB_ENDPOINT_OUT) | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE,
                        /* bRequest      */ 0xad,
                        /* wValue        */ (uint16_t) (fsize >> 16),
                        /* wIndex        */ (uint16_t) (fsize & 0x0000ffff),
                        /* Data          */ NULL,
                        /* wLength       */ 0,
            EP0TIMEOUT);

            if (r < 0)
                return r;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup boot
* \brief mxcam_boot_firmware_fast:
*   boot the camera firmware over usb bulk endpoint
*
* \param *image  : eCos/linux kernel image name
* \param *opt_image  :initrd image in case of linux, NULL for eCos boot
* \param *callbk  :register the callbk function pointer here, _callbk_ will be
* invoked before and after the file transfer over usb
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - image is NULL
* - callbk is NULL
* \retval MXCAM_ERR_IMAGE_SEND_FAILED  - if send fails
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   boot the camera with image and/or opt_image.
*<br> as a status, function callbk will invoked before and after the
*file transfer over usb with FW_STATE.In case of eCos boot,opt_image will be NULL.
*In case of linux, opt_image will be initrd image.
*/

int mxcam_boot_firmware_fast(const char *image, const char *opt_image,
           void (*callbk)(FW_STATE st, const char *filename))
{
#if !defined(WIN32)
    int r;
    unsigned char *trabuffer = NULL;
    struct stat stfile;
    unsigned int *data_buf;
    FILE *fd;
    int transferred;
    
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !image || !callbk)
        return MXCAM_ERR_INVALID_PARAM;

    if(!fast_boot_enable)
        if(!is_bootdevice(devhandle))
            return MXCAM_ERR_INVALID_DEVICE;

    callbk(FW_STARTED,image);
    if(stat(image,&stfile)){
        return MXCAM_ERR_FILE_NOT_FOUND;    
         callbk(FW_COMPLETED,image);
    }
    //allocate memory for fw & 8 bytes(4:fw-size,4:json-size)
    trabuffer = (unsigned char *)malloc(stfile.st_size+8);
    if (!trabuffer){
        callbk(FW_COMPLETED,image);
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;
    }
    data_buf = (unsigned int *)trabuffer;
    data_buf[0] = stfile.st_size;
    data_buf[1] = json_file_size;

#if !defined(_WIN32)
    fd = fopen(image, "rb");
#else
    ret = fopen_s(image, "rb");
#endif
    if (fd == NULL){
        free(trabuffer);
        callbk(FW_COMPLETED,image);
        return MXCAM_ERR_NOT_VALID_IMAGE;
    }
    r = fread(trabuffer+8, sizeof(char), stfile.st_size, fd);
    if(r!=stfile.st_size){
        printf("ERR: fread err %d \n",r);
        free(trabuffer);
        callbk(FW_COMPLETED,image);
        fclose(fd);
        return MXCAM_ERR_NOT_VALID_IMAGE;
    }
    fclose(fd);

    r = libusb_bulk_transfer(devhandle,
                               0x01, //Bulk OUT EP
                               (unsigned char *)trabuffer,
                               stfile.st_size+8,
                               &transferred,
                               20000);
    if(r) {
        printf("fw tx error r = %d transferred %d\n",r,transferred);
        free(trabuffer);
        callbk(FW_COMPLETED,image);
        return MXCAM_ERR_IMAGE_SEND_FAILED;
    }
     
    callbk(FW_COMPLETED,image);
    free(trabuffer);
    
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
#else
	MXPRINT("Not supported on Windows\n");
	return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
#endif
}

//send partial image just to init the ddr
int mxcam_init_ddr(const char *image)
{
    int r;
    unsigned int fsize = 0, wblkcount = 0;
    int retryc = 0;
    unsigned char *trabuffer;
    unsigned int count;
    unsigned int imageaddr=0;
    struct stat stfile;
    image_header_t img_hd;
    int fwupgrd = 0;
    int fwpactsize = FWPACKETSIZE;
    unsigned char brequest = 0xde;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !image )
        return MXCAM_ERR_INVALID_PARAM;

    if(!is_bootdevice(devhandle))
        return MXCAM_ERR_INVALID_DEVICE;

    /**********************/
    if(stat(image,&stfile))
        return MXCAM_ERR_FILE_NOT_FOUND;
    trabuffer = (unsigned char *)grab_file(image, fwpactsize, &fsize, &wblkcount);

    if (!trabuffer)
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;

    /* Check that the image is bigger than the header size */
    if(fsize < sizeof(image_header_t)) {
        free(trabuffer);
        return MXCAM_ERR_NOT_VALID_IMAGE;
    }
    memcpy(&img_hd, trabuffer, sizeof(image_header_t));

    imageaddr=get_loadaddr(&img_hd);

    MXPRINT("File %s Size %d \n", image, fsize);
    MXPRINT("Blk count %d\n", wblkcount);
    MXPRINT("Image load addr %0x\n",imageaddr);

    if( fwupgrd == 0 ){
        /*Send image download address in <wValue wIndex> */
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE,
            /* bRequest      */ brequest - 1,
            /* wValue        */ (uint16_t) (imageaddr >> 16),
            /* wIndex        */ (uint16_t) (imageaddr & 0x0000ffff),
            /* Data          */ NULL,
            /* wLength       */ 0,
                        0);//No time out
        if (r < 0) {
            free(trabuffer);
            return MXCAM_ERR_IMAGE_SEND_FAILED;
        }
    }
    //send only 1/4 th size of the total image
    //thats sufficient to init the ddr
    for (count = 0; count < wblkcount/4; count++) {
retry:
        r = libusb_control_transfer(devhandle,
                /* bmRequestType*/
                LIBUSB_ENDPOINT_OUT |LIBUSB_REQUEST_TYPE_VENDOR |
                LIBUSB_RECIPIENT_INTERFACE,
                /* bRequest     */brequest,
                /* wValue       */(uint16_t)(count >> 16),
                /* wIndex       */(uint16_t)(count & 0x0000ffff),
                /* Data         */
                trabuffer + (count * fwpactsize),
                /* wLength       */ fwpactsize,
                0); // No timeout
        if (r != fwpactsize) {
            MXPRINT("Retry:libusb_control_transfer cnt %d\n",count);
            if (retryc <= 3) {
                retryc++;
                goto retry;
            }
            free(trabuffer);
            return MXCAM_ERR_IMAGE_SEND_FAILED;
        }
    }
    free(trabuffer);

    /*********************/
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup camerainfo
* \brief mxcam_read_eeprom_config_mem:
*   read the config memory area stored on camera persistent storage memory
*
* \param area :area to read config data
* \param *buf :config data will be written on buf on success
* \param len  :length of config data needs to copied from camera memory
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - buf is NULL
* - len is 0
* - area >= LAST_INFO
* \retval MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY - if the vendor area not
* initialzed properly on the camera.
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   read camera configuration record from camera persistent storage memory
*/
int mxcam_read_eeprom_config_mem(char *buf, unsigned int len)
{
    int r;
    int readl = 0, total_len = 0, length = 0;
    char *json_out = NULL;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !buf || len == 0)
        return MXCAM_ERR_INVALID_PARAM;

    while(total_len < (int)len){
        if(FWPACKETSIZE > (len-total_len))
            readl =  len - total_len;
        else
            readl = FWPACKETSIZE;

        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ GET_EEPROM_CONFIG,
                /* wValue        */ total_len,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char*)&buf[total_len],
                /* wLength       */ readl,
                /* imeout*/          EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed GET_EEPROM_CONFIG %d\n", r);
            if ( r == LIBUSB_ERROR_PIPE ){
                return MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY;
            }
            return r;
        }
        total_len = total_len + readl;
    }
    buf[total_len] = 0;

    json_out = (char *)malloc(3*total_len);
    if(json_out == NULL)
        return MXCAM_ERR_MEMORY_ALLOC;
    
    //de Minify
    json_deminify(&buf[0], json_out, &length);  
    
    memcpy(&buf[0], json_out, length);
    free(json_out);
    MXPRINT("%s (OUT)\n",__func__);
    return 0;
}

int mxcam_get_json_size(unsigned int *len)
{
    int r;
    unsigned int length = 0;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL)
        return MXCAM_ERR_INVALID_PARAM;
    
    r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ GET_JSON_SIZE,
                        /* wValue        */ 0,
                        /* MSB 4 bytes   */
                        /* wIndex        */ 0,
                        /* Data          */ (unsigned char *)&length,
                        /* wLength       */ sizeof(unsigned int),
                        /* imeout*/          EP0TIMEOUT
                        );
    if (r < 0) {
        MXPRINT("Failed GET_JSON_SIZE %d\n", r);
        if ( r == LIBUSB_ERROR_PIPE ){
            return MXCAM_ERR_FILE_NOT_FOUND;
        }
        return r;
    }

    *len = le32toh(length);

    return 0;
}

/**
* \ingroup camerainfo
* \brief mxcam_get_config_size:
*   get camera configuration record size for the config area
* \param area      :area to get config record size
* \param *size_out :returns identified size from the camera

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - *size_out is NULL
* \retval MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA - if area >= LAST_INFO
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   get camera configuration record  size from camera
* persistent storage memory for a specific \ref CONFIG_AREA
*/
int mxcam_get_config_size(CONFIG_AREA area,unsigned short *size_out)
{
    int r;
    MXPRINT("%s (IN)\n",__func__);
    if( area >= LAST_INFO )
        return MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA;

    if(devhandle == NULL || !size_out)
        return MXCAM_ERR_INVALID_PARAM;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_EEPROM_CONFIG_SIZE,
            /* wValue        */ (uint16_t)area,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char*)size_out,
            /* wLength       */ sizeof(unsigned short),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed GET_EEPROM_CONFIG_SIZE %d\n", r);
        return r;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}
#if 0
#ifndef API_EXPORTED
static int isempty_config(char *cfg_mem,int size)
{
    int count=0;
    /*
    * check if config is empty or not ?
    */
    for (; count < size ; count++,cfg_mem++ ){
        if( *cfg_mem == '\0' ){
            return 0;
        }
    }
    return 1;
}
#endif
#endif
/**
* \ingroup configuration
* \brief mxcam_get_all_key_values:
*   get all configuration records from camera for a sepecific \ref CONFIG_AREA
* \param area     :area to get all configuration records
* \param *callback  :register the callbk function pointer here, _callbk_ will be
* invoked when \ref mxcam_get_all_key_values
* gets a valid record,end of config area or found an empty config area
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* \retval MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA - if area >= LAST_INFO
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   get all configuration records from camera for
* a specific \ref CONFIG_AREA .The function _callbk_ will be invoked when
*\ref mxcam_get_all_key_values  gets a valid record,end of config area or
*found an empty config area
*/

int mxcam_get_all_key_values(void)
{
    int r, len=0, total_len = 0, readl, remaining_len=0;
//#if defined(_WIN32)
//    char *NextToken;
//#endif
    char *buf;
    MXPRINT("%s (IN)\n",__func__);
    //r = mxcam_get_config_size(area,&size);
    //if (r)
    //  return r;
    r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ GET_JSON_SIZE,
                        /* wValue        */ 0,
                        /* MSB 4 bytes   */
                        /* wIndex        */ 1, //system obj
                        /* Data          */ (unsigned char *)&len,
                        /* wLength       */ sizeof(unsigned int),
                        /* imeout*/          EP0TIMEOUT
                        );
    if (r < 0) {
        MXPRINT("Failed GET_JSON_SIZE %d\n", r);
        if ( r == LIBUSB_ERROR_PIPE ){
            return MXCAM_ERR_FILE_NOT_FOUND;
        }
        return r;
    }
    len = le32toh(len);
    buf = (char *)malloc(len);
    while(total_len < (int)len){
        remaining_len = (len-total_len);
        if(FWPACKETSIZE > remaining_len )
            readl = remaining_len;
        else
            readl = FWPACKETSIZE;

        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ GET_EEPROM_CONFIG,
                /* wValue        */ total_len,
                /* MSB 4 bytes   */
                /* wIndex        */ 1,
                /* Data          */ (unsigned char*)&buf[total_len],
                /* wLength       */ readl,
                /* imeout*/          EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed GET_EEPROM_CONFIG %d\n", r);
            free(buf);
            if ( r == LIBUSB_ERROR_PIPE ){
                return MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY;
            }
            return r;
        }
        total_len = total_len + readl;
    }
    printf("%s\n",buf);
    free(buf);
    MXPRINT("%s (OUT)\n",__func__);

    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_get_ccr_size:
*   get the size of the complete ccr list
*
* \param area     :area to get ccr list size.
* \param size_out :returns identified size from the camera
*
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - size_out is not a valid pointer
* \retval MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA - if area >= LAST_INFO
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   this will read just the size of ccr list
* from camera and return it in size_out.
*/
int mxcam_get_ccr_size(CONFIG_AREA area,unsigned short *size_out)
{
    int r;
    MXPRINT("%s (IN)\n",__func__);
    if( area >= LAST_INFO )
        return MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA;

    if(devhandle == NULL || !size_out)
        return MXCAM_ERR_INVALID_PARAM;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_CCR_SIZE,
            /* wValue        */ (uint16_t)area,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char*)size_out,
            /* wLength       */ sizeof(unsigned short),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed GET_EEPROM_CONFIG_SIZE %d\n", r);
        return r;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;

}

/**
* \ingroup configuration
* \brief mxcam_read_ccr_mem:
*   get the complete ccr list from camera
* \param area     :area to get ccr list size.
* \param buf      :buffer to read the ccr list
* \param len      :size of the data to be read from camera
*
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - buf is NULL
* - len is zero
* - area >= LAST_INFO
* \retval MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY- if libusb returns LIBUSB_ERROR_PIPE
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   this will read the specified length(parameter len) size of data from camera
* and returns' it in parameter buf.
*/
int mxcam_read_ccr_mem(CONFIG_AREA area,char *buf,unsigned short len)
{
    int r;
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !buf || len == 0 || area >= LAST_INFO)
        return MXCAM_ERR_INVALID_PARAM;

    r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ GET_CCR_LIST,
                        /* wValue        */ (uint16_t)area,
                        /* MSB 4 bytes   */
                        /* wIndex        */ 0,
                        /* Data          */ (unsigned char*)buf,
                        /* wLength       */ len,
                        /* imeout*/          EP0TIMEOUT
                        );
    if (r < 0) {
        MXPRINT("Failed GET_CCR_LIST%d\n", r);
        if ( r == LIBUSB_ERROR_PIPE ){
            return MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY;
        }
        return r;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return 0;
}

/**
* \ingroup configuration
* \brief mxcam_get_all_ccr:
*   get the complete ccr from camera
* \param *callbk  :register the callbk function pointer here, _callbk_ will be
* invoked before and after the file transfer over usb
*
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - buf is NULL
* - len is zero
* - area >= LAST_INFO
* \retval MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY- if libusb returns LIBUSB_ERROR_PIPE
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   this gets the size of the complete ccr using \ref mxcam_get_ccr_size
* and reads the coplete ccr using \ref mxcam_read_ccr_mem and finally for every ccr
* record it invokes the _callbk_ function
*/
int mxcam_get_all_ccr(CONFIG_AREA area, void (*callbk)(GET_ALL_KEY_STATE st, int keycnt, void *data))
{
    int r;
    unsigned short size;
    char *cfg_mem,*string,*key;
#if defined(_WIN32)
    char *NextToken;
#endif
    int count=0;
    MXPRINT("%s (IN)\n",__func__);
    r = mxcam_get_ccr_size(area ,&size);
    if (r)
        return r;

    if (size == 0){
        callbk(GET_ALL_KEY_NO_KEY,0,NULL);
        return MXCAM_OK;
    }
    cfg_mem = (char *)malloc(size);
    r = mxcam_read_ccr_mem(area, cfg_mem, size);
    if (r)
        return r;
    string = cfg_mem;

    while ( size != 0 ) {
#if !defined(_WIN32)
        key=strtok(string,"\n");
#else
        key=strtok_s(string,"\n",&NextToken);
#endif
        if(callbk)
            callbk(GET_ALL_KEY_VALID,0,key);
        string += strlen(key) + strlen("\n");
        size = size - (int)(strlen(key)+strlen("\n"));
        ++count;
    }
    if(callbk){
        callbk(GET_ALL_KEY_COMPLETED, count, NULL);
    }
    free(cfg_mem);
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup camerainfo
* \brief mxcam_read_flash_image_header:
*   get 64 bytes flash image header
* \param *header  :pointer to image_header_t structure
* \IMG_HDR_TYPE hdr_type : possible values of this filed are  
* \            For running fw image header : 0,
* \            if fw image hdr need to be read from snor : 1, for bootloader header: 2
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - header is NULL
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   get 64 bytes flash image header \ref image_header_t. This image header
*information used to get camera information. ih_name in image_header_t is used
* stored camera firmware version information
*/
int mxcam_read_flash_image_header(image_header_t *header, IMG_HDR_TYPE hdr_type)
{
    int r;
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !header)
        return MXCAM_ERR_INVALID_PARAM;

    if (hdr_type == SNOR_FW_HEADER)
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_SNOR_IMG_HEADER,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *) header,
            /* wLength       */ (uint16_t) sizeof(image_header_t),
            /* timeout*/   EP0TIMEOUT
            );
    else if (hdr_type == RUNNING_FW_HEADER)
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_IMG_HEADER,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *) header,
            /* wLength       */ (uint16_t) sizeof(image_header_t),
            /* timeout*/   EP0TIMEOUT
            );
    else if (hdr_type == BOOTLOADER_HEADER)
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_BOOTLOADER_HEADER,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *) header,
            /* wLength       */ (uint16_t) sizeof(image_header_t),
            /* timeout*/   EP0TIMEOUT
            );          
    else 
        return MXCAM_ERR_INVALID_PARAM;

    if (r < 0) {
        MXPRINT("Failed GET_SNOR_IMG_HEADER: %d\n", r);
        return r;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup upgrade
* \brief mxcam_read_nvm_pgm_status:
*   get non volatile memory progamming status
* \param *status  :unsigned char pointer to retrieve programming status information
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - status is NULL
* \retval Negativevalue - upon libusb error
* \retval MXCAM_ERR_FW_SNOR_FAILED - when firmware image upgrade fails
* \retval MXCAM_ERR_FAIL - when an unknown error returns from skypecam
* \retval MXCAM_ERR_FW_IMAGE_CORRUPTED - if the image sent to skypecam is corrupted
* \retval MXCAM_OK  - upon success
*
* \remark
 *  get non volatile memory programming status see \ref MXCAM_STAT_ERR
*/
int mxcam_read_nvm_pgm_status(unsigned char *status)
{
    int r;
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !status )
        return MXCAM_ERR_INVALID_PARAM;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_NVM_PGM_STATUS,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ status,
            /* wLength       */ sizeof(unsigned char),
            /*  timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed GET_NVM_PGM_STATUS  %d\n", r);
        return r;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return *status;
}

/**
* \ingroup configuration
* \brief mxcam_erase_eeprom_config:
*   erase a specific \ref CONFIG_AREA on camera persistent storage memory
* \param area     :area is erase the config area
* \param size     :size of vendor size area for maxim area size is fixed as 1K

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - area >= LAST_INFO
* - area == VENDOR_INFO && size <= 12
* \retval MXCAM_ERR_ERASE_SIZE -if  erase size exceeds MAX_VEND_SIZE
* \retval MXCAM_ERR_ERASE_UNKNOWN_AREA - unknown erase \ref CONFIG_AREA
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   erase a specific \ref CONFIG_AREA on camera persistent storage memory.
* see also \ref mxcam_get_config_size
*/
int mxcam_erase_eeprom_config(CONFIG_AREA area,unsigned short size)
{
#if 0
    int r;

    unsigned char status[64];
    unsigned char cmd_sta;
    unsigned char data[] = "ERASE";

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || area >= LAST_INFO ||
           (area == VENDOR_INFO && size <= 12 ))
        return MXCAM_ERR_INVALID_PARAM;

    if(is_bootdevice(devhandle))
                return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ ERASE_EEPROM_CONFIG,
            /* wValue        */ (uint16_t)area,
            /* wIndex        */ size,
            /* Data          */ (unsigned char*) &data,
            /* wLength       */ (uint16_t)strlen("ERASE"),
            /*  timeout*/    EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed ERASE_EEPROM_CONFIG  %d\n", r);
        return r;
    }

    while (1){
        sleep(1);
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ GET_NVM_PGM_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ status,
                /* wLength       */ sizeof(status),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed GET_PROGRAM_STATUS  %d\n", r);
            return r;
        }
        cmd_sta = *(unsigned char *)status;
        if (cmd_sta >= MXCAM_ERR_FAIL){
            return cmd_sta;
        }
        if (cmd_sta == 0){
            break;
        }
    }
    MXPRINT("%s (OUT)\n",__func__);
#endif
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_save_eeprom_config:
*   save a specific \ref CONFIG_AREA on camera persistent storage memory
* \param area     :area is erase the config area
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - area >= LAST_INFO
* \retval MXCAM_ERR_SAVE_UNKNOWN_AREA - unknown save \ref CONFIG_AREA
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   The KEY=VALUE pairs are manipulated in camera's volatile memory,use this
* API to stored manipulated KEY=VALUE pairs on camera's non volatile memory.
*/
int mxcam_save_eeprom_config(const char *jsonfile, uint32_t json_index)
{
    int r = MXCAM_OK, total_size, json_size;
    unsigned int status;
    unsigned char *buffer = NULL;
    FILE *fd;
    struct stat stfile;
    int mx64580_count = 0;
    unsigned char *mbuf = NULL;

    MXPRINT("%s (IN)\n",__func__);

    if(mxcam_get_av_stream_status() == STREAM_STATE_RUN){
        printf("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
#if !defined(_WIN32)
    fd = fopen(jsonfile, "rb");
#else
    r = fopen_s(&fd,jsonfile, "rb");
#endif
    if(stat(jsonfile,&stfile))
        return -1;

    total_size = stfile.st_size;
    if(total_size >= MAX_JSON_FILE_SIZE){ //max supported size is MAX_JSON_FILE_SIZE
        printf("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        goto out_error;
    }

    buffer = (unsigned char *)malloc(total_size+1);
    if(buffer == NULL){
        r = -1;
        goto out_error;
    }
    mbuf = buffer;

    //read the whole file in buffer and Minify
    fread(buffer, total_size, 1, fd);
    buffer[total_size] = 0;
    cJSON_Minify((char *)buffer);
    json_size = total_size = (int)strlen((const char *)buffer);
    json_size += 1;
    total_size += 1;
    printf("Sending Minified Configuration file %s of size %d Bytes ...\n",jsonfile,total_size);

    while(total_size > 0){
        int readl = 0;
        
        if(FWPACKETSIZE > total_size)
            readl = total_size;
        else
            readl = FWPACKETSIZE;
        
        //r = (int)fread(buffer, readl, 1, fd);

        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ SEND_JSON_FILE,
            /* wValue        */ json_size,
            /* MSB 4 bytes   */
            /* wIndex        */ json_index,
            /* Data          */ buffer,
            /* wLength       */ readl,
            /* timeout*/   0 
            );
        if (r < 0) {
            printf("Failed SEND_JSON %d\n", r);
            return r;
        }
        total_size = total_size - readl;
        buffer = buffer + readl;
    }

    while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        usleep(1000*500); //typical block erase type will be more then 500ms    
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SET_OR_REMOVE_KEY_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&status,
                /* wLength       */ sizeof(unsigned int),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            if(fd)fclose(fd);
            if(mbuf)free(mbuf);
            return r;
        }
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;

        mx64580_count++;
    }
    if(status != MXCAM_OK){
        printf("ERROR: %s\n",get_status(status));   
    }
out_error:
    if(fd)fclose(fd);
    if(mbuf)free(mbuf);

    MXPRINT("%s (OUT)\n",__func__);
    return r;
}

/**
* \ingroup configuration
* \brief mxcam_set_key:
*   Add/modify a KEY=VALUE pair on camera's volatile memory
* \param area     :area is erase the config area
* \param *keyname :KEY
* \param *value   :VALUE

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - area >= LAST_INFO
* - keyname is NULL
* - value is NULL
* \retval MXCAM_ERR_SETKEY_OVER_FLOW_NO_MEM - Not enough memory to save new
* key on memory
* \retval MXCAM_ERR_SETKEY_UNKNOWN_AREA -  Unkown area to set key
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   Add/modify a KEY=VALUE pair on camera's volatile memory
*/
int mxcam_set_key(const char* keyname, const char* value)
{
    int r,size;
    unsigned int status;
    char *packet;
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !keyname || !value)
        return MXCAM_ERR_INVALID_PARAM;

    if(is_bootdevice(devhandle))
                return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(mxcam_get_av_stream_status() == STREAM_STATE_RUN){
        printf("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
    // form the packet
    size = (int)strlen(keyname) + (int)strlen(value) + 2;
    packet = malloc(size);

#if !defined(_WIN32)
    strcpy(packet,keyname);
    strcpy(&packet[strlen(keyname) + 1],value);
#else
    strcpy_s(packet,size, keyname);
    size -= (int)(strlen(keyname) + 1);
    strcpy_s(&packet[strlen(keyname) + 1], size, value);
#endif

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ SET_EEPROM_KEY_VALUE    ,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)packet,
            /* wLength       */ (int)strlen(keyname) + (int)strlen(value) + 2,
            /* timeout*/   EP0TIMEOUT
            );
    free(packet);
    if (r < 0) {
        MXPRINT("Failed SET_EEPROM_KEY_VALUE %d\n", r);
        return r;
    }

    while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        usleep(1000*500); //typical block erase type will be more then 500ms    
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SET_OR_REMOVE_KEY_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&status,
                /* wLength       */ sizeof(unsigned int),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            return r;
        }
        status = le32toh(status);
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;
        mx64580_count++;
    }

    if(status != MXCAM_OK){
        printf("Status: %s\n",get_status(status));  
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_remove_key:
*   remove a KEY=VALUE pair on camera's volatile memory
* \param area     :area is erase the config area
* \param *keyname :KEY

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - area >= LAST_INFO
* - keyname is NULL
* \retval MXCAM_ERR_REMOVE_KEY_UNKNOWN_AREA -  Unknown area to remove key
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   remove a KEY=VALUE pair on camera's volatile memory
*/
int mxcam_remove_key(const char* keyname)
{
    int r;
    unsigned int status;
    char *data;
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !keyname)
        return MXCAM_ERR_INVALID_PARAM;

    if(mxcam_get_av_stream_status() == STREAM_STATE_RUN){
        printf("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
    data = malloc(strlen(keyname)+1);
    if (data == NULL)
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;

#if !defined(_WIN32)
    strcpy(data, keyname);
#else
    strcpy_s(data,(strlen(keyname)+1), keyname);
#endif

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ REMOVE_EEPROM_KEY,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char*) data,
            /* wLength       */ (int)strlen(keyname) + 1,
            /*  timeout*/   EP0TIMEOUT
            );
    free(data);
    if (r < 0) {
        MXPRINT("Failed REMOVE_EEPROM_KEY %d\n", r);
        return r;
    }

    while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        usleep(1000*500); //typical block erase type will be more then 500ms    
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SET_OR_REMOVE_KEY_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&status,
                /* wLength       */ sizeof(unsigned int),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            return r;
        }
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;
        mx64580_count++;
    }

    if(status != MXCAM_OK){
        printf("ERROR: %s\n",get_status(status));   
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_get_value:
*   get VALUE for given KEY from camera's volatile memory
* \param area        :area is erase the config area
* \param *keyname    :KEY
* \param **value_out :On success VALUE would be copied in *value_out

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - area >= LAST_INFO
* - keyname is NULL
* - value_out is NULL
* \retval MXCAM_ERR_GETVALUE_UNKNOWN_AREA -  Unknown area to get value
* \retval MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND -  Value not found for given KEY
* \retval MXCAM_ERR_NOT_ENOUGH_MEMORY - malloc failed
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   get VALUE for given KEY from camera's volatile memory
* see also \ref mxcam_free_get_value_mem
*/
int mxcam_get_value(const char* keyname, char** value_out)
{
    int r;
    unsigned char cmd_sta;
    unsigned char *value;
    char *data;
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !keyname || !value_out)
        return MXCAM_ERR_INVALID_PARAM;

    value = malloc(FWPACKETSIZE);
    data = malloc(strlen(keyname)+1);
    if (value == NULL || data == NULL) {
        free( value ) ;
        free( data  ) ;
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;
    }

#if !defined(_WIN32)
    strcpy(data, keyname);
#else
    strcpy_s(data, (strlen(keyname)+1), keyname);
#endif

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ REQ_GET_EEPROM_VALUE,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ (int)strlen(keyname) + 1,
            /* timeout*/   EP0TIMEOUT
            );
    free(data);
    data    = NULL ;
    if (r < 0) {
        free( value ) ;
        MXPRINT("Failed REQ_GET_EEPROM_VALUE %d\n", r);
        return r;
    }

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_EEPROM_VALUE,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ value,
            /* wLength       */ 1024, //FWPACKETSIZE TBD,
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        free( value ) ;
        MXPRINT("Failed GET_EEPROM_VALUE %d\n", r);
        return r;
    }
    cmd_sta = *(unsigned char *)value;
    if (cmd_sta == MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND){
        free(value);
        *value_out = NULL;
        return MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND;
    }
    // skip the status byte;
    value += 1;
    *value_out =(char *) value;
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_get_ccrvalue:
*   get VALUE for given CCR KEY from camera's CCR record
* \param *keyname    :KEY
* \param **value_out :On success VALUE would be copied in *value_out

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - keyname is NULL
* - value_out is NULL
* \retval MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND -  Value not found for given KEY
* \retval MXCAM_ERR_NOT_ENOUGH_MEMORY - malloc failed
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   get VALUE for the provided CCR KEY from camera's CCR Record
*/
int mxcam_get_ccrvalue(const char* keyname, char** value_out)
{
    int r;
    unsigned char cmd_sta;
    unsigned char *value;
    char *data;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !keyname || !value_out)
        return MXCAM_ERR_INVALID_PARAM;

    value = malloc(FWPACKETSIZE);
    data = malloc(strlen(keyname)+1);

    if (value == NULL || data == NULL)
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;

#if !defined(_WIN32)
    strcpy(data, keyname);
#else
    strcpy_s(data,(strlen(keyname)+1), keyname);
#endif


    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ REQ_GET_CCRKEY_VALUE,
            /* wValue        */ (uint16_t)MAXIM_INFO,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ (int)strlen(keyname) + 1,
            /* timeout*/   EP0TIMEOUT
            );
    free(data);
    if (r < 0) {
        MXPRINT("Failed REQ_GET_EEPROM_VALUE %d\n", r);
        return r;
    }

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_CCRKEY_VALUE,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ value,
            /* wLength       */ FWPACKETSIZE * sizeof(uint8_t),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed GET_CCRKEY_VALUE %d\n", r);
        return r;
    }
    cmd_sta = *(unsigned char *)value;
    if (cmd_sta == MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND){
        free(value);
        *value_out = NULL;
        return MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND;
    }
    // skip the status byte;
    value += 1;
    *value_out =(char *) value;
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_set_configutaion:
*       set VALUE for configurtion like bulk or isoc to the config descritor
* \param config      :config value to be set in config descriptor
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device no booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_set_configuration(EP_CONFIG config)
{
    int r = 0,i;
    int num = 0;
    struct libusb_config_descriptor *handle = NULL;
    EP_CONFIG conf=0;

    MXPRINT("%s (IN)\n",__func__);

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_get_configuration(devhandle, (int*)&conf);
    if(r < 0) {
        MXPRINT("Failed GET_CONFIGURATION  %d\n", r);
        return r;
    }

    if ( conf == config ){
        MXPRINT("present configuration is %d\n",conf);
        return 0;
    }


    if(devhandle == NULL )
        return MXCAM_ERR_INVALID_PARAM;

    r =  libusb_get_active_config_descriptor( libusb_get_device(devhandle),&handle);
    if (r || (handle == NULL)){
        MXPRINT("Failed REQ_INTERFACE_NUMBER  %d\n", r);
        libusb_free_config_descriptor(handle);
        return r;
     }

    num = handle->bNumInterfaces;


    for(i=0;i<num;i++){
        r = libusb_detach_kernel_driver(devhandle, i);
    }

    r = libusb_set_configuration(devhandle,config);
    if(r < 0) {
        MXPRINT("Failed SET_CONFIGURATION  %d\n", r);
        libusb_free_config_descriptor(handle);
        return r;
    }

    for(i=0;i<num;i++){
        r = libusb_attach_kernel_driver(devhandle , i);
    }

    libusb_free_config_descriptor(handle);

    MXPRINT("%s (OUT)\n",__func__);
    return 0;
}

/**
* \ingroup configuration
* \brief mxcam_i2c_write:
*   will do a i2c write operation in camera
* \param inst       : i2c instance (0 or 1)
* \param type       : i2c device type
* \param *payload   : pointer to payload information
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/

int mxcam_i2c_write(uint16_t inst, uint16_t type, i2c_payload_t *payload)
{
    i2c_data_t i2c_stat;
    int r=0;
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
    return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                     LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ I2C_WRITE,
                    /* wValue        */ (uint16_t)inst,
                    /* MSB 4 bytes   */
                    /* wIndex        */ (uint16_t)type,
                    /* Data          */ (unsigned char *)payload,
                    /* wLength       */ sizeof(i2c_payload_t),
                    /* timeout*/   EP0TIMEOUT
                    );
    if (r < 0) {
      MXPRINT("Failed I2C_WRITE %d\n", r);
      return r;
    }

    if (is_max64580(devhandle)){
      while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        usleep(1000*5);
        r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                    LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ I2C_WRITE_STATUS,
                    /* wValue        */ 0,
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char *)&i2c_stat,
                    /* wLength       */ sizeof(i2c_data_t),
                    /* timeout*/   EP0TIMEOUT
                    );
        if (r < 0) {
            MXPRINT("Failed I2C_WRITE_STATUS %d\n", r);
            return r;
        }
        if(i2c_stat.len == 0xF){
            mx64580_count++;
            usleep(1000*5);
        } else
            break;
    }
    } else {
        r = libusb_control_transfer(devhandle,
                        /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ I2C_WRITE_STATUS,
                        /* wValue        */ 0,
                        /* wIndex        */ 0,
                        /* Data          */ (unsigned char *)&i2c_stat,
                        /* wLength       */ sizeof(i2c_data_t),
                        /* timeout*/   EP0TIMEOUT
                        );
        if (r < 0) {
            MXPRINT("Failed I2C_WRITE_STATUS %d\n", r);
            return r;
        }
    }

    if (i2c_stat.len < 0) {
    MXPRINT("i2c write error\n");
    return MXCAM_ERR_I2C_WRITE;
    }

    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_i2c_read:
*       will do a i2c read operation in camera
* \param inst       : i2c instance (0 or 1)
* \param type       : i2c device type
* \param *payload   : pointer to payload information - value returned here
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_i2c_read(uint16_t inst, uint16_t type, i2c_payload_t *payload)
{
    int r;
    i2c_data_t data;
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
    return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ I2C_READ,
                /* wValue        */ (uint16_t)inst,
                /* MSB 4 bytes   */
                /* wIndex        */ (uint16_t)type,
                /* Data          */ (unsigned char *)payload,
                /* wLength       */ (uint16_t)sizeof(i2c_payload_t),
                /* timeout*/   EP0TIMEOUT
                );
    if (r < 0) {
    MXPRINT("Failed I2C_READ %d\n", r);
    return r;
    }

    if(is_max64580(devhandle)){
    while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        usleep(1000*10);
        r = libusb_control_transfer(devhandle,
                        /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ I2C_READ_STATUS,
                        /* wValue        */ 0,
                        /* wIndex        */ 0,
                        /* Data          */ (unsigned char *)&data,
                        /* wLength       */ sizeof(i2c_data_t),
                        /* timeout*/   EP0TIMEOUT
                        );
        if (r < 0) {
            MXPRINT("Failed I2C_READ_STATUS %d\n", r);
            return r;
        }
        if(data.len == 0xF){
            mx64580_count++;
            usleep(1000*10);
        } else {
            break;
        }
    }
    } else {
        r = libusb_control_transfer(devhandle,
                            /* bmRequestType */
                            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                             LIBUSB_RECIPIENT_INTERFACE),
                            /* bRequest      */ I2C_READ_STATUS,
                            /* wValue        */ 0,
                            /* wIndex        */ 0,
                            /* Data          */ (unsigned char *)&data,
                            /* wLength       */ sizeof(i2c_data_t),
                            /* timeout*/   EP0TIMEOUT
                            );
        if (r < 0) {
            MXPRINT("Failed I2C_READ_STATUS %d\n", r);
            return r;
        }
    }

    if (data.len < 0 || data.len == 0xF) {
        MXPRINT("i2c read error\n");
        return MXCAM_ERR_I2C_READ;
    }

    memcpy(payload->data.buf, data.buf, data.len);

    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_i2c_burstwrite:
*   will do a i2c burst write operation in camera
* \param inst       : i2c instance (0 or 1)
* \param type       : i2c device type
* \param *payload   : pointer to payload information
* \param *buf       : pointer to buffer to be written
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/

int mxcam_i2c_burstwrite(uint16_t inst, uint16_t type, i2c_burstpayload_t *payload, unsigned char *buf)
{
    i2c_burststatus_t i2c_stat;
    int r=0;
    int mx64580_count = 0;
    int total_size=payload->len;
    unsigned char *buffer=buf;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
    return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                     LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ I2C_BURSTWRITE,
                    /* wValue        */ (uint16_t)inst,
                    /* MSB 4 bytes   */
                    /* wIndex        */ (uint16_t)type,
                    /* Data          */ (unsigned char *)payload,
                    /* wLength       */ sizeof(i2c_burstpayload_t),
                    /* timeout*/   EP0TIMEOUT
                    );
    if (r < 0) {
      MXPRINT("Failed I2C_BURSTWRITE %d\n", r);
      return r;
    }

    usleep(1000*100);
    while(total_size > 0){
        int readl = 0;
        
        if(FWPACKETSIZE > total_size)
            readl = total_size;
        else
            readl = FWPACKETSIZE;
       
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ I2C_BURSTDATA,
            /* wValue        */ payload->len,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ buffer,
            /* wLength       */ readl,
            /* timeout*/   0 
            );
        if (r < 0) {
            printf("Failed sending I2C_BURSTDATA %d\n", r);
            return r;
        }
        total_size = total_size - readl;
        buffer = buffer + readl;
    }

    while(mx64580_count <= MXCAM_I2CBURST_MAX_RETRIES){
        usleep(1000*5);
        r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                    LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ I2C_BURSTSTATUS,
                    /* wValue        */ 0,
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char *)&i2c_stat,
                    /* wLength       */ sizeof(i2c_burststatus_t),
                    /* timeout*/   EP0TIMEOUT
                    );
        if (r < 0) {
            MXPRINT("Failed I2C_BURSTSTATUS %d\n", r);
            return r;
        }
        if(i2c_stat.done == 0){
            mx64580_count++;
            usleep(1000*100);     //Burst write can take a long time
        } else
            break;
    }

    if (i2c_stat.len < 0 || i2c_stat.len == 0) {
        MXPRINT("i2c burst write error\n");
        return MXCAM_ERR_I2C_WRITE;
    }
    else if (i2c_stat.done == 0) {
        MXPRINT("i2c burst write timeout\n");
        return MXCAM_ERR_I2C_WRITE;
    }

    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_i2c_burstread:
*       will do a i2c burst read operation in camera
* \param inst       : i2c instance (0 or 1)
* \param type       : i2c device type
* \param *payload   : pointer to payload information
* \param *buf       : return value
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_i2c_burstread(uint16_t inst, uint16_t type, i2c_burstpayload_t *payload, unsigned char *buf)
{
    int r;
    i2c_burststatus_t i2c_stat;
    int mx64580_count = 0;
    int total_len=0;
    int readl;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
    return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ I2C_BURSTREAD,
                /* wValue        */ (uint16_t)inst,
                /* MSB 4 bytes   */
                /* wIndex        */ (uint16_t)type,
                /* Data          */ (unsigned char *)payload,
                /* wLength       */ (uint16_t)sizeof(i2c_payload_t),
                /* timeout*/   EP0TIMEOUT
                );
    if (r < 0) {
        MXPRINT("Failed I2C_BURSTREAD %d\n", r);
        return r;
    }

    while(mx64580_count <= MXCAM_I2CBURST_MAX_RETRIES){
        usleep(1000*10);
        r = libusb_control_transfer(devhandle,
                        /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ I2C_BURSTSTATUS,
                        /* wValue        */ 0,
                        /* wIndex        */ 0,
                        /* Data          */ (unsigned char *)&i2c_stat,
                        /* wLength       */ sizeof(i2c_burststatus_t),
                        /* timeout*/   EP0TIMEOUT
                        );
        if (r < 0) {
            MXPRINT("Failed I2C_BURSTSTATUS %d\n", r);
            return r;
        }
        if(i2c_stat.done == 0){
            mx64580_count++;
            usleep(1000*100);    //Burst read can take a long time
        } else {
            break;
        }
    }

    if (i2c_stat.len < 0 || i2c_stat.len == 0) {
        MXPRINT("i2c burst read error\n");
        return MXCAM_ERR_I2C_READ;
    }
    else if (i2c_stat.done == 0) {
        MXPRINT("i2c burst read timeout\n");
        return MXCAM_ERR_I2C_READ;
    }

    while(total_len < payload->len){
        if(FWPACKETSIZE > (payload->len-total_len))
            readl =  payload->len - total_len;
        else
            readl = FWPACKETSIZE;

        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ I2C_BURSTDATA,
                /* wValue        */ total_len,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char*)&buf[total_len],
                /* wLength       */ readl,
                /* imeout*/          EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed I2C_BURSTDATA %d\n", r);
            if ( r == LIBUSB_ERROR_PIPE ){
                return MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY;
            }
            return r;
        }
        total_len = total_len + readl;
    }

    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}
/**
* \ingroup configuration
* \brief mxcam_spi_rw:
*   will do a spi read / write operation in camera
* \param *payload   : spi data structure to tx/rx
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_spi_rw(spi_payload_t *payload)
{
    int r;
    spi_data_t data;
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ SPI_RW,
            /* wValue        */ (uint16_t)0x0,
            /* MSB 4 bytes   */
            /* wIndex        */ (uint16_t)0x0,
            /* Data          */ (unsigned char *)payload,
            /* wLength       */ (uint16_t)(sizeof(spi_payload_t)),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed SPI_RW %d\n", r);
        return r;
    }

    if(is_max64580(devhandle)) {
        while(mx64580_count <= MXCAM_SPI_MAX_RETRIES) {
            usleep(1000*5);
            r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SPI_RW_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&data,
                /* wLength       */ (uint16_t)(sizeof(spi_data_t)),
                /* timeout*/   EP0TIMEOUT
                );
            if (r < 0) {
                MXPRINT("Failed SPI_RW_STATUS %d\n", r);
                return r;
            }
        
            if(data.len == -1){
                mx64580_count++;
                usleep(1000*5);
            } else
                break;
        }
    } else {
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ SPI_RW_STATUS,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)&data,
            /* wLength       */ (uint16_t)(sizeof(spi_data_t)),
            /* timeout*/   EP0TIMEOUT
            );
        if (r < 0) {
            MXPRINT("Failed SPI_READ_STATUS %d\n", r);
            return r;
        }
    }

    if (data.len < 0) {
        MXPRINT("spi read error\n");
        return MXCAM_ERR_SPI_RW;
    }

    memcpy(payload->data.buf, data.buf, data.len);

    MXPRINT("%s (OUT) len %x buf %x %x\n",__func__, payload->data.len, payload->data.buf[0], payload->data.buf[1]);
    return MXCAM_OK;
}


/**
* \ingroup configuration
* \brief mxcam_tcw_write:
*   write a SPI device Timing Control Word on camera
* \param value      : value to be written
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_tcw_write(uint32_t value)
{
    unsigned char status[64];
    unsigned char cmd_sta = MXCAM_ERR_FAIL;
    int r;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ TCW_WRITE,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)&value,
            /* wLength       */ sizeof(value),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed TCW_WRITE, %d\n", r);
        return r;
    }

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ TCW_WRITE_STATUS,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */ status,
            /* wLength       */ sizeof(status),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed TCW_WRITE_STATUS %d\n", r);
        return r;
    }
    cmd_sta = *(unsigned char *)status;
    if (cmd_sta >= MXCAM_ERR_FAIL){
        return cmd_sta;
    }
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_tcw_read:
*       read SPI Device's Timimg Control Word
* \param *value         : return value
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_tcw_read(uint32_t *value)
{
        int r;
    unsigned char status[64];
    unsigned char cmd_sta;
    char data[] = "TCW";

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ TCW_READ,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *) data,
            /* wLength       */ (uint16_t)strlen(data),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed TCW_READ %d\n", r);
        return r;
    }
    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ TCW_READ_STATUS,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)&status,
            /* wLength       */ sizeof(status),
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed TCW_READ_STATUS %d\n", r);
        return r;
    }
    cmd_sta = *(unsigned char *)status;
    if (cmd_sta >= MXCAM_ERR_FAIL){
        return cmd_sta;
    }
    //skip the status byte
    memcpy(value, (status+1), sizeof(uint32_t));
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_isp_write:
*   will do a isp write operation in camera
* \param  addr      : isp register address
* \param  value     : value to be written
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/

int mxcam_isp_write(uint32_t addr, uint32_t value)
{
    unsigned char status[64];
    unsigned char cmd_sta;
    int r;
    int mx64580_count = 0;
    uint32_t isp_data[2];
    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
    return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
    return MXCAM_ERR_INVALID_DEVICE;

    isp_data[0] = addr;
    isp_data[1] = value;

    r = libusb_control_transfer(devhandle,
        /* bmRequestType */
        (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
         LIBUSB_RECIPIENT_INTERFACE),
        /* bRequest      */ ISP_WRITE,
        /* wValue        */ 0,
        /* wIndex        */ 0,
        /* Data          */ (unsigned char *)&isp_data[0],
        /* wLength       */ sizeof(isp_data),
        /* timeout*/   EP0TIMEOUT
        );

    if (r < 0) {
    MXPRINT("Failed ISP_WRITE %d\n", r);
    return r;
    }

    while(mx64580_count <= MXCAM_ISP_MAX_RETRIES){

        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ ISP_WRITE_STATUS,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */ status,
            /* wLength       */ sizeof(status),
            /* timeout*/   EP0TIMEOUT
            );

        if (r < 0) {
        MXPRINT("Failed ISP_WRITE_STATUS %d\n", r);
        return r;
        }
        if((status[0] & 0xff) != 0xff)
        break;
        mx64580_count++;
        usleep(1000*1);
    }

    cmd_sta = *(unsigned char *)status;
    if (cmd_sta >= MXCAM_ERR_FAIL){
    return cmd_sta;
    }

    MXPRINT("%s (OUT)\n",__func__);

    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_isp_read:
*       will do a isp read operation in camera
* \param  addr          : isp device address
* \param *value         : return value
* \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL
* \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
*/
int mxcam_isp_read(uint32_t addr, uint32_t *value)
{
    int r;
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
    return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

    r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ ISP_READ,
                    /* wValue        */ 0,
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char *)&addr,
                    /* wLength       */ (uint16_t)sizeof(uint32_t),
                    /* timeout*/   EP0TIMEOUT
                    );

    if (r < 0) {
        MXPRINT("Failed ISP_READ %d\n", r);
        return r;
    }

    while(mx64580_count <= MXCAM_ISP_MAX_RETRIES){
        r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ ISP_READ_STATUS,
                    /* wValue        */ 0,
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char *)value,
                    /* wLength       */ sizeof(uint32_t),
                    /* timeout*/   EP0TIMEOUT
                    );
        if (r < 0) {
            MXPRINT("Failed ISP_READ_STATUS %d\n", r);
            return r;
        }

        if((value[0] & 0xff) != 0xff)
            break;
        mx64580_count++;
        usleep(1000*20);     //Needs a bigger timeout in case codec firmware is running with high load
    }

    MXPRINT("%s (OUT)\n",__func__);

    return MXCAM_OK;
}

int mxcam_isp_enable(uint32_t enable)
{
    int r;
    unsigned char status[64];
    char data[] = "SAVE";
    int mx64580_count = 0;

    MXPRINT("%s (IN)\n",__func__);

#ifndef DONT_CHK_DEV_ID
    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;
#endif

    r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ ISP_ENABLE,
                    /* wValue        */ (uint16_t) enable,
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char *)data,
                    /* wLength       */ (uint16_t)strlen(data),
                    /* timeout*/   EP0TIMEOUT
                    );

    if (r < 0) {
        MXPRINT("Failed ISP_ENABLE %d\n", r);
        return r;
    }

    while(mx64580_count <= MXCAM_ISP_MAX_RETRIES){

       r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ ISP_ENABLE_STATUS,
                    /* wValue        */ 0,
                    /* wIndex        */ 0,
                    /* Data          */ status,
                    /* wLength       */ sizeof(status),
                    /* timeout*/   EP0TIMEOUT
                    );

        if (r < 0) {
        MXPRINT("Failed ISP_ENABLE_STATUS %d\n", r);
        return r;
        }

        if((status[0] & 0xff) != 0xff)
        break;
        mx64580_count++;
        usleep(1000*1);
    }

    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup configuration
* \brief mxcam_usbtest:
*   Set specified USB test mode
* \param testmode   :test mode to set
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - testmode is out of range
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*/
int mxcam_usbtest(uint32_t testmode)
{
#define USB_FEATURE_TESTMODE 2
    int r;
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || (testmode > 5))
        return MXCAM_ERR_INVALID_PARAM;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */ 0,
            /* bRequest      */ LIBUSB_REQUEST_SET_FEATURE,
            /* wValue        */ USB_FEATURE_TESTMODE,
            /* wIndex        */ (testmode << 8),
            /* Data          */ NULL,
            /* wLength       */ 0,
            /* timeout*/   EP0TIMEOUT
            );
    if (r < 0) {
        MXPRINT("Failed USB testmode %d\n", r);
        return r;
    }

    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
 * \ingroup configuration
 * \brief mxcam_qcc_write:
 *   will do a qcc write operation in camera
 * \param  bid       : qcc block id
 * \param  addr      : qcc register address
 * \param  length    : length of the register
 * \param  value     : value to be written
 * \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL or length is
 *  not 1, 2 or 4
 * \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
 * \retval Negativevalue - upon libusb error
 * \retval MXCAM_OK  - upon success
 *
 */
int mxcam_qcc_write(uint16_t bid, uint16_t addr, uint16_t length, uint32_t value)
{
    int cmd_sta;
    int r;


    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

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
        return MXCAM_ERR_INVALID_PARAM;
    }

    r = libusb_control_transfer(devhandle,
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
        MXPRINT("Failed QCC_WRITE %d\n", r);
        return r;
    }

    r = libusb_control_transfer(devhandle,
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
        MXPRINT("Failed QCC_WRITE_STATUS %d\n", r);
        return r;
    }

    MXPRINT("%s (OUT)\n",__func__);

    return cmd_sta;
}

/**
 * \ingroup configuration
 * \brief mxcam_isp_read:
 *       will do a isp read operation in camera
 * \param  bid       : qcc block id
 * \param  addr      : qcc register address
 * \param  length    : length of the register
 * \param *value     : return value
 * \retval MXCAM_ERR_INVALID_PARAM - devhandle is NULL or length is
 *  not 1, 2 or 4
 * \retval MXCAM_ERR_INVALID_DEVICE -  Device not booted completely
 * \retval Negativevalue - upon libusb error
 * \retval MXCAM_OK  - upon success
 *
 */
int mxcam_qcc_read(uint16_t bid, uint16_t addr, uint16_t length, uint32_t *value)
{
    int r;
    int mask;

    MXPRINT("%s (IN)\n",__func__);

    if(is_max64180(devhandle))
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;

    if(!(is_uvcdevice(devhandle)))
        return MXCAM_ERR_INVALID_DEVICE;

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
        return MXCAM_ERR_INVALID_PARAM;
    }

    r = libusb_control_transfer(devhandle,
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
        MXPRINT("Failed QCC_READ %d\n", r);
        return r;
    }

    *value &= mask;

    MXPRINT("%s (OUT)\n",__func__);

    return MXCAM_OK;
}


/**
* \ingroup configuration
* \brief mxcam_free_get_value_mem:
*   free the resource allocated by \ref mxcam_get_value
* \param *value_mem   :pointer to a memory,allocated by \ref mxcam_get_value

* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - value_mem  is NULL
* \retval MXCAM_ERR_GETVALUE_UNKNOWN_AREA -  Unknown area to get value
* \retval MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND -  Value not found for given KEY
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   free the resource allocated by \ref mxcam_get_value
*/
int mxcam_free_get_value_mem(char* value_mem)
{
    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !value_mem )
        return MXCAM_ERR_INVALID_PARAM;

    // add status byte first before the free
    value_mem -= 1 ;
    free(value_mem);
    MXPRINT("%s (OUT)\n",__func__);
    return MXCAM_OK;
}

/**
* \ingroup misc
* \brief mxcam_reset:
*   reboot the camera
*
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   reboot the camera
*/
int mxcam_reset(void)
{
    int ret;
    ret = tx_libusb_ctrl_cmd (RESET_BOARD, 0);
    /* 
     * device gets disconnected and libusb returns no device
     * consider it as success
     */
    if (ret == LIBUSB_ERROR_NO_DEVICE)
        ret = MXCAM_OK;
    return ret;
}

/**
* \ingroup misc
* \brief mxcam_reboot:
*   reboot the camera with Nth image in SNOR
*
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   reboot the camera
*/
int mxcam_reboot(uint16_t value)
{
    int ret;
    MXPRINT("mxcam_reboot called with value %d\n", value);
    ret = tx_libusb_ctrl_cmd (REBOOT_BOARD, value);
    /* 
     * device gets disconnected and libusb returns no device
     * consider it as success
     */
    if (ret == LIBUSB_ERROR_NO_DEVICE)
        ret = MXCAM_OK;
    return ret;
}

/**
* \page examplecode example code snippet to understand libmxcam API usage
*
\code
static int open_device()
{
    int ret=1;

    if (poll)
        ret = mxcam_poll_one();
    else if (poll_new)
        ret = mxcam_poll_new();
    else if (bus != 0 && dev_addr != 0)
        ret = mxcam_open_by_busaddr(bus, dev_addr);
    else
        ret = mxcam_open_by_devnum(dev_num);

    return ret;
}

static void reboot_maxim_camera(void)
{
    int r = 0;
    r = open_device();
    if (r){
        printf("Failed to open_device (%s)\n",mxcam_error_msg(r));

    }
    r = mxcam_reset();
    if (r) {
        printf("Failed %s (%s)\n",__func__,mxcam_error_msg(r));
    }
    mxcam_close();
}
\endcode
*/

/**
* \ingroup library
* \brief mxcam_register_vidpid:
*   register a device with its vid/pid
* \param vid   :vendor id
* \param pid   :product id
* \param *desc :device description
* \param chip  :camera soc type

* \retval MXCAM_ERR_VID_PID_ALREADY_REGISTERED - vid and pid already registered
* \retval MXCAM_OK  - upon success
*
* \remark
*   DEPRECATED.
*   This was needed for the scan, open and poll functions to work.
*/
int mxcam_register_vidpid(int vid, int pid, char *desc, SOC_TYPE soc, DEVICE_TYPE dev)
{
    /* For backward compatibility */
    return MXCAM_OK;
}

/**
* \ingroup library
* \brief mxcam_scan:
*   Scan for plugged Maxim cameras.
* \param **devlist   :list of registered devices
* \retval Negativevalue - upon libusb error
* \retval Positivevalue - number of device(s) found
* \retval MXCAM_OK  - upon success
* \remark
*/
#define CC_VIDEO 0x0e
#define CC_AUDIO 0x01
#define VENDOR_SPECIFIC 0xff
int mxcam_scan(struct mxcam_devlist **devlist, int fast_boot)
{
    struct libusb_device *dev;
    size_t i = 0;
    int r = 0, found;
    struct mxcam_devlist *usbdev, *usbdev_prev;

    if (devlist)
        *devlist = NULL;
    usbdev = NULL;
    usbdev_prev = NULL;

    r = libusb_init(&dcontext);
    if (r < 0) {
        return r;
    }

    if(fast_boot){
        fast_boot_enable = 1;
        return 1; //assume to found one geo device
    }

    if(devs != NULL && devs[0] != NULL)
        libusb_free_device_list(devs, 1);

    /* If cache exists, free it */
    if(devlist_cache != NULL) {
        struct mxcam_devlist *d = devlist_cache;
        while(devlist_cache != NULL) {
            d = devlist_cache;
            devlist_cache = devlist_cache->next;
            free(d);
        }
    }
    if (libusb_get_device_list(dcontext, &devs) < 0)
        return -1;

    found = 0;
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf_desc;
        const struct libusb_interface *dev_interface;
        const struct libusb_interface_descriptor *altsetting;
        int data[2] = {-1, -1};
        DEVICE_TYPE type = DEVTYPE_UNKNOWN;
        SOC_TYPE soc = SOC_UNKNOWN;

        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
            continue;

        r = libusb_get_config_descriptor_by_value(dev, 1, &conf_desc);
        if(r < 0)
            continue;

        /* Detecting the type and state of camera */
#if !defined(_WIN32)
        dev_interface = conf_desc->interface;
#else
        dev_interface = conf_desc->dev_interface;
#endif

        altsetting = dev_interface->altsetting;
        /* We are only interested in devices whose first USB class is
         *  - a Vendor specific class
         *  - a UVC class
         * */

#if !defined(_WIN32)
        if (altsetting->bInterfaceClass != VENDOR_SPECIFIC   &&
             altsetting->bInterfaceClass != CC_VIDEO )
#else
        if ( altsetting->bInterfaceClass != VENDOR_SPECIFIC   &&
             altsetting->bInterfaceClass != CC_VIDEO  &&
             altsetting->bInterfaceClass != CC_AUDIO )
#endif
        {
            libusb_free_config_descriptor(conf_desc);
            continue;
        }

        /* Open the device to communicate with it */
        r = libusb_open(dev, &devhandle);
        if (r < 0) {
            libusb_free_config_descriptor(conf_desc);
            continue;
        }

        /* Send Vendor specific command to determine if the
         * USB device is a Maxim camera*/
        r = mxcam_whoru((char *)&data);
        if (r<0) {
            /* Did not respond: this is not a Maxim camera
             * or it is an old generation Maxim camera that
             * support CMD_WHO_R_U. We don't support them
             * in this function (see mxcam_scan_oldcam())*/
            libusb_free_config_descriptor(conf_desc);
            libusb_close(devhandle);
            continue;
        }
        data[0] = le32toh((unsigned int) data[0]);
        data[1] = le32toh((unsigned int) data[1]);
        //printf("(%x:%x) chip:mode = %i:%i\n",
        //      desc.idVendor, desc.idProduct,
        //      data[0], data[1]);

        /* Get the type of Camera */
        switch(data[0]) {
        case MAX64380:
        case MAX64480:
        case MAX64580:
            soc = data[0];
            break;
        case MAX64180:
        default:
            /* Camera type not supported.
             * Skip to the next USB device */
            libusb_free_config_descriptor(conf_desc);
            libusb_close(devhandle);
            continue;
        }

        /* Get the mode in which the camera is */
        switch(data[1])  {
        case 0:
            type = DEVTYPE_UVC;
            break;
        case 1:
            type = DEVTYPE_BOOT;
            break;
        default:
            /* Camera mode mode not supported.
             * Skip to the next USB device */
            libusb_free_config_descriptor(conf_desc);
            libusb_close(devhandle);
            continue;
        }

        libusb_close(devhandle);

        found++;
        usbdev = malloc(sizeof(struct mxcam_devlist));
        usbdev->vid = desc.idVendor;
        usbdev->pid = desc.idProduct;
        usbdev->desc = "Camera";
        usbdev->bus = libusb_get_bus_number(dev);
        usbdev->addr = libusb_get_device_address(dev);
        usbdev->type = type;
        usbdev->soc = soc;
        usbdev->dev = dev;
        usbdev->next = NULL;
        if (usbdev_prev)
            usbdev_prev->next = usbdev;
        if (found == 1 && devlist)
            *devlist = usbdev;
        usbdev_prev = usbdev;
        libusb_free_config_descriptor(conf_desc);
    }

    devlist_cache = *devlist;
    devhandle = NULL;
    return found;
}

/**
* \ingroup library
* \brief mxcam_scan_old:
*   Scan for plugged old generation Maxim cameras.
* \param **devlist   :list of registered devices
* \retval Negativevalue - upon libusb error
* \retval Positivevalue - number of device(s) found
* \retval MXCAM_OK  - upon success
* \remark
*/
int mxcam_scan_oldcam(struct mxcam_devlist **devlist)
{
    struct libusb_device *dev;
    size_t i = 0;
    int tmp=1;
    int r = 0, found;
    struct mxcam_devlist *usbdev, *usbdev_prev;

    if (devlist)
        *devlist = NULL;
    usbdev = NULL;
    usbdev_prev = NULL;

    r = libusb_init(&dcontext);
    if (r < 0) {
        return r;
    }

    if(devs != NULL && devs[0] != NULL)
        libusb_free_device_list(devs, 1);

    /* If cache exists, free it */
    if(devlist_cache != NULL) {
        struct mxcam_devlist *d = devlist_cache;
        while(devlist_cache != NULL) {
            d = devlist_cache;
            devlist_cache = devlist_cache->next;
            free(d);
        }
    }
    if (libusb_get_device_list(dcontext, &devs) < 0)
        return -1;

    found = 0;
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf_desc;
        const struct libusb_interface *dev_interface;
        const struct libusb_interface_descriptor *altsetting;
        unsigned int buf;
        int data[2] = {-1, -1};
        DEVICE_TYPE type = DEVTYPE_UNKNOWN;
        SOC_TYPE soc = SOC_UNKNOWN;

        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
            continue;

        r = libusb_get_config_descriptor_by_value(dev, 1, &conf_desc);
        if(r < 0)
            continue;

        /* Detecting the type and state of camera */
#if !defined(_WIN32)
        dev_interface = conf_desc->interface;
#else
        dev_interface = conf_desc->dev_interface;
#endif
        altsetting = dev_interface->altsetting;

        /* Skip cameras that supports CMD_WHO_R_U */
#if !defined(_WIN32)
        if (altsetting->bInterfaceClass == VENDOR_SPECIFIC ||
                altsetting->bInterfaceClass == CC_VIDEO) 
#else
        if (altsetting->bInterfaceClass == VENDOR_SPECIFIC ||
                altsetting->bInterfaceClass == CC_VIDEO ||
                altsetting->bInterfaceClass == CC_AUDIO )
#endif
        {
            /* Open the USB device */
            r = libusb_open(dev, &devhandle);
            if (r < 0) {
                libusb_free_config_descriptor(conf_desc);
                continue;
            }

            /* Send CMD_WHO_R_U request */
            r = mxcam_whoru((char *)&data);
            if (r >= 0) {
                /* Got an answer: check whether this is a
                 * legal answer */
                if(data[0] >= 0 && data[0] < NUM_SOC &&
                    data[1] >= 0 && data[1] < NUM_DEVTYPE) {
                    /* This is a legal answer: this a camera
                     * supporting CMD_WHO_R_U.
                     * We skip it for this function. */
                    libusb_free_config_descriptor(conf_desc);
                    libusb_close(devhandle);
                    continue;
                }
            }
        }

        /* Vendor specific class for 'bootloader' state */
        if (altsetting->bInterfaceClass == VENDOR_SPECIFIC) {
            /* Check if it is a Maxim bootloader by trying
             * to boot a fake firmware*/
            r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                    LIBUSB_RECIPIENT_INTERFACE,
                    /* bRequest      */ 0xed ,
                    /* wValue        */ 0,
                    /* wIndex        */ 1,
                    /* Data          */ (unsigned char *) &tmp,
                    /* wLength       */ 4,
                    10);
            if ((r == -9) || (r == -99)) {
                libusb_free_config_descriptor(conf_desc);
                libusb_close(devhandle);
                continue;
            }

            type = DEVTYPE_BOOT;

            /* Detect wheter it is 64380 old bootloader or
             * 64180 bootloader. */
            r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                     LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ GET_EEPROM_CONFIG,
                    /* wValue        */ (uint16_t)MAXIM_INFO,
                    /* MSB 4 bytes   */
                    /* wIndex        */ 0,
                    /* Data          */ (unsigned char*)&buf,
                    /* wLength       */ sizeof(unsigned int),
                    /* Timeout*/        10
                    );

            if (r < 0){
                soc = MAX64380;
            } else
                soc = MAX64180;

            libusb_close(devhandle);

        /* Video class for 'booted' state */
#if !defined(_WIN32)
        } else if (altsetting->bInterfaceClass == CC_VIDEO) {
#else
        } else if ((altsetting->bInterfaceClass == CC_VIDEO)|| 
                   (altsetting->bInterfaceClass == CC_AUDIO)){
#endif
            type = DEVTYPE_UVC;

            /* Detect whether it is 64380 old firmware ot 64180. */
            /* Make a QCC read to detect if it is a 64180 or a
             * 64380: only 64380's firmware supports it */
            tmp = 4;
            r = libusb_control_transfer(devhandle,
                    /* bmRequestType */
                    (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                     LIBUSB_RECIPIENT_INTERFACE),
                    /* bRequest      */ QCC_READ,
                    /* wValue        */ 0x6,
                    /* wIndex        */ 0xfc,
                    /* Data          */ (unsigned char*)&tmp,
                    /* wLength       */ 4,
                    /* timeout       */ 100
                    );

            if(tmp == 0x500 || tmp == 0x510){
                soc = MAX64380;
            } else
                soc = MAX64180;

            libusb_close(devhandle);
        } else {
            libusb_free_config_descriptor(conf_desc);
            continue;
        }

        found++;
        usbdev = malloc(sizeof(struct mxcam_devlist));
        usbdev->vid = desc.idVendor;
        usbdev->pid = desc.idProduct;
        usbdev->desc = "Camera";
        usbdev->bus = libusb_get_bus_number(dev);
        usbdev->addr = libusb_get_device_address(dev);
        usbdev->type = type;
        usbdev->soc = soc;
        usbdev->dev = dev;
        usbdev->next = NULL;
        if (usbdev_prev)
            usbdev_prev->next = usbdev;
        if (found == 1 && devlist)
            *devlist = usbdev;
        usbdev_prev = usbdev;
        libusb_free_config_descriptor(conf_desc);
    }

    devlist_cache = *devlist;
    devhandle = NULL;
    return found;
}

static int device_count = 0;

/**
* \ingroup library
* \brief mxcam_open_by_devnum:
*   open a device based on device number
* \param dev_num   :device number
* \param devlist   : scanned device list

* \retval MXCAM_ERR_INVALID_DEVICE - Failed to open device
* \retval Negativevalue - upon libusb error
* \retval MXCAM_ERR_DEVICE_NOT_FOUND - Device not found
* \retval MXCAM_OK  - upon success
* \remark
* use \ref mxcam_scan to get valid device number
*/
int mxcam_open_by_devnum(int dev_num, struct mxcam_devlist *devlist)
{
    int r;

    if(devlist != NULL) {
        device_count++;
        //if there is no next device and device is still not found return error
        if((devlist->next == NULL) && (dev_num > device_count))
            return MXCAM_ERR_INVALID_DEVICE;    

        if(dev_num != device_count)
            return MXCAM_ERR_INVALID_PARAM;
    
        r = libusb_open(devlist->dev, &devhandle);
        if (r < 0) {
            return MXCAM_ERR_INVALID_DEVICE;
        }
        cur_mxdev = devlist;
        return MXCAM_OK;
    }
    return MXCAM_ERR_INVALID_DEVICE;
}

int mxcam_open_device_vid_pid(int vid, int pid){
#if !defined(WIN32)
    int r;

    devhandle = libusb_open_device_with_vid_pid (dcontext, vid, pid);

    if(!devhandle){
        return MXCAM_ERR_DEVICE_NOT_FOUND;
    }
    if(!mxcam_check_fastboot_compatible())
        return MXCAM_ERR_INVALID_BOOTLOADER;

    r = libusb_claim_interface(devhandle, 0);
    if(r){
        return MXCAM_ERR_INVALID_DEVICE;
    }
    return MXCAM_OK;
#else
	MXPRINT("Not supported on Windows\n");
	return MXCAM_ERR_INVALID_DEVICE;
#endif
}

int mxcam_check_fastboot_compatible(void){
#if !defined(WIN32)
    int r = 0;
    struct libusb_config_descriptor *conf_desc;
    const struct libusb_interface_descriptor *intf_desc;
    struct libusb_device *dev; 
    const struct libusb_interface *interface; 

    dev = libusb_get_device(devhandle);
    if(dev==NULL){
        return r;
    }
    //check if device has Bulk out ep
    r = libusb_get_config_descriptor_by_value(dev, 1, &conf_desc);
	if(r < 0){
		return r;
    }
    interface = conf_desc->interface;
    intf_desc = interface->altsetting;

    if((intf_desc->bInterfaceClass == VENDOR_SPECIFIC)  &&
        (intf_desc->bNumEndpoints == 1)){
        libusb_free_config_descriptor(conf_desc);
        return 1;
    }

    return r;
#else
	MXPRINT("Not supported on Windows\n");
	return 0;
#endif
}

/**
* \ingroup library
* \brief mxcam_open_by_busaddr:
*   Open a device based on its bus number and device address
* \param bus   :device number
* \param addr   :device address

* \retval MXCAM_ERR_INVALID_DEVICE - Failed to open device
* \retval Negativevalue - upon libusb error
* \retval MXCAM_ERR_DEVICE_NOT_FOUND - Device not found
* \retval MXCAM_OK on success
* \remark
* use \ref mxcam_scan to get valid bus number and address
*/
int mxcam_open_by_busaddr(int bus, int addr, struct mxcam_devlist *devlist)
{
    int r;

    if(devlist != NULL) {
        if(bus != devlist->bus || addr != devlist->addr) {
            if(devlist->next == NULL)
                return MXCAM_ERR_INVALID_DEVICE;
    
            return MXCAM_ERR_INVALID_PARAM;
        }

        r = libusb_open(devlist->dev, &devhandle);
        if (r < 0) {
            return MXCAM_ERR_INVALID_DEVICE;
        }
        cur_mxdev = devlist;
        return MXCAM_OK;
    }
    return MXCAM_ERR_DEVICE_NOT_FOUND;;
}

/**
* \ingroup library
* \brief mxcam_poll_one:
*   Check if a registered device is plugged and open it or open the first
* found if there are multiples registered devices connected. If no registered
* device is plugged yet wait until one is plugged
*
* \param devlist   :scanned device list

* \retval MXCAM_ERR_INVALID_DEVICE - Failed to open device
* \retval Negativevalue - upon libusb error
* \retval number of devices device found
*
*/
int mxcam_poll_one(struct mxcam_devlist *devlist)
{
    int ret;

    while(1) {
        ret = mxcam_open_by_devnum(1, devlist);
        if (ret == MXCAM_OK ||ret == MXCAM_ERR_INVALID_DEVICE ||ret < 0)
            break;
        sleep(1);
    };

    return ret;
}
#if 0
/**
* \ingroup library
* \brief mxcam_poll_new:
* Wait until a new registered device is plugged and open it
* \retval MXCAM_ERR_INVALID_DEVICE - Failed to open device
* \retval Negativevalue - upon libusb error
*/
int mxcam_poll_new(void)
{
    int ndev, found;
    struct mxcam_devlist *d, *dorig, *dnew;
    ndev = mxcam_scan(&dorig);

    /* Wait until a new device is connected */
    while (mxcam_scan(&dnew) <= ndev)
        sleep(1);

    /* A new device was connected; find which one and open it */
    while(dnew != NULL) {
        d = dorig;
        found = 0;
        while(d != NULL) {
            if (d->bus == dnew->bus && d->addr == dnew->addr) {
                found = 1;
                break;
            }
            d = d->next;
        }
        if (found == 0)
            return mxcam_open_by_busaddr(dnew->bus, dnew->addr);
        dnew = dnew->next;
    }

    return MXCAM_ERR_FAIL;
}
#endif
/**
* \ingroup library
* \brief mxcam_whoami:
* find connected camera core id
* \retval valid chip id 
*/
int mxcam_whoami(void)
{
    int chip_id = 0;

    if (is_max64380(devhandle))
        chip_id = 64380;
    else if (is_max64480(devhandle))
        chip_id = 64480;
    else if (is_max64180(devhandle))
        chip_id = 64180;
    if (is_max64580(devhandle))
        chip_id = 64580;

    return chip_id;
}

/**
* \ingroup library
* \brief mxcam_get_cmd_bitmap:
* get supported command bitmap from camera fw 
* \retval on success, the number of bytes actually transferred  
* \retval Negative value - upon libusb error 
*/
int mxcam_get_cmd_bitmap(char *buffer)
{
    int r;
    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_CMD_BITMAP,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)buffer,
            /* wLength       */ 256,
            /* timeout*/   1000
            );
    if (r < 0) {
        MXPRINT("Failed REQ_GET_EEPROM_VALUE %d\n", r);
        return r;
    }       

    return r;
}

/**
* \ingroup library
* \brief mxcam_whoru:
* get cmera id & mode from camera fw/bootloader 
* \retval on success, first 4 bytes are chip id & next 4 bytes are camera mode  
* \retval Negative value - upon libusb error 
*/
int mxcam_whoru(char *buffer)
{
    int r = 0;
    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ CMD_WHO_R_U,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)buffer,
            /* wLength       */ 8,
            /* timeout*/  100
            );

    if (r < 0) {
        MXPRINT("Failed WHO R U Req %d\n", r);
        return r;
    }       

    return r;   
}

/**
* \ingroup library
* \brief mxcam_close:
* close libmxcam library
*/
void mxcam_close(void)
{
    if(devs != NULL && devs[0] != NULL)
            libusb_free_device_list(devs, 1);
    if (devhandle != NULL) {
        libusb_release_interface(devhandle, 0);
        libusb_close(devhandle);
    }
    if(dcontext != NULL)
        libusb_exit(dcontext);
    if(devlist_cache != NULL) {
        struct mxcam_devlist *d = devlist_cache;
        while(devlist_cache != NULL) {
            d = devlist_cache;
            devlist_cache = devlist_cache->next;
            free(d);
        }
    }
    devs=NULL;
    devhandle=NULL;
    dcontext=NULL;
    device_count = 0;
}

int mxcam_memtest(uint32_t ddr_size)
{
    int r;
    unsigned char data[4];

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ MEMTEST,
            /* wValue        */ (uint16_t)ddr_size,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ data,
            /* wLength       */ 4,
            /* timeout*/  1000*30 //30sec timeout 
            );
    if (r < 0) {
        MXPRINT("Failed to start memtest %d\n", r);
        return r;
    }       

    return r;
}

int mxcam_get_memtest_result(uint32_t *value)
{
    int r;
    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ MEMTEST_RESULT,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)value,
            /* wLength       */ 4,
            /* timeout*/  1000*20  //20sec timeout
            );
    if (r < 0) {
        MXPRINT("Failed to get memtest result %d\n", r);
        return r;
    }       

    return r;
}

int mxcam_rw_gpio (int gpio_no, int value, int gpio_write, int *status)
{
    int r;
    uint8_t cmd;
    int mx64580_count = 0;
    char data[4];

    if(gpio_write)
        cmd = GPIO_WRITE;
    else
        cmd = GPIO_READ;

    r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ cmd,
            /* wValue        */ (uint16_t)gpio_no,
            /* MSB 4 bytes   */
            /* wIndex        */ value,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ 4,
            /* timeout*/     1000 //1sec timeout 
            );
    if (r < 0) {
        MXPRINT("Failed to program GPIO %d\n", r);
        return r;
    }

    while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GPIO_RW_STATUS,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ 4,
            /* timeout*/  1000  //1sec timeout 
            );
        if (r < 0) {
            MXPRINT("Failed to get GPIO_RW_STATUS %d\n", r);
            return r;
        }
        if((data[0] & 0xff) != 0xff)
            break;
        mx64580_count++;
        usleep(1000*10);
    }       

    *status = data[0];  

    return r;   
}

int mxcam_read_pwm(uint32_t id, 
    uint32_t *state, 
    uint32_t *hightime, 
    uint32_t *period,
    time_unit_t *unit)
{
    int ret = 0;
    char data[16];

    ret = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ PWM_READ,
            /* wValue        */ id,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ 16,
            /* timeout*/  1000*20  //20sec timeout 
            );
    if (ret < 0) {
        MXPRINT("Failed in PWM_READ %d\n", ret);
        return ret;
    }

    memcpy(state, data, sizeof(uint32_t));
    memcpy(unit, &data[4], sizeof(uint32_t));
    memcpy(hightime, &data[8], sizeof(uint32_t));
    memcpy(period, &data[12], sizeof(uint32_t));

    return 0;   
}

int mxcam_write_pwm(uint32_t id, uint32_t state, uint32_t hightime, uint32_t period,
           time_unit_t unit)
{
    int ret = 0;
    char data[12];

    memcpy(data, &state, sizeof(uint32_t));
    memcpy(&data[4], &hightime, sizeof(uint32_t));
    memcpy(&data[8], &period, sizeof(uint32_t));

    ret = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ PWM_WRITE,
            /* wValue        */ id,
            /* MSB 4 bytes   */
            /* wIndex        */ (unit==NANNO) ? 1 : 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ 12,
            /* timeout*/  1000*20  //20sec timeout 
            );
    if (ret < 0) {
        MXPRINT("Failed in PWM_WRITE %d\n", ret);
        return ret;
    }
    ret = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ PWM_STATUS,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ 4,
            /* timeout*/  1000*20  //20sec timeout 
            );
    if (ret < 0) {
        MXPRINT("Failed in PWM_STATUS %d\n", ret);
        return ret;
    }
    if(data[0] == 1)
        return -1;
    else if(data[0] == 2)
        return MXCAM_ERR_PWMLED_ACTIVE;
        
    return 0;
}

static int mxcam_get_av_stream_status(void)
{
    int ret = 0;
    uint32_t state;

    ret = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ GET_AV_STREAMING_STATE,
            /* wValue        */ 0,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)&state,
            /* wLength       */ 4,
            /* timeout*/  1000*20  //20sec timeout 
            );
    if (ret < 0) {
        MXPRINT("Failed in GET_AV_STREAMING_STATE %d\n", ret);
        return ret;
    }

    return state;
}

int mxcam_notify_json_ep0(const char *json, const char *bin)
{
    int ret = 0, total_size = 0;
    struct stat stfile;
    unsigned char *mjson = NULL;
    FILE *fd;
    //get json size
    if(stat(json, &stfile))
        return MXCAM_ERR_FILE_NOT_FOUND;

    total_size = stfile.st_size;
    if(S_ISREG(stfile.st_mode) != 1)
        return MXCAM_ERR_FILE_NOT_FOUND;

    if(total_size > MAX_JSON_FILE_SIZE){
        printf("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return MXCAM_ERR_IMAGE_SEND_FAILED; 
    }

    mjson = (unsigned char *)malloc(total_size+1);
    if(mjson == NULL){
        return MXCAM_ERR_MEMORY_ALLOC;
    }
    fd = fopen(json, "rb");
    if(fd == NULL){
        free(mjson);
        return MXCAM_ERR_FILE_NOT_FOUND;    
    }

    fread(mjson, stfile.st_size, 1, fd);
    fclose(fd);
    mjson[total_size] = 0;
    cJSON_Minify((char *)mjson);
    total_size = (int)strlen((const char *)mjson);
    total_size += 1;
    free(mjson);
    if(bin) {
        //Align binary file download to 4 byte boundary
        total_size = (total_size + 3) &~3;
        //get bin size
        if(stat(bin, &stfile))
            return MXCAM_ERR_FILE_NOT_FOUND;

        
        total_size += stfile.st_size;
        if(S_ISREG(stfile.st_mode) != 1)
            return MXCAM_ERR_FILE_NOT_FOUND;
        if(total_size > MAX_JSON_FILE_SIZE){
            printf("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return MXCAM_ERR_IMAGE_SEND_FAILED; 
        }
    }
    
    ret = libusb_control_transfer(devhandle,
        /* bmRequestType */
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
        LIBUSB_RECIPIENT_INTERFACE,
        /* bRequest      */ SEND_JSON_SIZE,
        /* wValue        */ total_size,
        /* wIndex        */ 0,
        /* Data          */ NULL,
        /* wLength       */ 0,
                    0);//No time out
    if (ret < 0) {
        return MXCAM_ERR_IMAGE_SEND_FAILED;
    }
    return MXCAM_OK;
}

int mxcam_notify_json(const char *json, const char *bin)
{
    int total_size = 0;
    struct stat stfile;

	if(!fast_boot_enable)
		return mxcam_notify_json_ep0(json, bin);

    //get json size
    if(stat(json, &stfile))
        return MXCAM_ERR_FILE_NOT_FOUND;

    total_size = stfile.st_size;
    if(S_ISREG(stfile.st_mode) != 1)
        return MXCAM_ERR_FILE_NOT_FOUND;

    if(total_size > MAX_JSON_FILE_SIZE){
        printf("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return MXCAM_ERR_IMAGE_SEND_FAILED; 
    }

    total_size += 1; //space for null

    if(bin) {
        //Align binary file download to 4 byte boundary
        total_size = (total_size + 3) &~3;
        //get bin size
        if(stat(bin, &stfile))
            return MXCAM_ERR_FILE_NOT_FOUND;

        
        total_size += stfile.st_size;
        if(S_ISREG(stfile.st_mode) != 1)
            return MXCAM_ERR_FILE_NOT_FOUND;
        if(total_size > MAX_JSON_FILE_SIZE){
            printf("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return MXCAM_ERR_IMAGE_SEND_FAILED; 
        }
    }
    json_file_size = total_size;

    return MXCAM_OK;
}

//check the json syntax
int mxcam_check_json_syntax(const char *json)
{
    char *json_mem = NULL;
    FILE *file = fopen(json, "rb");
    cJSON *root = NULL;
    struct stat stfile;
    int size;
    cJSON* obj_p;
    struct cJSON *attr;
    int error=0;

    if(file == NULL)
        return MXCAM_ERR_FILE_NOT_FOUND;
    if(stat(json, &stfile))
        return MXCAM_ERR_FILE_NOT_FOUND;

    size = stfile.st_size;

    json_mem = (char*)malloc(size);
    if(json_mem == NULL){
        printf("%s: ERR malloc failed\n",__func__);
        fclose(file);
        return 1;
    }

    fread((void*)json_mem, size, 1, file);
    root = cJSON_Parse((char *)json_mem);
    if(root==NULL){
        fclose(file);
        free(json_mem);
        return MXCAM_ERR_VEND_ERR_JSON_SYNTAX_ERR;
    }

    /* this parsing of system object is to verify whether all the keys in system object 
     * are in string format and not in any other format. This limitation is from firmware
     * parser limitation which accepts only strings in key value pairs for system object
     */
    obj_p = cJSON_GetObjectItem(root, "system");
    if ( obj_p == NULL || obj_p->child == NULL){
        fclose(file);
        cJSON_Delete(root);
        free(json_mem);
        return MXCAM_ERR_VEND_ERR_JSON_SYNTAX_ERR;
    }

//This max string size value is taken from firmware. uvc_vendor.c
#define SYSTEM_MAX_STRING_SIZE 65
    attr = obj_p->child;
    while(attr != NULL){
        if((attr)->type != cJSON_Object){
            if (((cJSON*)attr)->string == NULL || ((cJSON*)attr)->valuestring == NULL ){
                printf("@ %32s %32s\n", ((cJSON*)attr->prev)->string, ((cJSON*)attr->prev)->valuestring);
                if ( ((cJSON*)attr)->string != NULL ) {
                    printf("X %32s %32s\n", ((cJSON*)attr)->string, "null");
                }
                printf("all system key value should be in STRING format !!\n");
                error = 1;
                break;
            }
            //printf("sizeof key_size=%d,strlen=%d %s\t",SYSTEM_MAX_STRING_SIZE,strlen(((cJSON*)attr)->string),((cJSON*)attr)->string);
            //printf("sizeof val_size=%d,strlen=%d %s\n",SYSTEM_MAX_STRING_SIZE,strlen(((cJSON*)attr)->valuestring),((cJSON*)attr)->valuestring);
        }
        if ( attr->next == NULL )
            break;

        attr = attr->next;
    }

    if ( error == 1 ){
        fclose(file);
        cJSON_Delete(root);
        free(json_mem);
        return MXCAM_ERR_VEND_ERR_JSON_SYNTAX_ERR;
    }
   
         
    cJSON_Delete(root);
    fclose(file);   
    free(json_mem);

    return 0;
}

//check the json syntax
int mxcam_check_ispcfg_syntax(const char *ispcfg)
{
    FILE *file;
    int ch0, ch1;

    file = fopen(ispcfg, "rb");

    if(file == NULL)
        return MXCAM_ERR_FILE_NOT_FOUND;
   
    ch0 = fgetc(file);
    if(ch0 != EOF)
        ch1 = fgetc(file); 

    fclose(file);
    if(ch0 != 0xfa || ch1 != 0xce){
        return MXCAM_ERR_VEND_ERR_ISPCFG_SYNTAX_ERR;
    }

    return 0;
}

//this function will add space and new line in a Minified json file
static void json_deminify(char *json, char *out, int *length)
{
    int space = 0;
    int count = 0;
    while(*json){
        //if { or , is found increase space count and add newline
        if((*json == '{') || (*json == ',')){
            if(*json == '{'){
                space += 4;
            }
            *out++=*json++;
            (*length)++;
            //add new line
            *out++='\n';
            (*length)++;

            //add space
            for(count = 0; count<space; count++)
                *out++=' '; 
            *length = *length + space;
            
        }
        // if } is found adjust space
        else if(*json == '}'){

            *out++='\n';
            (*length)++;

            if(space > 4)   
                space = space - 4;
            else if(space == 4)
                space = 0;
            //add space
            for(count = 0; count<space; count++)
                *out++=' '; 
            *length = *length + space;

            *out++=*json++;
            (*length)++;
        }
        else{
            *out++=*json++; 
            (*length)++;
        }
    }
    *out = 0; // and null-terminate.
}

int mxcam_send_json_ep0(const char *json, const char *bin)
{
    int r = 0, total_size = 0;
    struct stat stfilejson;
    struct stat stfilebin;
    int retryc = 0;
    FILE *fd;
    unsigned char *buffer = NULL;
    unsigned char *mbuf = NULL;
    
    //get json size
    if(stat(json, &stfilejson))
        return MXCAM_ERR_FILE_NOT_FOUND;

    total_size = stfilejson.st_size;
    if(total_size > MAX_JSON_FILE_SIZE){
        printf("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return MXCAM_ERR_IMAGE_SEND_FAILED; 
    }

    total_size++;         //Keep space for NULL termination

    if(bin) {
        //Align binary file download to 4 byte boundary
        total_size = (total_size + 3) &~3;
        
        //get bin size
        if(stat(bin, &stfilebin))
            return MXCAM_ERR_FILE_NOT_FOUND;

        
        total_size += stfilebin.st_size;

        if(total_size > MAX_JSON_FILE_SIZE){
            printf("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return MXCAM_ERR_IMAGE_SEND_FAILED; 
        }
    }    
    buffer = (unsigned char *)malloc(total_size);
    if(buffer == NULL){
        return MXCAM_ERR_MEMORY_ALLOC;
    }
    mbuf = buffer;
    fd = fopen(json, "rb");
    if(fd == NULL){
        free(buffer);
        return MXCAM_ERR_FILE_NOT_FOUND;    
    }

    fread(buffer, stfilejson.st_size, 1, fd);
    fclose(fd);
    buffer[stfilejson.st_size] = 0;
    //Minify
    cJSON_Minify((char *)buffer);
    total_size = (int)strlen((const char *)buffer);
    total_size += 1;
    if(bin) {
        int minimized_json_size = total_size;
        //Align binary file download to 4 byte boundary
        total_size = (total_size + 3) &~3;
        fd = fopen(bin, "rb");
        if(fd == NULL){
            free(mbuf);
            return MXCAM_ERR_FILE_NOT_FOUND;    
        }

        fread(buffer + total_size, stfilebin.st_size, 1, fd);
        fclose(fd);
        total_size += stfilebin.st_size;
        printf("Sending Minified Configuration file %s of size %d Bytes and Binary file %s of size %d Bytes for a total of %d Bytes after alignment...\n",
               json,minimized_json_size, bin, (int)(stfilebin.st_size), total_size);    
    }
    else
        printf("Sending Minified Configuration file %s of size %d Bytes ...\n",json,total_size);    

    while(total_size > 0){
        int readl = 0;
        
        if(FWPACKETSIZE > total_size)
            readl = total_size;
        else
            readl = FWPACKETSIZE;
        
        //r = (int)fread(buffer, readl, 1, fd);
retry:
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ SEND_JSON_FILE,
            /* wValue        */ total_size,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ buffer,
            /* wLength       */ readl,
            /* timeout*/   0 
            );
        if (r != readl){
            printf("Retry:libusb_control_transfer cnt %d\n",retryc);
            if (retryc <= 3) {
                retryc++;
                goto retry;
            }
            
            if(mbuf)free(mbuf);
            printf("Failed SEND_JSON %d\n", r);
            return MXCAM_ERR_IMAGE_SEND_FAILED;
        }
        total_size = total_size - readl;
        buffer = buffer + readl;
    }   

    if(mbuf)free(mbuf);
    
    return MXCAM_OK;
}

int mxcam_send_json(const char *json, const char *bin)
{
    int r = 0, total_size = 0;
    struct stat stfilejson;
    struct stat stfilebin;
    FILE *fd;
    unsigned char *buffer = NULL;
    unsigned char *mbuf = NULL;
    int transferred;

	if(!fast_boot_enable)
		return mxcam_send_json_ep0(json, bin);

    //get json size
    if(stat(json, &stfilejson))
        return MXCAM_ERR_FILE_NOT_FOUND;

    total_size = stfilejson.st_size;
    if(total_size > MAX_JSON_FILE_SIZE){
        printf("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return MXCAM_ERR_IMAGE_SEND_FAILED; 
    }

    total_size++;         //Keep space for NULL termination

    if(bin) {
        //Align binary file download to 4 byte boundary
        total_size = (total_size + 3) &~3;
        
        //get bin size
        if(stat(bin, &stfilebin))
            return MXCAM_ERR_FILE_NOT_FOUND;

        
        total_size += stfilebin.st_size;

        if(total_size > MAX_JSON_FILE_SIZE){
            printf("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return MXCAM_ERR_IMAGE_SEND_FAILED; 
        }
    }    

    buffer = (unsigned char *)malloc(json_file_size);
    if(buffer == NULL){
        return MXCAM_ERR_MEMORY_ALLOC;
    }
    mbuf = buffer;
    fd = fopen(json, "rb");
    if(fd == NULL){
        free(buffer);
        return MXCAM_ERR_FILE_NOT_FOUND;    
    }

    fread(buffer, stfilejson.st_size, 1, fd);
    fclose(fd);
    buffer[stfilejson.st_size] = 0;
    //Minify
    //cJSON_Minify((char *)buffer); //dont minify for fastboot

    total_size = stfilejson.st_size + 1;
    if(bin) {
        int minimized_json_size = total_size;
        //Align binary file download to 4 byte boundary
        total_size = (total_size + 3) &~3;
        fd = fopen(bin, "rb");
        if(fd == NULL){
            free(mbuf);
            return MXCAM_ERR_FILE_NOT_FOUND;    
        }

        fread(buffer + total_size, stfilebin.st_size, 1, fd);
        fclose(fd);
        total_size += stfilebin.st_size;
        printf("Sending Config file %s of size %d Bytes and Binary file %s of size %d Bytes for a total of %d Bytes after alignment...\n",
               json,minimized_json_size, bin, (int)(stfilebin.st_size), total_size);    
    }
    else
        printf("Sending Config file %s of size %d Bytes ...\n",json,total_size);    

#if !defined(WIN32)
    r = libusb_bulk_transfer(devhandle,
                           0x01, //Bulk OUT EP
                           (unsigned char *)buffer,
                           total_size,
                           &transferred,
                           20000);
     if(r){
         printf("json tx error r = %d transferred %d\n",r,transferred);
         free(mbuf);
         return MXCAM_ERR_IMAGE_SEND_FAILED;
     }
#endif

    if(mbuf)free(mbuf);
    
    return MXCAM_OK;
}

// Returns < 0 on error
static int usb_send_ispcfgfile(struct libusb_device_handle *dhandle, 
    const char *filename, int fwpactsize,unsigned char brequest)
{
    int r, ret;
    int total_size;
    struct stat stfile;
    FILE *fd=NULL; 
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
   
    if(fd == NULL)
        return -1;
 
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

int mxcam_ispcfg_load_file(const char* file_name)
{
    int ret=0;
    int data[2]; //Dummy data - has to be sent

    if (devhandle == NULL){
        printf("camera handle is not initialised\n");
        return -1;
    }


    ret = libusb_control_transfer(devhandle,
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
    if(ret < 0) 
    {
        printf("UPDATE_ISPCFG failed\n");
        return ret;
    }
    
    ret = usb_send_ispcfgfile(devhandle, file_name, FWPACKETSIZE, SEND_ISPCFG_FILE);
    if(ret < 0)
    {
        printf("SEND_ISPCFG failed\n");
        return ret;
    }

    ret = libusb_control_transfer(devhandle,
                /* bmRequestType */
        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ ISPCFG_FILE_DONE,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)data,
                /* wLength       */ sizeof(data),
                /* timeout*/     0 
        );
    if(ret < 0)
        printf("ISPCFG_FILE_DONE failed\n");

    return ret;
}

/**
* \ingroup configuration
* \brief mxcam_write_ispcfg:
*   save a specific \ref CONFIG_AREA on camera persistent storage memory
* \param ispcfgfile     :ISP bin file to write
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* \retval Negativevalue - upon libusb erroru
* \retval MXCAM_OK  - upon success
*
* \remark
*   Use this API to store ISP configuration binary on camera's non volatile memory.
*/
int mxcam_write_ispcfg(const char *ispcfgfile, uint32_t isp_index)
{
    int r = MXCAM_OK, total_size;
    unsigned int status;
    unsigned char *buffer = NULL;
    FILE *fd;
    struct stat stfile;
    int mx64580_count = 0;
    unsigned char *mbuf = NULL;

    MXPRINT("%s (IN)\n",__func__);

    if(mxcam_get_av_stream_status() == STREAM_STATE_RUN){
        printf("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
#if !defined(_WIN32)
    fd = fopen(ispcfgfile, "rb");
#else
    r = fopen_s(&fd,ispcfgfile, "rb");
#endif
    if(stat(ispcfgfile,&stfile))
        return -1;

    total_size = stfile.st_size;
    if(total_size >= MAX_ISPCFG_FILE_SIZE){ //max supported size is MAX_JSON_FILE_SIZE
        printf("ERR: Max supported ISP configuration file size %d Bytes\n", MAX_ISPCFG_FILE_SIZE);
        goto out_error;
    }

    buffer = (unsigned char *)malloc(total_size);
    if(buffer == NULL){
        r = -1;
        goto out_error;
    }
    mbuf = buffer;

    //read the whole file in buffer
    fread(buffer, total_size, 1, fd);
    printf("Sending ISP Configuration file %s of size %d Bytes ...\n",ispcfgfile,total_size);

    while(total_size > 0){
        int readl = 0;
        
        if(FWPACKETSIZE > total_size)
            readl = total_size;
        else
            readl = FWPACKETSIZE;
        
        //r = (int)fread(buffer, readl, 1, fd);

        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ WRITE_ISPCFG_FILE,
            /* wValue        */ stfile.st_size,    //Current firmware expects full size to be passed here
            /* MSB 4 bytes   */
            /* wIndex        */ isp_index,
            /* Data          */ buffer,
            /* wLength       */ readl,
            /* timeout*/   0 
            );
        if (r < 0) {
            printf("Failed WRITE_ISPCFG %d\n", r);
            return r;
        }
        total_size = total_size - readl;
        buffer = buffer + readl;
    }

    while(mx64580_count <= MXCAM_I2C_MAX_RETRIES){
        usleep(1000*500); //typical block erase type will be more then 500ms    
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SET_OR_REMOVE_KEY_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&status,
                /* wLength       */ sizeof(unsigned int),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            if(fd)fclose(fd);
            if(mbuf)free(mbuf);
            return r;
        }
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;

        mx64580_count++;
    }
    if(status != MXCAM_OK){
        printf("ERROR: %s\n",get_status(status));   
    }
out_error:
    if(fd)fclose(fd);
    if(mbuf)free(mbuf);

    MXPRINT("%s (OUT)\n",__func__);
    return r;
}

/**
* \ingroup camerainfo
* \brief mxcam_read_ispcfg:
*   read the ISP config memory area stored on camera persistent storage memory
*
* \param *buf :config data will be written on buf on success
* \param len  :length of config data needs to copied from camera memory
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following condition meet
* - devhandle is NULL
* - buf is NULL
* - len is 0
* \retval MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY - if the vendor area not
* initialzed properly on the camera.
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   read camera ISP configuration from camera persistent storage memory
*/
int mxcam_read_ispcfg(char *buf, unsigned int len)
{
    int r;
    int readl = 0, total_len = 0;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL || !buf || len == 0)
        return MXCAM_ERR_INVALID_PARAM;

    while(total_len < (int)len){
        if(FWPACKETSIZE > (len-total_len))
            readl =  len - total_len;
        else
            readl = FWPACKETSIZE;

        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ READ_ISPCFG_FILE,
                /* wValue        */ total_len,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char*)&buf[total_len],
                /* wLength       */ readl,
                /* imeout*/          EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed GET_EEPROM_CONFIG %d\n", r);
            if ( r == LIBUSB_ERROR_PIPE ){
                return MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY;
            }
            return r;
        }
        total_len = total_len + readl;
    }

    MXPRINT("%s (OUT)\n",__func__);
    return 0;
}

int mxcam_get_ispcfg_size(unsigned int *len)
{
    int r;
    unsigned int length = 0;

    MXPRINT("%s (IN)\n",__func__);
    if(devhandle == NULL)
        return MXCAM_ERR_INVALID_PARAM;
    
    r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                        (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                         LIBUSB_RECIPIENT_INTERFACE),
                        /* bRequest      */ GET_ISPCFG_SIZE,
                        /* wValue        */ 0,
                        /* MSB 4 bytes   */
                        /* wIndex        */ 0,
                        /* Data          */ (unsigned char *)&length,
                        /* wLength       */ sizeof(unsigned int),
                        /* imeout*/          EP0TIMEOUT
                        );
    if (r < 0) {
        MXPRINT("Failed GET_ISPCFG_SIZE %d\n", r);
        if ( r == LIBUSB_ERROR_PIPE ){
            return MXCAM_ERR_FILE_NOT_FOUND;
        }
        return r;
    }

    *len = le32toh(length);

    return 0;
}

/**
* \ingroup configuration
* \brief mxcam_write_gridmap:
*   save a specific \ref CONFIG_AREA on camera persistent storage memory
* \param gridmap0     :Grid map text file to write
* \retval MXCAM_ERR_INVALID_PARAM - if any one of the following conditions meet
* - devhandle is NULL
* \retval Negativevalue - upon libusb error
* \retval MXCAM_OK  - upon success
*
* \remark
*   Use this API to store Grid Map file on camera's non volatile memory.
*/
int mxcam_write_gridmap(const char *input[])
{
    int r = MXCAM_OK;
    unsigned int status, total_size = 0;
    unsigned char *buffer = NULL, *buf = NULL;

	
    int mx64580_count = 0;
    
    MXPRINT("%s (IN)\n",__func__);

    if(mxcam_get_av_stream_status() == STREAM_STATE_RUN){
        printf("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }

	r = mxcam_ldmap_fill_map(input, &buffer, &total_size);
	if (r != MXCAM_OK) {
		if (buffer)
			free(buffer);
		return !MXCAM_OK;
	}
	buf = buffer;

    while(total_size > 0){
        int writel = 0;
        
        if(FWPACKETSIZE > total_size)
            writel = total_size;
        else
            writel = FWPACKETSIZE;
        
        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ WRITE_GRIDMAP_FILE,
            /* wValue        */ total_size,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ buffer,
            /* wLength       */ writel,
            /* timeout*/   0 
            );
        if (r < 0) {
            printf("Failed WRITE_GRIDMAP %d\n", r);
            return r;
        }
        total_size = total_size - writel;
        buffer = buffer + writel;
    }

    while(mx64580_count <= MXCAM_MAX_RETRIES){
        usleep(1000*500); //typical block erase type will be more then 500ms    
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SET_OR_REMOVE_KEY_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&status,
                /* wLength       */ sizeof(unsigned int),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            goto out_error;
        }
        
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;

        mx64580_count++;
    }
    if(status != MXCAM_OK){
        printf("ERROR: %s\n",get_status(status));   
    }

	if(buf) free(buf);
	total_size = 0;
		
	r = mxcam_ldmap_fill_map(input+4, &buffer, &total_size);
	if (r != MXCAM_OK) {
		if (buffer)
			free(buffer);
		return !MXCAM_OK;
	}
	buf = buffer;

    while(total_size > 0){
        int writel = 0;
        
        if(FWPACKETSIZE > total_size)
            writel = total_size;
        else
            writel = FWPACKETSIZE;

        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ WRITE_GRIDMAP_FILE2,
            /* wValue        */ total_size,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ buffer,
            /* wLength       */ writel,
            /* timeout*/   0 
            );
        if (r < 0) {
            printf("Failed WRITE_GRIDMAP %d\n", r);
            return r;
        }
        total_size = total_size - writel;
        buffer = buffer + writel;
    }

    while(mx64580_count <= MXCAM_MAX_RETRIES){
        usleep(1000*500); //typical block erase type will be more then 500ms    
        r = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ SET_OR_REMOVE_KEY_STATUS,
                /* wValue        */ 0,
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&status,
                /* wLength       */ sizeof(unsigned int),
                /* timeout*/   EP0TIMEOUT
                );
        if (r < 0) {
            MXPRINT("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            goto out_error;
        }
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;

        mx64580_count++;
    }
    if(status != MXCAM_OK){
        printf("ERROR: %s\n",get_status(status));   
    }

out_error:
    if(buf) free(buf);

    MXPRINT("%s (OUT)\n",__func__);
    return r;
}

#if !defined(_WIN32)

int mxcam_xmodem_transmit(const char *dev, const char *file)
{

    int fd = -1;
    unsigned char *buf = NULL;
    struct stat stat_buf;

    fd = open(file, O_RDONLY);
    if (fd < 0) {
        printf("File not found: %s\n", file);
        return MXCAM_ERR_FILE_NOT_FOUND;
    }

    if (fstat(fd, &stat_buf)) {
        printf("File (%s) cant be fstat, size %ld\n", 
                        file, stat_buf.st_size);
        close(fd);
        return MXCAM_ERR_IMAGE_SEND_FAILED;
    }   

    if (0 == (unsigned)stat_buf.st_size) {
                printf("Bad size: %s is not valid file\n", file);
                close(fd);
        return MXCAM_ERR_IMAGE_SEND_FAILED;
        }

    buf = (unsigned char *)mmap(0, stat_buf.st_size, 
                    PROT_READ, MAP_SHARED, fd, 0);
        if ((caddr_t)buf == (caddr_t)-1) {
            fprintf (stderr, " Can't read %s\n", file);
        close(fd);
        return MXCAM_ERR_IMAGE_SEND_FAILED;
        }

    if (xmodem_transmit(dev, buf, stat_buf.st_size) < 0) {
        printf("\nXmodem Transmit failed\n");
        (void) munmap((void *)buf, stat_buf.st_size);
        close(fd);
        return MXCAM_ERR_IMAGE_SEND_FAILED;
    }

    printf("\nXmodem Tranmit Done\n");

    (void) munmap((void *)buf, stat_buf.st_size);
    close(fd);
    return MXCAM_OK;
}
#endif

int mxcam_handle_pwmled(pwmled_info_t *led, char pwmled_write)
{
    int ret = 0;

    if(pwmled_write){
        ret = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ PWM_LED_SET,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)led,
                /* wLength       */ sizeof (pwmled_info_t),
                /* timeout*/  1000*20  //20sec timeout 
                );
        if (ret < 0) {
            MXPRINT("Failed in PWM_LED_SET %d\n", ret);
            return ret;
        } 
    }else{
        //read
        ret = libusb_control_transfer(devhandle,
                /* bmRequestType */
                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                 LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ PWM_LED_GET,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)led,
                /* wLength       */ sizeof (pwmled_info_t),
                /* timeout*/  1000*20  //20sec timeout 
                );
        if (ret < 0) {
            MXPRINT("Failed in PWM_LED_GET %d\n", ret);
            return ret;
        }
    }

    return ret;
}

int mxcam_boot_from_snor(uint32_t json_index)
{
    int ret = 0;

    ret = libusb_control_transfer(devhandle,
            /* bmRequestType */
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE,
            /* bRequest      */ START_SNOR_BOOT,
            /* wValue        */ json_index,
            /* wIndex        */ 0,
            /* Data          */ NULL,
            /* wLength       */ 0,
                        0);//No time out
 
    return ret;
}
