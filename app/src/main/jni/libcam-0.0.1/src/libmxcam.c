#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "cJSON.h"
#include "common.h"
#include "libmxcam.h"
static struct libusb_device_handle *devhandle = NULL;
static int fast_boot_enable = 0;
static int json_file_size = 0;
int mxcam_rw_gpio (int gpio_no, int value, int gpio_write, int *status, struct libusb_device_handle *devhandle)
{
    auto r;
    uint8_t cmd;
    auto mx64580_count = 0;
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
        MXCAM_ERROR("Failed to program GPIO \n");
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
			MXCAM_ERROR("Failed to get GPIO_RW_STATUS \n");
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

int mxcam_read_flash_image_header(image_header_t *header, IMG_HDR_TYPE hdr_type, struct libusb_device_handle *devhandle)
{
    int r;
	
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

    if (r < 0)
        return r;
	
    return MXCAM_OK;
}

int mxcam_i2c_write(uint16_t inst, uint16_t type, i2c_payload_t *payload, struct libusb_device_handle *usb_devh)
{
    i2c_data_t i2c_stat;
    int r=0;
    int a = 3;
    int mx64580_count = 0;
	devhandle = usb_devh;
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
        MXCAM_ERROR("libusb_control_transfer failed\n");
        return r;
    }

    if (a > 0) {
    	while(mx64580_count <= MXCAM_I2C_MAX_RETRIES) {
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
	            MXCAM_ERROR("libusb_control_transfer failed");
	            return r;
	        }
			
	        if(i2c_stat.len == 0xF) {
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
            MXCAM_ERROR("libusb_control_transfer failed");
            return r;
        }
    }

    if (i2c_stat.len < 0) {
        MXCAM_ERROR("i2c_stat.len failed\n");
   		return 1;
    }

    return MXCAM_OK;
}

int mxcam_get_json_size(unsigned int *len, struct libusb_device_handle *usb_devh)
{
    int r;
    unsigned int length = 0;
	devhandle = usb_devh;
    if (devhandle == NULL)
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
        MXCAM_ERROR("Failed GET_JSON_SIZE %d\n", r);
        if ( r == LIBUSB_ERROR_PIPE ){
            return MXCAM_ERR_FILE_NOT_FOUND;
        }
		
        return r;
    }

    *len = le32toh(length);

    return 0;
}

void json_deminify(char *json, char *out, int *length)
{
    int space = 0;
    int count = 0;
	
    while (*json) {
        if ((*json == '{') || (*json == ',')) {
            if (*json == '{') {
                space += 4;
            }
			
            *out++=*json++;
            (*length)++;
            *out++='\n';
            (*length)++;
            for(count = 0; count<space; count++)
                *out++=' '; 
            *length = *length + space;      
        } else if(*json == '}') {
            *out++='\n';
            (*length)++;
            if (space > 4)   
                space = space - 4;
            else if (space == 4)
                space = 0;
            for (count = 0; count<space; count++)
                *out++=' '; 
            *length = *length + space;

            *out++=*json++;
            (*length)++;
        } else {
            *out++=*json++; 
            (*length)++;
        }
    }
	
    *out = 0;
}

int mxcam_read_eeprom_config_mem(char *buf, unsigned int len)
{
    int r;
    int readl = 0, total_len = 0, length = 0;
    char *json_out = NULL;

    if (devhandle == NULL || !buf || len == 0)
        return MXCAM_ERR_INVALID_PARAM;

    while (total_len < (int)len) {
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
            MXCAM_ERROR("Failed GET_EEPROM_CONFIG %d\n", r);
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
    
    json_deminify(&buf[0], json_out, &length);  
    memcpy(&buf[0], json_out, length);
    free(json_out);

    return 0;
}

int parse_json_serial_number(char *pMsg, char *serial)
{
    if (NULL == pMsg) {
        MXCAM_ERROR("pMsg is NULL...");
        return -1;
    }

    cJSON *pJson = cJSON_Parse(pMsg);
    if (NULL == pJson) {
        MXCAM_ERROR("pJson is NULL...");
        return -1;
    }

    cJSON *pSub = cJSON_GetObjectItem(pJson, "system");
    if (NULL == pSub) {
        MXCAM_ERROR("get object named system faild");
        return -1;
    }

    cJSON *pSec = cJSON_GetObjectItem(pSub, "SERIALNUMBER");
    if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
        return -1;
    }

    bzero(serial, strlen(serial));
    memcpy(serial, pSec->valuestring, strlen(pSec->valuestring));
    return 0;
}

void parse_json(char *pMsg, char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart)
{	
	if (NULL == pMsg) {
		MXCAM_ERROR("pMsg is NULL...");
		return ;
	}
	
	cJSON *pJson = cJSON_Parse(pMsg);
	if (NULL == pJson) {
		MXCAM_ERROR("pJson is NULL...");
		return ;
	}
	
	cJSON *pSub = cJSON_GetObjectItem(pJson, "system");
	if (NULL == pSub) {
        MXCAM_ERROR("get object named system faild");
     	return ;    
    }
	
	cJSON *pSec = cJSON_GetObjectItem(pSub, "SERIALNUMBER");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
     	return ;    
    }
	
	bzero(serial, strlen(serial));
	memcpy(serial, pSec->valuestring, strlen(pSec->valuestring));
	
	pSec = cJSON_GetObjectItem(pSub, "SENSORL_XSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
     	return ;    
    }
	
	bzero(sensorl_xstart, strlen(sensorl_xstart));
	memcpy(sensorl_xstart, pSec->valuestring, strlen(pSec->valuestring));

	pSec = cJSON_GetObjectItem(pSub, "SENSORL_YSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
     	return ;    
    }
	
	bzero(sensorl_ystart, strlen(sensorl_ystart));
	memcpy(sensorl_ystart, pSec->valuestring, strlen(pSec->valuestring));

	pSec = cJSON_GetObjectItem(pSub, "SENSORR_XSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
     	return ;    
    }
	
	bzero(sensorr_xstart, strlen(sensorr_xstart));
	memcpy(sensorr_xstart, pSec->valuestring, strlen(pSec->valuestring));

	pSec = cJSON_GetObjectItem(pSub, "SENSORR_YSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
     	return ;    
    }
	
	bzero(sensorr_ystart, strlen(sensorr_ystart));
	memcpy(sensorr_ystart, pSec->valuestring, strlen(pSec->valuestring));
}

int mxcam_save_eeprom_config(const char *jsonfile, struct libusb_device_handle *usb_devh)
{
    int r = MXCAM_OK, total_size, json_size;
    unsigned int status;
    unsigned char *buffer = NULL;
    FILE *fd;
    struct stat stfile;
    int mx64580_count = 0;
    unsigned char *mbuf = NULL;
	devhandle = usb_devh;
	
    if (mxcam_get_av_stream_status() == STREAM_STATE_RUN) {
        MXCAM_ERROR("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
	
#if !defined(_WIN32)
    fd = fopen(jsonfile, "rb");
#else
    r = fopen_s(&fd,jsonfile, "rb");
#endif
    if(stat(jsonfile, &stfile))
        return -1;

    total_size = stfile.st_size;
    if (total_size >= MAX_JSON_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        goto out_error;
    }

    buffer = (unsigned char *)malloc(total_size+1);
    if (buffer == NULL) {
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
    MXCAM_ERROR("Sending Minified Configuration file %s of size %d Bytes ...\n", jsonfile, total_size);

    while (total_size > 0) {
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
            /* wIndex        */ 0,
            /* Data          */ buffer,
            /* wLength       */ readl,
            /* timeout*/   0 
            );
        if (r < 0) {
            MXCAM_ERROR("Failed SEND_JSON %d\n", r);
            return r;
        }
		
        total_size = total_size - readl;
        buffer = buffer + readl;
    }

    while (mx64580_count <= MXCAM_I2C_MAX_RETRIES) {
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
            MXCAM_ERROR("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            if(fd)fclose(fd);
            if(mbuf)free(mbuf);
            return r;
        }
		
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;
        mx64580_count++;
    }
	
    if(status != MXCAM_OK){
        MXCAM_ERROR("ERROR: %s\n", get_status(status));
    }
out_error:
    if(fd)fclose(fd);
    if(mbuf)free(mbuf);
	
    return 0;
}

int mxcam_get_av_stream_status(void)
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
        MXCAM_ERROR("Failed in GET_AV_STREAMING_STATE %d\n", ret);
        return ret;
    }

    return state;
}

char * get_status(const int status)
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

void mxcam_fw_print_status(FW_STATE st, const char *filename)
{
    if(st == FW_STARTED){
        MXCAM_ERROR("Sending %s...", filename);
        fflush(stdout);
    }
	
    if(st == FW_COMPLETED){
        MXCAM_ERROR(" : %s\n", "Done");
        fflush(stdout);
    }
}

int tx_libusb_ctrl_cmd(VEND_CMD_LIST req, uint16_t wValue)
{
    int r;
    char data[4] = {0};
    uint16_t wLength=4;
	
    r = libusb_control_transfer(devhandle,
                        /* bmRequestType */
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
        LIBUSB_RECIPIENT_INTERFACE,
        req,            /* bRequest */
        wValue,         /* wValue */
        0,          	/* wIndex */
        (unsigned char *)data,  /* Data */
        wLength,
        EP0TIMEOUT);
    if (r < 0) {
		MXCAM_ERROR("tx_libusb_ctrl_cmd failed......");
        return r;
    }

    return MXCAM_OK;
}

int mxcam_get_value(const char* keyname, char** value_out, struct libusb_device_handle *usb_devh)
{
    int r;
    unsigned char cmd_sta;
    unsigned char *value;
    char *data;
  	devhandle = usb_devh;
	
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
    data = NULL ;
    if (r < 0) {
        free( value ) ;
        MXCAM_ERROR("Failed REQ_GET_EEPROM_VALUE %d\n", r);
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
        MXCAM_ERROR("Failed GET_EEPROM_VALUE %d\n", r);
        return r;
    }
	
    cmd_sta = *(unsigned char *)value;
    if (cmd_sta == MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND){
        free(value);
        *value_out = NULL;
        return MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND;
    }
    
    value += 1;
    *value_out =(char *) value;
   
    return MXCAM_OK;
}

int mxcam_set_key(const char* keyname, const char* value, struct libusb_device_handle *usb_devh)
{
    int r,size;
    unsigned int status;
    char *packet;
    int mx64580_count = 0;
	devhandle = usb_devh;
	
    if (devhandle == NULL || !keyname || !value)
        return MXCAM_ERR_INVALID_PARAM;

    if (mxcam_get_av_stream_status() == STREAM_STATE_RUN) {
        MXCAM_ERROR("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
	
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
        MXCAM_ERROR("Failed SET_EEPROM_KEY_VALUE %d\n", r);
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
            MXCAM_ERROR("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            return r;
        }
        status = le32toh(status);
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;
        mx64580_count++;
    }

    if(status != MXCAM_OK){
        MXCAM_ERROR("Status: %s\n",get_status(status));
    }
    
    return MXCAM_OK;
}

int mxcam_free_get_value_mem(char* value_mem)
{
    if(devhandle == NULL || !value_mem )
        return MXCAM_ERR_INVALID_PARAM;

    value_mem -= 1 ;
    free(value_mem);

    return MXCAM_OK;
}

void *grab_file(const char *filename, int blksize, unsigned int *size,
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
    while ( (ret = (int)fread((char*)(buffer + (*size)), sizeof(char),max-(*size),fd)) > 0 ) {
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

int isvalidimage(image_header_t* img)
{
    return (ntohl(img->ih_magic) != IH_MAGIC) ? 1 : 0 ;
}

unsigned int get_loadaddr(image_header_t *img)
{
    return ntohl(img->ih_load);
}

int usb_send_file(struct libusb_device_handle *dhandle,
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

    if (stat(filename, &stfile))
        return MXCAM_ERR_FILE_NOT_FOUND;
	
    trabuffer = (unsigned char *)grab_file(filename, fwpactsize, &fsize, &wblkcount);
    if (!trabuffer)
        return MXCAM_ERR_NOT_ENOUGH_MEMORY;

    if (fsize < sizeof(image_header_t)) {
        free(trabuffer);
        return MXCAM_ERR_NOT_VALID_IMAGE;
    }
	
    memcpy(&img_hd, trabuffer, sizeof(image_header_t));
    if (!isbootld) {
        if (isvalidimage(&img_hd)) {
            free(trabuffer);
            return MXCAM_ERR_NOT_VALID_IMAGE;
        }
       
        if ((uint32_t)(stfile.st_size) != 
                (uint32_t)(ntohl(img_hd.ih_size) + sizeof(image_header_t))){    
            free(trabuffer);
            return MXCAM_ERR_NOT_VALID_IMAGE;
        }
    }

    imageaddr = get_loadaddr(&img_hd);
    total_remain = fsize;
    if ( fwupgrd == 0 ) {
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
            MXCAM_ERROR("Retry:libusb_control_transfer cnt %d\n",count);
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

int mxcam_upgrade_firmware(fw_info *fw, void (*callbk)(FW_STATE st, const char *filename), 
	int is_rom_img, char **cur_bootmode, struct libusb_device_handle *usb_devh)
{
    int r;
    image_header_t header;
    FILE *fin;
    char *hdr;
    unsigned int fsize=0;
    char *bootmode = NULL;
	devhandle = usb_devh;
	
    if (devhandle == NULL || !fw || (!fw->image && !fw->bootldr)
          ||fw->img_media >= LAST_MEDIA ||
          fw->bootldr_media >= LAST_MEDIA || !callbk) {
        return MXCAM_ERR_INVALID_PARAM;
    }

    if (mxcam_get_av_stream_status() == STREAM_STATE_RUN) {
        MXCAM_ERROR("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }

    r = tx_libusb_ctrl_cmd(FW_UPGRADE_START, FWPACKETSIZE);
    if (r)
        return r;
	
    r = mxcam_get_value("BOOTMODE", &bootmode, NULL);
    if (!r && strcmp(bootmode, "usb") != 0) {
        r = mxcam_set_key("BOOTMODE", "usb", NULL);
        if (r) {
            MXCAM_ERROR("Failed setting bootmode key\n");
			mxcam_free_get_value_mem(bootmode);
            return r;
        }
    } else {
        mxcam_free_get_value_mem(bootmode);
        bootmode = NULL;
    }

    if (bootmode != NULL) {
        *cur_bootmode = bootmode;
    } else
        *cur_bootmode = NULL;
	
    if (fw->image) {
        callbk(FW_STARTED, fw->image);
        r = tx_libusb_ctrl_cmd(START_TRANSFER, (uint16_t)(fw->img_media));
        if (r)
            return r;
		
        r = usb_send_file(devhandle, fw->image, TX_IMAGE, FWPACKETSIZE, 1, 0, &fsize);
        if (r)
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
        if (r)
            return r;
        /* Tx mboot header */
        header.ih_magic = ntohl(IH_MAGIC);

#if !defined(_WIN32)
        fin = fopen(fw->bootldr, "r" );
#else
        r = fopen_s(&fin, fw->bootldr, "r" );
#endif
        if (fin == (FILE *)NULL) {
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
        if (r)
            return r;
        callbk(FW_COMPLETED,fw->bootldr);
    }
	
    r = tx_libusb_ctrl_cmd(FW_UPGRADE_COMPLETE, (uint16_t)(fw->mode));
    if (r)
        return r;
    
    return MXCAM_OK;
}

int mxcam_read_nvm_pgm_status(unsigned char *status)
{
    int r;
   
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
        MXCAM_ERROR("Failed GET_NVM_PGM_STATUS  %d\n", r);
        return r;
    }
    
    return *status;
}

void wait_for_upgrade_complete(char *cur_bootmode)
{
    int r;
    int cnt = 0;
	unsigned char wrt_st = 0;
	
    while (1) {
        sleep(1);
        r = mxcam_read_nvm_pgm_status(&wrt_st);
        if (r<0) {
            MXCAM_ERROR("mxcam_read_nvm_pgm_status failed");
            break;
        }
		
        if (wrt_st >= MXCAM_ERR_FAIL) {
            MXCAM_ERROR("wrt_st >= MXCAM_ERR_FAIL");
            break;
        }
		
        if (wrt_st < MXCAM_ERR_FAIL) {
            MXCAM_ERROR("\rFirmware upgrade in progress...");
            fflush(stdout);
            if(3 == cnt)
                cnt = 0;
        }
		
        if (wrt_st == 0) {
            MXCAM_ERROR("\rCompleted firmware upgrade...:)             "
                    "                                           \n");
            fflush(stdout);
            break;
        }
    }

    if (cur_bootmode != NULL) {
        if((wrt_st == 0) || (r>=0))
            r = mxcam_set_key("BOOTMODE", cur_bootmode, NULL);    

        mxcam_free_get_value_mem(cur_bootmode);
    }
}

int mxcam_get_ispcfg_size(unsigned int *len, struct libusb_device_handle *usb_devh)
{
    int r;
    unsigned int length = 0;
	devhandle = usb_devh;

    if (devhandle == NULL)
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
        MXCAM_ERROR("Failed GET_ISPCFG_SIZE %d\n", r);
        if ( r == LIBUSB_ERROR_PIPE ){
            return MXCAM_ERR_FILE_NOT_FOUND;
        }
		
        return r;
    }

    *len = le32toh(length);

    return 0;
}

int mxcam_read_ispcfg(char *buf, unsigned int len)
{
    int r;
    int readl = 0, total_len = 0;

    if (devhandle == NULL || !buf || len == 0)
        return MXCAM_ERR_INVALID_PARAM;

    while (total_len < (int)len) {
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
            MXCAM_ERROR("Failed GET_EEPROM_CONFIG %d\n", r);
            if ( r == LIBUSB_ERROR_PIPE ){
                return MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY;
            }
			
            return r;
        }
		
        total_len = total_len + readl;
    }

    return 0;
}

int mxcam_check_ispcfg_syntax(char *ispcfg, struct libusb_device_handle *usb_devh)
{
    FILE *file;
    int ch0, ch1;
	devhandle = usb_devh;
	
    file = fopen(ispcfg, "rb");
    if (file == NULL)
        return MXCAM_ERR_FILE_NOT_FOUND;
   
    ch0 = fgetc(file);
    if (ch0 != EOF)
        ch1 = fgetc(file); 

    fclose(file);
    if (ch0 != 0xfa || ch1 != 0xce)
        return MXCAM_ERR_VEND_ERR_ISPCFG_SYNTAX_ERR;

    return 0;
}

int mxcam_write_ispcfg(const char *ispcfgfile)
{
    int r = MXCAM_OK, total_size;
    unsigned int status;
    unsigned char *buffer = NULL;
    FILE *fd;
    struct stat stfile;
    int mx64580_count = 0;
    unsigned char *mbuf = NULL;

    if (mxcam_get_av_stream_status() == STREAM_STATE_RUN) {
        MXCAM_ERROR("ERR: Camera A/V Streaming is running, Please stop streaming to update flash\n");
        return MXCAM_ERR_FEATURE_NOT_SUPPORTED;
    }
	
#if !defined(_WIN32)
    fd = fopen(ispcfgfile, "rb");
#else
    r = fopen_s(&fd, ispcfgfile, "rb");
#endif
    if(stat(ispcfgfile, &stfile))
        return -1;

    total_size = stfile.st_size;
    if (total_size >= MAX_ISPCFG_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max supported ISP configuration file size %d Bytes\n", MAX_ISPCFG_FILE_SIZE);
        goto out_error;
    }

    buffer = (unsigned char *)malloc(total_size);
    if (buffer == NULL) {
        r = -1;
        goto out_error;
    }
	
    mbuf = buffer;
    fread(buffer, total_size, 1, fd);
    while (total_size > 0) {
        int readl = 0;
        
        if(FWPACKETSIZE > total_size)
            readl = total_size;
        else
            readl = FWPACKETSIZE;

        r = libusb_control_transfer(devhandle,
            /* bmRequestType */
            (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
             LIBUSB_RECIPIENT_INTERFACE),
            /* bRequest      */ WRITE_ISPCFG_FILE,
            /* wValue        */ stfile.st_size,
            /* MSB 4 bytes   */
            /* wIndex        */ 0,
            /* Data          */ buffer,
            /* wLength       */ readl,
            /* timeout*/   0 
            );
        if (r < 0) {
            MXCAM_ERROR("Failed WRITE_ISPCFG %d\n", r);
            return r;
        }
		
        total_size = total_size - readl;
        buffer = buffer + readl;
    }

    while (mx64580_count <= MXCAM_I2C_MAX_RETRIES) {
        usleep(1000*500); 
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
            MXCAM_ERROR("Failed SET_OR_REMOVE_KEY_STATUS %d\n", r);
            if(fd)fclose(fd);
            if(mbuf)free(mbuf);
            return r;
        }
		
        if((status & MXCAM_STAT_EEPROM_SAVE_IN_PROG) != MXCAM_STAT_EEPROM_SAVE_IN_PROG)
            break;

        mx64580_count++;
    }
	
    if (status != MXCAM_OK)
        MXCAM_ERROR("ERROR: %s\n", get_status(status));
out_error:
    if(fd)fclose(fd);
    if(mbuf)free(mbuf);

    return 0;
}

int mxcam_check_json_syntax(const char *json, struct libusb_device_handle *usb_devh)
{
	devhandle = usb_devh;
    char *json_mem = NULL;
    FILE *file = fopen(json, "rb");
	if (NULL == file) {
		MXCAM_ERROR("mxcam_check_json_syntax failed");
		return 1;
	}
	
    cJSON *root = NULL;
    struct stat stfile;
    int size;
    cJSON* obj_p;
    struct cJSON *attr;
    int error=0;

    if(file == NULL)
        return 1;

    if(stat(json, &stfile))
        return 1;

    size = stfile.st_size;
    json_mem = (char*)malloc(size);
    if (json_mem == NULL) {
        MXCAM_ERROR("malloc failed\n");
        fclose(file);
        return 1;
    }

    fread((void*)json_mem, size, 1, file);
    root = cJSON_Parse((char *)json_mem);
    if (root==NULL) {
        fclose(file);
        free(json_mem);
        return 1;
    }

    obj_p = cJSON_GetObjectItem(root, "system");
    if ( obj_p == NULL || obj_p->child == NULL) {
        fclose(file);
        cJSON_Delete(root);
        free(json_mem);
        return 1;
    }

#define SYSTEM_MAX_STRING_SIZE 65
    attr = obj_p->child;
    while (attr != NULL) {
        if ((attr)->type != cJSON_Object) {
            if (((cJSON*)attr)->string == NULL || ((cJSON*)attr)->valuestring == NULL ) {
                MXCAM_ERROR("@ %32s %32s\n", ((cJSON*)attr->prev)->string, ((cJSON*)attr->prev)->valuestring);
                if ( ((cJSON*)attr)->string != NULL ) {
                    MXCAM_ERROR("X %32s %32s\n", ((cJSON*)attr)->string, "null");
                }

                MXCAM_ERROR("all system key value should be in STRING format !!\n");
                error = 1;
                break;
            }
        }

        if ( attr->next == NULL )
            break;

        attr = attr->next;
    }

    if ( error == 1 ) {
		fclose(file);
        cJSON_Delete(root);
        free(json_mem);
        return 1;
    }

    cJSON_Delete(root);
    fclose(file);
    free(json_mem);

    return 0;
}

int mxcam_notify_json(const char *json, const char *bin)
{
    int total_size = 0;
    struct stat stfile;
	
	if (!fast_boot_enable)
		return mxcam_notify_json_ep0(json, bin);

    if (stat(json, &stfile))
        return 1;

    total_size = stfile.st_size;
    if (S_ISREG(stfile.st_mode) != 1)
        return 1;

    if (total_size > MAX_JSON_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return 1;
    }

    total_size += 1;
    if (bin) {
        total_size = (total_size + 3) &~3;
        if(stat(bin, &stfile))
            return 1;

        total_size += stfile.st_size;
        if (S_ISREG(stfile.st_mode) != 1)
            return 1;

        if (total_size > MAX_JSON_FILE_SIZE) {
            MXCAM_ERROR("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return 1;
        }
    }

    json_file_size = total_size;

    return 0;
}

int mxcam_notify_json_ep0(const char *json, const char *bin)
{
    int ret = 0, total_size = 0;
    struct stat stfile;
    unsigned char *mjson = NULL;
    FILE *fd;

    if (stat(json, &stfile))
        return 1;

    total_size = stfile.st_size;
    if (S_ISREG(stfile.st_mode) != 1)
        return 1;

    if (total_size > MAX_JSON_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return 1;
    }

    mjson = (unsigned char *)malloc(total_size+1);
    if (mjson == NULL) {
        return 1;
    }

    fd = fopen(json, "rb");
    if (fd == NULL) {
        free(mjson);
        return 1;
    }

    fread(mjson, stfile.st_size, 1, fd);
    fclose(fd);
    mjson[total_size] = 0;
    cJSON_Minify((char *)mjson);
    total_size = (int)strlen((const char *)mjson);
    total_size += 1;
    free(mjson);
    if (bin) {
        total_size = (total_size + 3) &~3;
        if(stat(bin, &stfile))
            return 1;

        total_size += stfile.st_size;
        if(S_ISREG(stfile.st_mode) != 1)
            return 1;
		
        if (total_size > MAX_JSON_FILE_SIZE) {
            MXCAM_ERROR("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return 1;
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
                    0);
    if (ret < 0) {
        return 1;
    }

    return 0;
}

int mxcam_boot_firmware(const char *image, const char *opt_image, void (*callbk)(FW_STATE st, const char *filename), struct libusb_device_handle *usb_devh)
{
    int r;
	int a = 0;
    unsigned int fsize = 0;
	devhandle = usb_devh; 
	
    if (devhandle == NULL || !image || !callbk)
        return 1;
	
    callbk(FW_STARTED, image);
    r = usb_send_file(devhandle, image, 0xde, FWPACKETSIZE, 0, 0, &fsize);
    if (r) {
		MXCAM_ERROR("usb_send_file failed ");
    	return r;
    }
	
    callbk(FW_COMPLETED, image);
	fsize = 0;
    if (opt_image) {
        callbk(FW_STARTED,opt_image);
        r = usb_send_file(devhandle, opt_image, 0xed,
                      FWPACKETSIZE, 0, 0, &fsize);
        if (r)
    		return r;

        callbk(FW_COMPLETED, opt_image);
    }
	
	if (a > 2) {
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

            if (r < 0) {
				MXCAM_ERROR("libusb_control_transfer failed");
				return r;
            }
    }

    return 0;
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

	if (!fast_boot_enable)
		return mxcam_send_json_ep0(json, bin);

    if (stat(json, &stfilejson))
        return 1;

    total_size = stfilejson.st_size;
    if (total_size > MAX_JSON_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return 1;
    }

    total_size++;
    if (bin) {
        total_size = (total_size + 3) &~3;
        if(stat(bin, &stfilebin))
            return 1;

        total_size += stfilebin.st_size;
        if (total_size > MAX_JSON_FILE_SIZE) {
            MXCAM_ERROR("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return 1;
        }
    }

    buffer = (unsigned char *)malloc(json_file_size);
    if (buffer == NULL) {
        return 1;
    }

    mbuf = buffer;
    fd = fopen(json, "rb");
    if (fd == NULL) {
        free(buffer);
        return 1;
    }

    fread(buffer, stfilejson.st_size, 1, fd);
    fclose(fd);
    buffer[stfilejson.st_size] = 0;
    total_size = stfilejson.st_size + 1;
    if (bin) {
        int minimized_json_size = total_size;
        total_size = (total_size + 3) &~3;
        fd = fopen(bin, "rb");
        if (fd == NULL) {
            free(mbuf);
            return 1;
        }

        fread(buffer + total_size, stfilebin.st_size, 1, fd);
        fclose(fd);
        total_size += stfilebin.st_size;
        MXCAM_ERROR("Sending Config file %s of size %d Bytes and Binary file %s of size %d Bytes for a total of %d Bytes after alignment...\n",
               json,minimized_json_size, bin, (int)(stfilebin.st_size), total_size);
    }
    else
        MXCAM_ERROR("Sending Config file %s of size %d Bytes ...\n",json,total_size);

#if !defined(WIN32)
    r = libusb_bulk_transfer(devhandle,
                           0x01, //Bulk OUT EP
                           (unsigned char *)buffer,
                           total_size,
                           &transferred,
                           20000);
     if (r) {
         MXCAM_ERROR("json tx error r = %d transferred %d\n", r, transferred);
         free(mbuf);
         return 1;
     }
	 
#endif

    if(mbuf)free(mbuf);

    return 0;
}

int mxcam_send_json_ep0(const char *json, const char *bin)
{
	FILE *fd;
	int retryc = 0;
	int r = 0, total_size = 0;
    struct stat stfilejson;
    struct stat stfilebin;  
    unsigned char *buffer = NULL;
    unsigned char *mbuf = NULL;

    if (stat(json, &stfilejson))
        return 1;

    total_size = stfilejson.st_size;
    if (total_size > MAX_JSON_FILE_SIZE) {
        MXCAM_ERROR("ERR: Max support json file size %d Bytes\n", MAX_JSON_FILE_SIZE);
        return 1;
    }

    total_size++;
    if (bin) {        
        total_size = (total_size + 3) &~3;
        if (stat(bin, &stfilebin))
            return 1;

        total_size += stfilebin.st_size;
        if (total_size > MAX_JSON_FILE_SIZE) {
            MXCAM_ERROR("ERR: Max support json + bin file size %d Bytes\n", MAX_JSON_FILE_SIZE);
            return 1;
        }
    }

    buffer = (unsigned char *)malloc(total_size);
    if(buffer == NULL)
        return 1;

    mbuf = buffer;
    fd = fopen(json, "rb");
    if (fd == NULL) {
        free(buffer);
        return 1;
    }

    fread(buffer, stfilejson.st_size, 1, fd);
    fclose(fd);
    buffer[stfilejson.st_size] = 0;
    cJSON_Minify((char *)buffer);
    total_size = (int)strlen((const char *)buffer);
    total_size += 1;
    if (bin) {
        int minimized_json_size = total_size;
        total_size = (total_size + 3) &~3;
        fd = fopen(bin, "rb");
        if (fd == NULL) {
            free(mbuf);
			MXCAM_ERROR("fopen bin failed");
            return 1;
        }

        fread(buffer + total_size, stfilebin.st_size, 1, fd);
        fclose(fd);
        total_size += stfilebin.st_size;
        MXCAM_ERROR("Sending Minified Configuration file %s of size %d Bytes and Binary file %s of size %d Bytes for a total of %d Bytes after alignment...\n",
               json, minimized_json_size, bin, (int)(stfilebin.st_size), total_size);
    } else
        MXCAM_ERROR("Sending Minified Configuration file %s of size %d Bytes ...\n", json, total_size);

    while (total_size > 0) {
        int readl = 0;
        if(FWPACKETSIZE > total_size)
            readl = total_size;
        else
            readl = FWPACKETSIZE;
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
        if (r != readl) {
            MXCAM_ERROR("Retry:libusb_control_transfer cnt %d\n", retryc);
            if (retryc <= 3) {
                retryc++;
                goto retry;
            }

            if (mbuf)free(mbuf);
	            MXCAM_ERROR("Failed SEND_JSON %d\n", r);
	            return 1;
        }

        total_size = total_size - readl;
        buffer = buffer + readl;
    }

    if(mbuf)free(mbuf);

    return 0;
}

char *parse_merge_json(char *pMsg, char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart)
{
	if (NULL == pMsg) {
		MXCAM_ERROR("pMsg is NULL...");
	}

	cJSON *pJson = cJSON_Parse(pMsg);
	if (NULL == pJson) {
		MXCAM_ERROR("pJson is NULL...");
	}
	
	cJSON *pSub = cJSON_GetObjectItem(pJson, "system");
	if (NULL == pSub) {
        MXCAM_ERROR("get object named system faild");
    }
	
	cJSON *pSec = cJSON_GetObjectItem(pSub, "SERIALNUMBER");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
    }

	pSec->valuestring = serial;
	
	pSec = cJSON_GetObjectItem(pSub, "SENSORL_XSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
    }
	
	pSec->valuestring = sensorl_xstart;

	pSec = cJSON_GetObjectItem(pSub, "SENSORL_YSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
    }
	
	pSec->valuestring = sensorl_ystart;

	pSec = cJSON_GetObjectItem(pSub, "SENSORR_XSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
    }

	pSec->valuestring = sensorr_xstart;
	
	pSec = cJSON_GetObjectItem(pSub, "SENSORR_YSTART");
	if (NULL == pSec) {
        MXCAM_ERROR("get object named SERIALNUMBER faild");
    }

	pSec->valuestring = sensorr_ystart;
	return cJSON_Print(pJson);
}
