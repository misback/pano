#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if !defined(_WIN32)
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <arpa/inet.h>
#else
#include <sys/stat.h>
#include "getopt-win.h"
#include "devsetup.h"
#endif
#include "libmxcam.h"
#include "common.h"
#include "mxcam.h"
int mxcam_gpiorw_on_and_off(int gpio_no, int value, struct libusb_device_handle *usb_devh)
{
	int r = 0, status = 0;
		
	if (value == 0 || value == 1) {
        r = mxcam_rw_gpio (gpio_no, value, 1, &status, usb_devh);
        if ((r<0) ||  (status != 0))
            MXCAM_ERROR("gpio write failed \n");
        else
            MXCAM_ERROR("write gpio on success\n");
    }

	return r;
}

#if defined(_WIN32)
char *strtok_r(char *str, const char *delim, char **saveptr){
     char *token = NULL;
     token = strtok( str, delim ); 
     *saveptr = token;
     return token;
}
#endif

int mxcam_get_vendor_info(char *release, char *vendor_info, struct libusb_device_handle *usb_devh)
{
    auto r = 0, i = 0;
    image_header_t hdr;
    struct mxcam_devlist *devlist = NULL;  
	static char *saveptr1, *saveptr2, *saveptr3;
    char *token;
	
    r = mxcam_read_flash_image_header(&hdr, RUNNING_FW_HEADER, usb_devh);
    if (r) {
        MXCAM_ERROR("mxcam_read_flash_image_header failed \n");
        return -1;
    }
	
    token = strtok_r((char *)hdr.ih_name, ";", &saveptr1);
    if (token != NULL) {
        token = strtok_r(saveptr1, ";",&saveptr2);
        if (token != NULL) {
            strcpy((char *)release, token);
            MXCAM_ERROR("Release Version  : %s\n", release);
        }
		
        memset(vendor_info, 0x0, sizeof(vendor_info));
        strncpy(vendor_info, (char *)hdr.ih_name + MAX_VENDOR_INFO - 1, MAX_VENDOR_INFO);
    }
	
    return MXCAM_OK;
}

int mxcam_subcmd_i2cwrite( const uint32_t value, struct libusb_device_handle *usb_devh)
{
	/**
	* value = 0x10 ,fps = 15;
	* value = 0x12 ,fps = 17;
	* value = 0x14 ,fps = 20;
	* value = 0x18 ,fps = 22;
	*/
    int r = 0;
    int i2c_inst = 1;
    int i2c_type = 1;
    i2c_payload_t payload;

	if (value < 0x10 || value > 0x18) {
		MXCAM_ERROR("the value is error\n");
        return 1;
	}

    payload.dev_addr = 0x20;
    payload.sub_addr = 0x3031;
    payload.data.len = 1;
    memcpy(payload.data.buf, (char *)&value, 1);
    r = mxcam_i2c_write(i2c_inst, i2c_type, &payload, usb_devh);
    if (r) {
        MXCAM_ERROR("mxcam_i2c_write failed\n");
        return 1;
    }

    return MXCAM_OK;
}

int	mxcam_subcmd_get_serialnumber(char *serialnumber, struct libusb_device_handle *usb_devh)
{
    int r = 0;
    unsigned int json_len = 0, i;
    char *file;

    r = mxcam_get_json_size(&json_len, usb_devh);
    if (json_len == 0 || json_len >= 64*1024) {
        MXCAM_ERROR("ERR: unsupported length of json %d Bytes", json_len);
        return -1;
    }

    file = malloc(json_len*3);
    memset(file, 0, json_len);
    r = mxcam_read_eeprom_config_mem(file, json_len);
    if (r < 0) {
        MXCAM_ERROR("mxcam_read_eeprom_config_mem failed");
        free(file);
        return -1;
    }

    r = parse_json_serial_number(file, serialnumber);
    if (r < 0) {
        MXCAM_ERROR("parse_json_serial_number failed");
        free(file);
        return -1;
    }

    free(file);
    return r;
}

int	mxcam_subcmd_readcfg(char *serial, char *sensorl_xstart, char *sensorl_ystart, 
	char *sensorr_xstart, char *sensorr_ystart, struct libusb_device_handle *usb_devh)
{
    int r = 0;
    unsigned int json_len = 0, i;
    char *file;

    r = mxcam_get_json_size(&json_len, usb_devh);
    if (json_len >= 64*1024) {
        MXCAM_ERROR("ERR: unsupported length of json %d Bytes\n", json_len);
        return -1;
    }

    file = malloc(json_len*3);
    memset(file, 0, json_len);
    r = mxcam_read_eeprom_config_mem(file, json_len);
    if (json_len == 0) {
        free(file);
        return r;
    }

	parse_json(file, serial, sensorl_xstart, sensorl_ystart, sensorr_xstart, sensorr_ystart); 
    free(file);
	
    return r;
}

int mxcam_subcmd_writecfg(char *jsonfile, struct libusb_device_handle *usb_devh)
{
    int r = 0, total_size;
    struct stat stfile;
	
    if (stat(jsonfile, &stfile)) {
        MXCAM_ERROR("ERR: Invalid json file provided\n");
        return -1;
    }
	
    if (stfile.st_size <= 0) {
        MXCAM_ERROR("ERR: Invalid json file provided\n");
        return -1;
    }
    
    r = mxcam_save_eeprom_config(jsonfile, usb_devh);

    return r;   
}

int mxcam_subcmd_flash(char *firmware, struct libusb_device_handle *usb_devh)
{
	int r;
    fw_info *fw;
	char *cur_bootmode = NULL;
	
	fw = (fw_info *)malloc(sizeof(fw_info));
	if (NULL == fw) {
		MXCAM_ERROR("malloc failed");
		return 1;
	}

	memset(fw, 0, sizeof(fw_info));
	fw->image = firmware;
	fw->bootldr = NULL;
	fw->img_media = SNOR;
	fw->bootldr_media = 0;
	fw->mode = MODE_NONE;
	r = mxcam_upgrade_firmware(fw, mxcam_fw_print_status, 0, &cur_bootmode, usb_devh);
	if (r) {
        MXCAM_ERROR("mxcam_upgrade_firmware failed");
        free(fw);
        return 1;
    }
	
	wait_for_upgrade_complete(cur_bootmode);
    free(fw);
	
	return 0;
}

int mxcam_subcmd_readispcfg(char *ispcfgfile, char *lsd, char *dsc, struct libusb_device_handle *usb_devh)
{
    int r = 0;
    unsigned int ispcfg_len = 0, i;
    char *file;
    FILE *fd;

    r = mxcam_get_ispcfg_size(&ispcfg_len, usb_devh);
    if (ispcfg_len >= 64*1024) {
        MXCAM_ERROR("ERR: unsupported length of ISP configuration %d Bytes\n", ispcfg_len);
        return -1;
    }

    if(ispcfg_len == 0)
        return -1;
	
    file = malloc(ispcfg_len);
    if (file == NULL) {
        MXCAM_ERROR("ERR: Cannot allocate %d bytes for reading ISP configuration\n", ispcfg_len);
        return -1;
    }
	
    memset(file, 0, ispcfg_len);
    r = mxcam_read_ispcfg(file, ispcfg_len);
    MXCAM_ERROR("read configuration file of size %d Bytes\n", ispcfg_len);
    if (ispcfgfile) {
        fd = fopen(ispcfgfile, "wb+");
        fwrite(file, ispcfg_len, 1, fd);
        fclose(fd);
    } else {
        MXCAM_ERROR("%s\n", file);
    }

    free(file);
	
    return r;
}

int mxcam_subcmd_writeispcfg(char *ispcfgfile, struct libusb_device_handle *usb_devh)
{
    int r = 0, total_size; 
    struct stat stfile;
	
    r = mxcam_check_ispcfg_syntax(ispcfgfile, usb_devh);
    if(r){
        MXCAM_ERROR("ERR: mxcam_check_ispcfg_syntax failed");
        return 1;
    }

    r = mxcam_write_ispcfg(ispcfgfile);

    return r;
}

int mxcam_subcmd_boot(char *image, char *json, char *bin, struct libusb_device_handle *usb_devh)
{
    int r = 0;
	char *opt_image=NULL;

	if (json) {
	    r = mxcam_check_json_syntax(json, usb_devh);
	    if (r) {
	        MXCAM_ERROR("mxcam_check_json_syntax failed");
	        return 1;
	    }
	}

	if (bin) {
	    r = mxcam_check_ispcfg_syntax(bin, usb_devh);
	    if (r) {
	        MXCAM_ERROR("mxcam_check_ispcfg_syntax failed");
	        return 1;
	    }
	}

	if (json) {
	    r = mxcam_notify_json(json, bin);
	    if (r != 0) {
	        MXCAM_ERROR("mxcam_notify_json failed");
	        return 1;
	    }
	}

	if (image) {
	    r = mxcam_boot_firmware(image, opt_image, mxcam_fw_print_status, usb_devh);
	    if (r) {
	        MXCAM_ERROR("mxcam_boot_firmware failed");
	        return 1;
	    }
	}

    if (json) {
        r = mxcam_send_json(json, bin);
        if (r) {
        	MXCAM_ERROR("mxcam_send_json failed");
            return 1;
        }
    }

    return 0;
}

int mxcam_subcmd_bootmode(char *bootmode, struct libusb_device_handle *usb_devh)
{
    int ret=1;
	
    if (bootmode) {    
        if(strcmp(bootmode, "snor") == 0)
            ret = mxcam_set_key("BOOTMODE", "snor", usb_devh);
        else if(strcmp(bootmode, "usb") == 0)
            ret = mxcam_set_key("BOOTMODE", "usb", usb_devh);
        else if(strcmp(bootmode, "uart") == 0)
            ret = mxcam_set_key("BOOTMODE", "uart", usb_devh);
        else {
            MXCAM_ERROR("Valid Bootmodes are usb/snor/uart");
            return 1;
        }
    } else {
        char *value;
        ret = mxcam_get_value("BOOTMODE", &value, usb_devh);
        if (!ret) {
            MXCAM_ERROR("%s\n", value);
            mxcam_free_get_value_mem(value);
        }
    }

    if (ret) {
        MXCAM_ERROR("mxcam_set_key failed");
        return 1;
    }

    return 0;
}

int mxcam_subcmd_merge(char *newfile, char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart)
{
	FILE *fp;
	struct stat stfile;
	unsigned int total_size;
	unsigned char *buffer = NULL;
	unsigned char *mbuf = NULL;
	
    if (stat(newfile, &stfile)) {
        MXCAM_ERROR("ERR: Invalid json file provided\n");
        return 1;
    }

	if (stfile.st_size <= 0) {
        MXCAM_ERROR("ERR: Invalid json file provided\n");
       	return 1;
    }

	total_size = stfile.st_size;
    if (total_size >= MAX_JSON_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
       	return 1;
    }
	
	buffer = (unsigned char *)malloc(total_size + 1);
    if (buffer == NULL) {
   		MXCAM_ERROR("malloc failed...");
		return 1;
    }

	fp = fopen(newfile, "rb");
	if (NULL == fp) {
		MXCAM_ERROR("fopen failed ...");
		return 1;
	}

	int ret = 0;
	memset(buffer, 0, total_size);
	ret = fread(buffer, 1, total_size, fp);
    buffer[total_size] = 0;
	mbuf = parse_merge_json(buffer, serial, sensorl_xstart, sensorl_ystart, sensorr_xstart, sensorr_ystart);
	if (mbuf) {
		fp = fopen(NEW_JSON, "w+");
	    fwrite(mbuf, strlen(mbuf), 1, fp);
		free(buffer);
		fclose(fp);
	}
	
	return 0;
}

int mxcam_boot_from_snor(libusb_device_handle *usb_devh)
{
    int ret = 0;
    ret = libusb_control_transfer(usb_devh,
        /* bmRequestType */
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
        LIBUSB_RECIPIENT_INTERFACE,
        /* bRequest      */ START_SNOR_BOOT,
        /* wValue        */ 0,
        /* wIndex        */ 0,
        /* Data          */ NULL,
        /* wLength       */ 0,
        0);//No time out
    return ret;
}

int mxcam_reset(libusb_device_handle *usb_devh)
{
    int ret;
    char data[4] = {0};
    uint16_t wLength=4;
    VEND_CMD_LIST req = RESET_BOARD;
    uint16_t wValue = 0;
    ret = libusb_control_transfer(usb_devh,
        /* bmRequestType */
        LIBUSB_ENDPOINT_OUT| LIBUSB_REQUEST_TYPE_VENDOR |
        LIBUSB_RECIPIENT_INTERFACE,
        req,            /* bRequest */
        wValue,         /* wValue */
        0,              /* wIndex */
        (unsigned char *)data,  /* Data */
        wLength,
        EP0TIMEOUT);

    return ret;
}
