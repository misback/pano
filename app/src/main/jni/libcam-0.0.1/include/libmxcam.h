#ifndef __LIBMXCAM_H__
#define __LIBMXCAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libusb/libusb.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#if !defined(_WIN32)
#include <stdint.h>
#else
#include "win-libusb.h"
#define __func__  __FUNCTION__
#define sleep(x)	Sleep(x*1000)
#define usleep(x)   Sleep(x/1000)
#define S_ISREG(mode) (((mode) & S_IFREG) == S_IFREG)
#endif
#define MXCAM_I2C_MAX_RETRIES 		10
#define MAX64380_BUILDNUMBER_POS 	0
#define MAX64380_BUILDNUMBER_LEN 	6
#define MAX64380_RELEASE_POS 		(MAX64380_BUILDNUMBER_POS + MAX64380_BUILDNUMBER_LEN)
#define MAX64380_BRANCH_LEN 		18
#define MAX_VENDOR_INFO 			16
/* image types */
#define IH_TYPE_INVALID		0	/* Invalid Image		*/
#define IH_TYPE_STANDALONE	1	/* Standalone Program	*/
#define IH_TYPE_KERNEL		2	/* OS Kernel Image		*/
#define IH_TYPE_RAMDISK		3	/* RAMDisk Image		*/
#define IH_TYPE_MULTI		4	/* Multi-File Image		*/
#define IH_TYPE_FIRMWARE	5	/* Firmware Image		*/
#define IH_TYPE_SCRIPT		6	/* Script file			*/
#define IH_TYPE_FILESYSTEM	7	/* Filesystem Image (any type)	*/
#define IH_TYPE_DDRBOOT     8
#define IH_TYPE_CACHEBOOT   9
#define IH_TYPE_DDRFBOOT    10
#define IH_TYPE_CACHEFBOOT  11
/* Compression Types */
#define IH_COMP_NONE		0	/*  No	 Compression Used	*/
#define IH_COMP_GZIP		1	/* gzip	 Compression Used	*/
#define IH_COMP_BZIP2		2	/* bzip2 Compression Used	*/
#define EP0TIMEOUT   				(0)    /* unlimited timeout */
#define IH_NMLEN                	32  
#define MXCAM_I2C_PAYLOAD_DATA_LEN 	2
#define FWPACKETSIZE 				4088   /*3072  2048 4096 */
#define MAX_JSON_FILE_SIZE 			200*1024
#define IH_MAGIC        			0x27051956
#define MAX_ISPCFG_FILE_SIZE 64*1024
#define MAX_BOOTLOADER_SIZE     	64*1024

typedef enum {
	/** Success (no error) */
	MXCAM_OK = 0,
	/** Started EEPROM FW Upgrade */
	MXCAM_STAT_EEPROM_FW_UPGRADE_START,//1
 	/** Completed  EEPROM FW Upgrade */
	MXCAM_STAT_EEPROM_FW_UPGRADE_COMPLETE,//2
 	/** Started SNOR FW Upgrade */
	MXCAM_STAT_SNOR_FW_UPGRADE_START,//3
 	/** Completed SNOR FW Upgrade */
	MXCAM_STAT_SNOR_FW_UPGRADE_COMPLETE,//4
 	/** Completed FW Upgrade */
	MXCAM_STAT_FW_UPGRADE_COMPLETE,//5
 	/** EEPROM Erase in progress */
	MXCAM_STAT_EEPROM_ERASE_IN_PROG,//6
 	/** EEPROM config save in progress */
	MXCAM_STAT_EEPROM_SAVE_IN_PROG,//7
 	/** ERR numbers starts here */
	MXCAM_ERR_FAIL = 128,
 	/** FW Image is corrupted */
	MXCAM_ERR_FW_IMAGE_CORRUPTED,//129
 	/** SNOR FW upgrade failed */
	MXCAM_ERR_FW_SNOR_FAILED,//130
 	/** Unsupported Flash memory */
	MXCAM_ERR_FW_UNSUPPORTED_FLASH_MEMORY,//131
 	/** Erase size exceeds MAX_VEND_SIZE */
	MXCAM_ERR_ERASE_SIZE,//132
 	/** Unknown area to erase */
	MXCAM_ERR_ERASE_UNKNOWN_AREA,//133
 	/** Unknown area to save */
	MXCAM_ERR_SAVE_UNKNOWN_AREA,//134
 	/** Not enough memory to save new key on memory */
	MXCAM_ERR_SETKEY_OVER_FLOW_NO_MEM,//135
 	/** Unknown area to set key */
	MXCAM_ERR_SETKEY_UNKNOWN_AREA,//136
 	/** Unknown area to remove key */
	MXCAM_ERR_REMOVE_KEY_UNKNOWN_AREA,//137
 	/** Unknown area to get key */
	MXCAM_ERR_GETVALUE_UNKNOWN_AREA,//138
 	/** Value not found for given key */
	MXCAM_ERR_GETVLAUE_KEY_NOT_FOUND,//139
	/** Failed to read TCW from flash */
	MXCAM_ERR_TCW_FLASH_READ,//140
	/** Failed to write TCW on flash */
	MXCAM_ERR_TCW_FLASH_WRITE,//141
	/** Failed to allocate memory on camera*/
	MXCAM_ERR_MEMORY_ALLOC,//142
	/** Vendor area is not initialized */
	MXCAM_ERR_VEND_AREA_NOT_INIT,//143
	/** Json syntax error */
	MXCAM_ERR_VEND_ERR_JSON_SYNTAX_ERR, //144
	MXCAM_ERR_SETKEY_UNSUPPORTED, //145
	/** Ispcfg syntax error */
	MXCAM_ERR_VEND_ERR_ISPCFG_SYNTAX_ERR, //146
	//Don't change the above values
 	/** Unknown area to get config size */
	MXCAM_ERR_GET_CFG_SIZE_UNKNOWN_AREA = 150,
 	/** Invalid parameter(s) */
	MXCAM_ERR_INVALID_PARAM,//151
 	/** Not a valid device */
	MXCAM_ERR_INVALID_DEVICE,//152
 	/** Failed to send image */
	MXCAM_ERR_IMAGE_SEND_FAILED,//153
 	/** File not found */
	MXCAM_ERR_FILE_NOT_FOUND,//154
	/** Not enough memory */
	MXCAM_ERR_NOT_ENOUGH_MEMORY,//155
	/** Not a valid image */
	MXCAM_ERR_NOT_VALID_IMAGE,//156
 	/** vid and pid already registered */
	MXCAM_ERR_VID_PID_ALREADY_REGISTERED,//157
	 /** device not found */
 	MXCAM_ERR_DEVICE_NOT_FOUND,//158
	/** vendor area not initialized*/
	MXCAM_ERR_UNINITIALIZED_VENDOR_MEMORY,//159
	/** feature not supported*/
	MXCAM_ERR_FEATURE_NOT_SUPPORTED,//160
	/** i2c read error */
	MXCAM_ERR_I2C_READ = 180,
	/** i2c write error */
	MXCAM_ERR_I2C_WRITE,
	/** spi read-write error */
	MXCAM_ERR_SPI_RW,
    /** Invalid bootloader used */
    MXCAM_ERR_INVALID_BOOTLOADER,
    /** PWMLED is active **/
    MXCAM_ERR_PWMLED_ACTIVE
} MXCAM_STAT_ERR;

typedef enum {
	FW_UPGRADE_START = 0x02,
	FW_UPGRADE_COMPLETE,		//0x3
	START_TRANSFER,				//0x4
	TRANSFER_COMPLETE,			//0x5
	TX_IMAGE,					//0x6
	GET_NVM_PGM_STATUS,			//0x7
	GET_SNOR_IMG_HEADER,		//0x8
	GET_EEPROM_CONFIG,			//0x9
	GET_EEPROM_CONFIG_SIZE,		//0xA
	REQ_GET_EEPROM_VALUE,		//0xB
	GET_EEPROM_VALUE,			//0xC
	SET_EEPROM_KEY_VALUE,		//0xD
	REMOVE_EEPROM_KEY,			//0xE
	SET_OR_REMOVE_KEY_STATUS,	//0xF
	GET_JSON_SIZE,				//0x10
	SEND_JSON_FILE,				//0x11
	RESET_BOARD,				//0x12
	I2C_WRITE,					//0x13
	I2C_READ,					//0x14
	I2C_WRITE_STATUS, 			//0x15
	I2C_READ_STATUS, 			//0x16
	TCW_WRITE,					//0x17
	TCW_WRITE_STATUS,			//0x18
	TCW_READ,					//0x19
	TCW_READ_STATUS,			//0x1A
	ISP_WRITE,					//0x1B
	ISP_READ,					//0x1C
	ISP_WRITE_STATUS,			//0x1D
	ISP_READ_STATUS,			//0x1E
	ISP_ENABLE,					//0x1F
	ISP_ENABLE_STATUS,			//0x20
	REQ_GET_CCRKEY_VALUE,		//0x21
	GET_CCRKEY_VALUE,			//0x22
	GET_CCR_SIZE, 				//0x23
	GET_CCR_LIST, 				//0x24
	GET_IMG_HEADER, 			//0x25
	GET_BOOTLOADER_HEADER, 		//0x26
	GET_CMD_BITMAP, 			//0x27
	CMD_WHO_R_U, 				//0x28	
	AV_ALARM,					//0x29
	AUDIO_STATS,				//0x2A
	AUDIO_VAD, 					//0x2B
	REBOOT_BOARD,				//0x2C
	//GET_SPKR_VOL, 			//0x2C
	SET_SPKR_VOL, 				//0x2D
	
	MEMTEST, 					//0x2E	
	MEMTEST_RESULT, 			//0x2F	

	ADD_FONT, 					//0x30
	SEND_FONT_FILE, 			//0x31	
	FONT_FILE_DONE, 			//0x32
	ADD_TEXT, 					//0x33
	TEXT, 						//0x34
	REMOVE_TEXT, 				//0x35
	SHOW_TIME, 					//0x36
	HIDE_TIME, 					//0x37
	UPDATE_TIME, 				//0x38

	AEC_ENABLE, 				//0x39,
	AEC_DISABLE, 				//0x3a,
	AEC_SET_SAMPLERATE, 		//0x3b,
	AEC_SET_DELAY, 				//0x3c

	AUDCLK_MODE_GET, 			//0x3d
	AUDCLK_MODE_SET, 			//0x3e

	SPKR_ENABLE,				//0x3f
	SPKR_DISABLE,				//0x40

	GPIO_READ, 					//0x41
	GPIO_WRITE,					//0x42
	GPIO_RW_STATUS, 			//0x43

	PWM_READ, 					//0x44
	PWM_WRITE, 					//0x45
	PWM_STATUS, 				//0x46

	GET_SPKR_SAMPLING_RATE, 	//0x47
	SET_SPKR_SAMPLING_RATE,		//0x48

	GET_AV_STREAMING_STATE, 	//0x49
	
	SEND_JSON_SIZE, 			//0x4a

	A_INTENSITY,  				//0X4B
    PWM_LED_SET, 				//0x4C
    PWM_LED_GET, 				//0x4D

    AGC_ENABLE,					//0x4E
	AGC_DISABLE,				//0x4F
	AUDIO_FILTER_PARAM, 		//0x50
	
	SPI_RW, 					//0x51
	SPI_RW_STATUS, 				//0x52

    HOSTIO_CMD = 0x5f,
	QHAL_VENDOR_REQ_START = 0x60,	
	QCC_READ = QHAL_VENDOR_REQ_START,
	QCC_WRITE, 					//0x61,
	QCC_WRITE_STATUS, 			//0x62,
	MEM_READ,
	MEM_WRITE,
	MEM_READ_STATUS,
	MEM_WRITE_STATUS,
	QHAL_VENDOR_REQ_END,

	SET_QPARAM = 0x69,
	
	UPDATE_ISPCFG=0x79,
	SEND_ISPCFG_FILE,
	ISPCFG_FILE_DONE,
	GET_ISPCFG_SIZE,
	WRITE_ISPCFG_FILE,
	READ_ISPCFG_FILE,
	
	LOGO_CMD_INIT=0x89,
	LOGO_CMD_DEINIT,
	LOGO_CMD_Y_UPDATE,
	LOGO_CMD_Y_UPDATE_DONE,
	LOGO_CMD_UV_UPDATE,
	LOGO_CMD_UV_UPDATE_DONE,
	LOGO_CMD_ALPHA_Y_UPDATE,
	LOGO_CMD_ALPHA_Y_UPDATE_DONE,
	LOGO_CMD_ALPHA_UV_UPDATE,
	LOGO_CMD_ALPHA_UV_UPDATE_DONE,
	LOGO_CMD_ALPHA,
	LOGO_CMD_COLORKEY,
	ADDMASK,
	REMOVEMASK,
	UPDATEMASKCOLOR,
	MASK_STATUS, 

	WRITE_GRIDMAP_FILE,
	START_SNOR_BOOT,  		//0x9a,
	VENDOR_REQ_LAST = 0x9F
} VEND_CMD_LIST;

typedef struct image_header {
	/** Image Header Magic Number 0x27051956 */
	uint32_t        ih_magic;
	/** Image Header CRC Checksum */
	uint32_t        ih_hcrc;
	/** Image Creation Timestamp in  in ctime format*/
	uint32_t        ih_time;
	/** Image Data Size           */            
	uint32_t        ih_size;
	/** Data  Load  Address       */         
	uint32_t        ih_load;
	/** Entry Point Address       */  
	uint32_t        ih_ep;
	/** Image Data CRC Check sum   */
	uint32_t        ih_dcrc;
	/** Operating System          */
	uint8_t         ih_os;
	/** CPU architecture          */              
	uint8_t         ih_arch;
	/** Image Type                */
	uint8_t         ih_type;
	/** Compression Type          */
	uint8_t         ih_comp;
	/** Image Name                */     
	uint8_t         ih_name[IH_NMLEN]; 
} image_header_t;

typedef enum  {
	DEVTYPE_BOOT = 0,
	DEVTYPE_UVC,
	NUM_DEVTYPE,
	DEVTYPE_UNKNOWN,
} DEVICE_TYPE;

typedef enum  {	
	MAX64180 = 0,
	MAX64380,	
	MAX64480,
	MAX64580,
	NUM_SOC,
	SOC_UNKNOWN,
} SOC_TYPE;

struct mxcam_devlist {
	/** vendor id */
	int vid;
	/** product id */
	int pid;
	/** device desc as a string: DEPRECATED */
	const char *desc;
	/** connected bus*/
	int bus;
	/** connected address*/
	int addr;
	/** camera type **/
	DEVICE_TYPE type;
	/** soc **/
	SOC_TYPE soc;
	/** pointer to usb struct **/
	void *dev;
	/** link to next node */
	struct mxcam_devlist *next;
};

typedef enum  {
	RUNNING_FW_HEADER = 0,
	SNOR_FW_HEADER,
	BOOTLOADER_HEADER,
	UNDEFINED,
} IMG_HDR_TYPE;

typedef struct {
	char len; 
	unsigned char buf[MXCAM_I2C_PAYLOAD_DATA_LEN];
} i2c_data_t;

typedef struct {
	unsigned short dev_addr;
	unsigned short sub_addr;
	i2c_data_t     data;
} i2c_payload_t;

typedef enum
{
    STREAM_STATE_UNDEFINED,
    STREAM_STATE_RUN,
    STREAM_STATE_STOP
} stream_state_t;

typedef enum {
	/** firmware upgrade media is EEPROM */
	EEPROM=0x1,
 	/** firmware upgrade media is parallel NOR */
	PNOR,
 	/** firmware upgrade media is serial NOR */
	SNOR,
 	/** max upgrade media for validation*/
	LAST_MEDIA,
} PGM_MEDIA;

typedef enum  {
	/** don't set the bootmode of bootloader */
	MODE_NONE = 0,
 	/** set the bootmode as USB */
	MODE_USB,
 	/** set the bootmode as serial nor */
	MODE_SNOR,
} BOOTMODE;

typedef struct fw_info {
	/** application image */
	const char *image;
	/** bootloader image */
	const char *bootldr;
	/** program media of application image */
	PGM_MEDIA img_media;
	/** program media of bootloader image */
	PGM_MEDIA bootldr_media;
	/** set this bootmode after bootloader image program */
	BOOTMODE mode;
} fw_info;

typedef enum  {
	/** started firmware download over usb  */
	FW_STARTED = 0,
 	/** completed firmware download over usb */
	FW_COMPLETED,	
} FW_STATE;

int mxcam_rw_gpio (int gpio_no, int value, int gpio_write, int *status, struct libusb_device_handle *devhandle);
int mxcam_read_flash_image_header(image_header_t *header, IMG_HDR_TYPE hdr_type, struct libusb_device_handle *devhandle);
int mxcam_i2c_write(uint16_t inst, uint16_t type, i2c_payload_t *payload, struct libusb_device_handle *usb_devh);
int mxcam_get_json_size(unsigned int *len, struct libusb_device_handle *usb_devh);
void json_deminify(char *json, char *out, int *length);
int mxcam_read_eeprom_config_mem(char *buf, unsigned int len);
int parse_json_serial_number(char *pMsg, char *serial);
void parse_json(char *pMsg, char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart);
int mxcam_save_eeprom_config(const char *jsonfile, struct libusb_device_handle *usb_devh);
int mxcam_get_av_stream_status(void);
char *get_status(const int status);
void mxcam_fw_print_status(FW_STATE st, const char *filename);
int tx_libusb_ctrl_cmd(VEND_CMD_LIST req, uint16_t wValue);
int mxcam_get_value(const char* keyname, char** value_out, struct libusb_device_handle *usb_devh);
int mxcam_set_key(const char* keyname, const char* value, struct libusb_device_handle *usb_devh);
int mxcam_free_get_value_mem(char* value_mem);
void *grab_file(const char *filename, int blksize, unsigned int *size, unsigned int *totblks);
int isvalidimage(image_header_t* img);
unsigned int get_loadaddr(image_header_t *img);
int usb_send_file(struct libusb_device_handle *dhandle, const char *filename, unsigned char brequest, int fwpactsize, int fwupgrd, int isbootld, unsigned int *fsz);
int mxcam_upgrade_firmware(fw_info *fw, void (*callbk)(FW_STATE st, const char *filename), int is_rom_img, char **cur_bootmode, struct libusb_device_handle *usb_devh);
int mxcam_read_nvm_pgm_status(unsigned char *status);
void wait_for_upgrade_complete(char *cur_bootmode);
int mxcam_get_ispcfg_size(unsigned int *len, struct libusb_device_handle *usb_devh);
int mxcam_read_ispcfg(char *buf, unsigned int len);
int mxcam_check_ispcfg_syntax(char *ispcfg, struct libusb_device_handle *usb_devh);
int mxcam_write_ispcfg(const char *ispcfgfile);
int mxcam_check_json_syntax(const char *json, struct libusb_device_handle *usb_devh);
int mxcam_notify_json(const char *json, const char *bin);
int mxcam_notify_json_ep0(const char *json, const char *bin);
int mxcam_boot_firmware(const char *image, const char *opt_image, void (*callbk)(FW_STATE st, const char *filename), struct libusb_device_handle *usb_devh);
int mxcam_send_json(const char *json, const char *bin);
int mxcam_send_json_ep0(const char *json, const char *bin);
char *parse_merge_json(char *pMsg, char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart);

#ifdef __cplusplus
}
#endif

#endif /*__LIBMXCAM_H__*/
