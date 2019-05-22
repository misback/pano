#ifndef __MXCAM_H__
#define __MXCAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#define NEW_JSON "/sdcard/new.json"
int mxcam_gpiorw_on_and_off(int gpio_no, int value, struct libusb_device_handle *usb_devh);
int mxcam_get_vendor_info(char *release, char *vendor_info, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_i2cwrite(const uint32_t value, struct libusb_device_handle *usb_devh);
int	mxcam_subcmd_get_serialnumber(char *serialnumber, struct libusb_device_handle *usb_devh);
int	mxcam_subcmd_readcfg(char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_writecfg(char *jsonfile, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_flash(char *firmware, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_readispcfg(char *ispcfgfile, char *lsd, char *dsc, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_writeispcfg(char *ispcfgfile, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_boot(char *image, char *json, char *bin, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_bootmode(char *bootmode, struct libusb_device_handle *usb_devh);
int mxcam_subcmd_merge(char *newfile, char *serial, char *sensorl_xstart, char *sensorl_ystart, char *sensorr_xstart, char *sensorr_ystart);
int mxcam_boot_from_snor(libusb_device_handle *usb_devh);
int mxcam_reset(libusb_device_handle *usb_devh);
#ifdef __cplusplus
}
#endif

#endif /*__MXCAM_H__*/