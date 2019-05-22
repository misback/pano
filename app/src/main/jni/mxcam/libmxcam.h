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

#ifndef __LIBMXCAM_H__
#define __LIBMXCAM_H__

#if !defined(_WIN32)
	#include <stdint.h>
#else
	#include "win-libusb.h"
	#define __func__  __FUNCTION__
	#define sleep(x)	Sleep(x*1000)
    #define usleep(x)   Sleep(x/1000)
    #define S_ISREG(mode) (((mode) & S_IFREG) == S_IFREG)

#endif


#define MXCAM_I2C_PAYLOAD_DATA_LEN 2
#define MXCAM_SPI_PAYLOAD_DATA_LEN 513
// created as default retry macro
#define MXCAM_MAX_RETRIES 10 
#define MXCAM_I2C_MAX_RETRIES 10 
#define MXCAM_I2CBURST_MAX_RETRIES 100 
#define MXCAM_SPI_MAX_RETRIES 5
#define MXCAM_ISP_MAX_RETRIES 5

/** \ingroup misc
 * Error and status codes of libmxcam.
 * Most of libmxcam functions return 0 on success.a negative value indicates
 * its a libusb error.other wise one of the these codes on failure
 */
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
	FW_UPGRADE_COMPLETE,//0x3
	START_TRANSFER,//0x4
	TRANSFER_COMPLETE,//0x5
	TX_IMAGE,//0x6
	GET_NVM_PGM_STATUS,//0x7
	GET_SNOR_IMG_HEADER,//0x8
	GET_EEPROM_CONFIG,//0x9
	GET_EEPROM_CONFIG_SIZE,//0xA
	REQ_GET_EEPROM_VALUE,//0xB
	GET_EEPROM_VALUE,//0xC
	SET_EEPROM_KEY_VALUE,//0xD
	REMOVE_EEPROM_KEY,//0xE
	SET_OR_REMOVE_KEY_STATUS,//0xF
	GET_JSON_SIZE,//0x10
	SEND_JSON_FILE,//0x11
	RESET_BOARD,//0x12
	I2C_WRITE,//0x13
	I2C_READ,//0x14
	I2C_WRITE_STATUS, //0x15
	I2C_READ_STATUS, //0x16
	TCW_WRITE,//0x17
	TCW_WRITE_STATUS,//0x18
	TCW_READ,//0x19
	TCW_READ_STATUS,//0x1A
	ISP_WRITE,//0x1B
	ISP_READ,//0x1C
	ISP_WRITE_STATUS,//0x1D
	ISP_READ_STATUS,//0x1E
	ISP_ENABLE,//0x1F
	ISP_ENABLE_STATUS,//0x20
	REQ_GET_CCRKEY_VALUE,//0x21
	GET_CCRKEY_VALUE,//0x22
	GET_CCR_SIZE, //0x23
	GET_CCR_LIST, //0x24
	GET_IMG_HEADER, //0x25
	GET_BOOTLOADER_HEADER, //0x26
	GET_CMD_BITMAP, //0x27
	CMD_WHO_R_U, //0x28	
	AV_ALARM, //0x29
	AUDIO_STATS,//0x2A
	AUDIO_VAD, //0x2B
	REBOOT_BOARD,//0x2C
	//GET_SPKR_VOL, //0x2C
	SET_SPKR_VOL, //0x2D
	
	MEMTEST, //0x2E	
	MEMTEST_RESULT, //0x2F	

	AEC_ENABLE = 0x39,
	AEC_DISABLE, //0x3a,
	AEC_SET_SAMPLERATE, //0x3b,
	AEC_SET_DELAY, //0x3c

	AUDCLK_MODE_GET, //0x3d
	AUDCLK_MODE_SET, //0x3e

	SPKR_ENABLE,//0x3f
	SPKR_DISABLE,//0x40

	GPIO_READ, //0x41
	GPIO_WRITE,//0x42
	GPIO_RW_STATUS, //0x43

	PWM_READ, //0x44
	PWM_WRITE, //0x45
	PWM_STATUS, //0x46

	GET_SPKR_SAMPLING_RATE, //0x47
	SET_SPKR_SAMPLING_RATE,//0x48

	GET_AV_STREAMING_STATE, //0x49
	
	SEND_JSON_SIZE, //0x4a

	A_INTENSITY,  //0X4B
    PWM_LED_SET, //0x4C
    PWM_LED_GET, //0x4D

    AGC_ENABLE,//0x4E
	AGC_DISABLE,//0x4F
	AUDIO_FILTER_PARAM, //0x50
	
	SPI_RW, //0x51
	SPI_RW_STATUS, //0x52

    HOSTIO_CMD = 0x5f,
	QHAL_VENDOR_REQ_START = 0x60,	
	QCC_READ = QHAL_VENDOR_REQ_START,
	QCC_WRITE, //0x61,
	QCC_WRITE_STATUS, //0x62,
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

	ADD_FONT, 
	SEND_FONT_FILE, 
	FONT_FILE_DONE, 
	ADD_TEXT, 
	TEXT, 
	REMOVE_TEXT, 
	SHOW_TIME, 
	HIDE_TIME, 
	UPDATE_TIME,
	OVERLAY_FONT_INIT,
	OVERLAY_FONT_Y_UPDATE,
	OVERLAY_FONT_Y_UPDATE_DONE,
	OVERLAY_FONT_UV_UPDATE,
	OVERLAY_FONT_UV_UPDATE_DONE,
	OVERLAY_FONT_ALPHA_Y_UPDATE,
	OVERLAY_FONT_ALPHA_Y_UPDATE_DONE,
	OVERLAY_FONT_ALPHA_UV_UPDATE,
	OVERLAY_FONT_ALPHA_UV_UPDATE_DONE, 
	OVERLAY_FONT_FILE_DONE,
	OVERLAY_ADD_TEXT, 
	OVERLAY_TEXT, 
	OVERLAY_REMOVE_TEXT, 
	OVERLAY_SHOW_TIME, 
	OVERLAY_HIDE_TIME, 
	OVERLAY_UPDATE_TIME,

	LOGO_CMD_INIT,
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
	I2C_BURSTWRITE,
	I2C_BURSTREAD,
	I2C_BURSTDATA,
	I2C_BURSTSTATUS,
	WRITE_GRIDMAP_FILE2,

	START_SNOR_BOOT = 0xae,

    COMPRESSED_ALPHA_CMD_INIT = 0xaf,
    COMPRESSED_ALPHA_CMD_UPDATE = 0xb0,
    COMPRESSED_ALPHA_CMD_UPDATE_DONE = 0xb1,
    COMPRESSED_ALPHA_CMD_STATUS = 0xb2,

    IRCF_SET = 0xb3,
    IRCF_GET_STATE = 0xb4,
    
	VENDOR_REQ_LAST = 0xFF
} VEND_CMD_LIST;

/** \ingroup upgrade
 * program media used for firmware upgrade
 */
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

/** \ingroup configuration
* config area
*/
typedef enum {
	/** maxim config area */
	MAXIM_INFO = 1,
 	/** vendor config area */
	VENDOR_INFO,
 	/** max config area for validation*/
	LAST_INFO
} CONFIG_AREA;

/** \ingroup configuration
* endpoint configuration
*/
typedef enum {
	/** isoc */
	MAXIM_ISOC = 1,
	/** bulk */
	MAXIM_BULK,
}EP_CONFIG;

/** \ingroup configuration
* represents registered callback function state of
* \ref mxcam_get_all_key_values function
*/
typedef enum {
	/** found an valid key and value pair*/
	GET_ALL_KEY_VALID = 1,
 	/** config area parsing is completed */
	GET_ALL_KEY_COMPLETED,
 	/** found a empty config area*/
	GET_ALL_KEY_NO_KEY,
} GET_ALL_KEY_STATE;

/** \ingroup upgrade
* represents BOOTMODE used in \ref mxcam_upgrade_firmware function
*/
typedef enum  {
	/** don't set the bootmode of bootloader */
	MODE_NONE = 0,
 	/** set the bootmode as USB */
	MODE_USB,
 	/** set the bootmode as serial nor */
	MODE_SNOR,
} BOOTMODE;

/** \ingroup boot
*represents registered callback function state of
*\ref mxcam_boot_firmware function
*/
typedef enum  {
	/** started firmware download over usb  */
	FW_STARTED = 0,
 	/** completed firmware download over usb */
	FW_COMPLETED,	
} FW_STATE;

/** \ingroup library
*
*\ref mxcam_register_vidpid
*/
typedef enum  {
	/** MAX64180  */
	MAX64180 = 0,
 	/** MAX64380 */
	MAX64380,	
	MAX64480,
	MAX64580,
	NUM_SOC,
	SOC_UNKNOWN,
} SOC_TYPE;

/** \ingroup library
*
*\ref mxcam_register_vidpid
*/
typedef enum  {
	/** boot device used by the bootloader
 	* during the first stage of the booting process   */
	DEVTYPE_BOOT = 0,
 	/** enumerated as uvc device */
	DEVTYPE_UVC,
	NUM_DEVTYPE,
	DEVTYPE_UNKNOWN,
} DEVICE_TYPE;

/** \ingroup upgrade
*structure to encapsulate firmware upgrade information that used in
\ref mxcam_upgrade_firmware function
*/
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

/** \ingroup library
*
* \ref mxcam_read_flash_image_header 
*/
typedef enum  {
	RUNNING_FW_HEADER = 0,
	SNOR_FW_HEADER,
	BOOTLOADER_HEADER,
	UNDEFINED,
} IMG_HDR_TYPE;

/** \ingroup library
*structure to hold information of all registered devices 
*/
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

/** Image Header Magic Number */
#define IH_MAGIC        0x27051956
/** Image Name Length */
#define IH_NMLEN                32          

/*
 * Compression Types
 */
#define IH_COMP_NONE		0	/*  No	 Compression Used	*/
#define IH_COMP_GZIP		1	/* gzip	 Compression Used	*/
#define IH_COMP_BZIP2		2	/* bzip2 Compression Used	*/

/* image types */
#define IH_TYPE_INVALID		0	/* Invalid Image		*/
#define IH_TYPE_STANDALONE	1	/* Standalone Program		*/
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

/** \ingroup configuration
* 64bytes flash image header information
*/
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

typedef struct {
	char len; // dual purpose for status and buf len
	unsigned char buf[MXCAM_I2C_PAYLOAD_DATA_LEN];
} i2c_data_t;
typedef struct {
	unsigned short dev_addr;
	unsigned short sub_addr;
	i2c_data_t     data;
} i2c_payload_t;

typedef struct {
	int done;         //0: Not done, 1: Done
	int len;          //Length read. -1 on error
} i2c_burststatus_t;
typedef struct {
	unsigned short dev_addr;
	unsigned short sub_addr;
	unsigned short len;         //Total length
	unsigned short burstlen;    //Burst length in bytes used for i2c burst read/write
	unsigned short burstdelay;  //Delay in ms after each burst write - needed for I2C EEPROMs
} i2c_burstpayload_t;

typedef struct {
	int len; // dual purpose for status and buf len
	unsigned char buf[MXCAM_SPI_PAYLOAD_DATA_LEN];
} spi_data_t;

typedef struct {
    unsigned int    clk;
    unsigned char   device;
    unsigned char   clk_polarity;
    unsigned char   clk_phase;
    unsigned char   chip_select;
    spi_data_t      data;
} spi_payload_t;


typedef enum {
	MILLI = 0,
	MICRO,
	NANNO
}time_unit_t;

typedef struct {
    uint32_t mode;
    uint32_t mperiod;
    uint32_t pwm0h;
    uint32_t pwm1h;
    uint32_t pwm2h;
    uint32_t period;

    uint32_t c2_pwm0h;
    uint32_t c2_pwm1h;
    uint32_t c2_pwm2h;
    uint32_t c2_period;
}pwmled_info_t;

/* API */
int mxcam_scan(struct mxcam_devlist **devlist, int fast_boot);
int mxcam_scan_oldcam(struct mxcam_devlist **devlist);
int mxcam_open_by_devnum(int dev_num, struct mxcam_devlist *devlist);
int mxcam_open_by_busaddr(int bus, int addr, struct mxcam_devlist *devlist);
int mxcam_poll_one(struct mxcam_devlist *devlist);
int mxcam_poll_new(void);
int mxcam_boot_firmware(const char *image, const char *opt_image,
	void (*callbk)(FW_STATE st, const char *filename));
int mxcam_boot_firmware_fast(const char *image, const char *opt_image,
           void (*callbk)(FW_STATE st, const char *filename));
int mxcam_upgrade_firmware(fw_info *fw,
	void (*callbk)(FW_STATE st, const char *filename),int is_rom_img, char **cur_bootmode);
int mxcam_read_nvm_pgm_status(unsigned char *status);
int mxcam_erase_eeprom_config(CONFIG_AREA area,unsigned short size);
int mxcam_set_key(const char *keyname, const char *value);
int mxcam_remove_key(const char *keyname);
int mxcam_save_eeprom_config(const char *jsonfile, uint32_t json_index);
int mxcam_get_config_size(CONFIG_AREA area,unsigned short *size_out);
int mxcam_read_eeprom_config_mem(char *buf,unsigned int len);
int mxcam_read_flash_image_header(image_header_t *header, IMG_HDR_TYPE hdr_type);
int mxcam_get_value(const char *keyname, char **value_out);
int mxcam_get_ccrvalue(const char *keyname, char **value_out);
int mxcam_get_all_ccr(CONFIG_AREA area, void (*callbk)(GET_ALL_KEY_STATE st, int keycnt, void *data));
int mxcam_free_get_value_mem(char *value_mem);
int mxcam_get_all_key_values(void);
int mxcam_reset(void);
int mxcam_reboot(uint16_t value);
void mxcam_close (void);
const char* mxcam_error_msg(const int err);
int mxcam_set_configuration(EP_CONFIG config);
int mxcam_i2c_write(uint16_t inst, uint16_t type, i2c_payload_t *payload);
int mxcam_i2c_read(uint16_t inst, uint16_t type, i2c_payload_t *payload);
int mxcam_i2c_burstwrite(uint16_t inst, uint16_t type, i2c_burstpayload_t *payload, unsigned char *buf);
int mxcam_i2c_burstread(uint16_t inst, uint16_t type, i2c_burstpayload_t *payload, unsigned char *buf);
int mxcam_spi_rw(spi_payload_t *payload);
int mxcam_tcw_write(uint32_t value);
int mxcam_tcw_read(uint32_t *value);
int mxcam_isp_write(uint32_t addr, uint32_t value);
int mxcam_isp_read(uint32_t addr, uint32_t *value);
int mxcam_isp_enable(uint32_t enable);
int mxcam_usbtest(uint32_t testmode);
int mxcam_qcc_write(uint16_t bid, uint16_t addr, uint16_t length, uint32_t value);
int mxcam_qcc_read(uint16_t bid, uint16_t addr, uint16_t length, uint32_t *value);
int mxcam_whoami(void);
int mxcam_get_cmd_bitmap(char *buffer);
int mxcam_whoru(char *buffer);
int mxcam_rw_gpio (int gpio_no, int value, int gpio_write, int *status);

int mxcam_memtest(uint32_t ddr_size);
int mxcam_get_memtest_result(uint32_t *value);
int mxcam_init_ddr(const char *image);

int mxcam_get_json_size(unsigned int *len);
int mxcam_read_pwm(uint32_t id, uint32_t *state, uint32_t *hightime, uint32_t *period, time_unit_t *unit);
int mxcam_write_pwm(uint32_t id, uint32_t state, uint32_t hightime, uint32_t period, time_unit_t unit);

int mxcam_notify_json(const char *json, const char *bin);
int mxcam_send_json(const char *json, const char *bin);
int mxcam_check_json_syntax(const char *json);
int mxcam_check_ispcfg_syntax(const char *ispcfg);
int mxcam_ispcfg_load_file(const char* file_name);
int mxcam_open_device_vid_pid(int vid, int pid);

int mxcam_check_fastboot_compatible(void);

int mxcam_write_ispcfg(const char *ispcfgfile, uint32_t isp_index);
int mxcam_get_ispcfg_size(unsigned int *len);
int mxcam_read_ispcfg(char *buf,unsigned int len);

int mxcam_write_gridmap(const char *arguments[]);

#if !defined(_WIN32)
int mxcam_xmodem_transmit(const char *dev, const char *file);
#endif

/* Deprecated API: kept for backward compatibility  */
int mxcam_register_vidpid(int vid, int pid, char *desc,SOC_TYPE soc,DEVICE_TYPE dev);
int mxcam_handle_pwmled(pwmled_info_t *led, char pwmled_write);

int mxcam_boot_from_snor(uint32_t json_index);

#endif //__LIBMXCAM_H__
