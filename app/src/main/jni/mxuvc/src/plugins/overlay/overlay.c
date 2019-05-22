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
#include <libusb/libusb.h>
#include <unistd.h>
#include "libusb/handle_events.h"
#include "mxuvc.h"
#include "common.h"
#include <sys/stat.h>
#include <assert.h>
static struct libusb_device_handle *camera = NULL;
static struct libusb_context *ctxt = NULL;
#define FILESIZE_CHECK 1
#define MAX_TEXT_LENGTH   	(24)
#define EP0TIMEOUT   		(0)
#define FWPACKETSIZE 		4088
#define QCC_BID_PMU         0x21      /* Partition Management Unit */
#define QCC_BID_PMC         0x22      /* Physical Memory Ctl. (SDRAM) */	
#define SLEEP_ALPHAMAP_DOWNLOAD      200000
#define POLL_ALPHAMAP_DOWNLOAD_STATUS     100000

static int qcc_write(uint16_t bid, uint16_t addr, uint16_t length, uint32_t value)
{
	int r;
	int cmd_sta;
	printf("%s (IN)\n",__func__);

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

	printf("%s (OUT)\n",__func__);

	return 0;
}


static int qcc_read(uint16_t bid, uint16_t addr, uint16_t length, uint32_t *value)
{
	int r;
	int mask;

	printf("%s (IN)\n",__func__);

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

int mxuvc_overlay_init()
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
		TRACE("overlay init done\n");
	}

	unsigned int data[4] = {0};
	// read csr registers    
	ret = qcc_read(QCC_BID_PMU, 0x08, 2, &data[0]);
	ret = qcc_read(QCC_BID_PMC, 0x14, 1, &data[1]);
	ret = qcc_read(QCC_BID_PMC, 0x15, 1, &data[2]);
	ret = qcc_read(QCC_BID_PMC, 0x16, 1, &data[3]);

	YUVUtil_Init2(data[0], data[1], data[2], data[3]);

	return ret;
}

int mxuvc_overlay_load_font(video_channel_t ch, int font_size, char* file_name)
{
	int ret=0;
	int data[2]; 

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	if(font_size != 8 && font_size != 16 && font_size != 32){
		TRACE("unsupported font size %d. Should be 8, 16 or 32.\n", font_size);
		return -1;
	}

	data[0] = ch;
	data[1] = font_size;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ ADD_FONT,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data,
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "ADD_FONT failed");
	
	ret = usb_send_file(camera, file_name, FWPACKETSIZE, SEND_FONT_FILE);

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ FONT_FILE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "FONT_FILE_DONE failed");
	usleep(100000);

	return ret;
}

int mxuvc_overlay_load_font_gpu(video_channel_t ch, overlay_font_params_t* font, char* file_name, char* alpha_channel)
{
	int data[8] = {0};
	int font_size =  0;
	int ret=0;

	int YUVformat=0;
	int startframe=0;
	int ysize = 0;
	int uvsize = 0;
	unsigned char alpha = 0xFF; // global alpha = max
	int alpha_ysize = 0;
	int alpha_uvsize = 0;
	char* alpha420p_filename=alpha_channel; // disable alpha support for now
	int width=0;
	int height=0;

	#ifdef FILESIZE_CHECK
	unsigned int yuv_filesize = 0;
	unsigned int alpha_filesize = 0;
	#endif
	
	unsigned char *y = NULL;
	unsigned char *uv = NULL;
	unsigned char *alpha_y = NULL;
	unsigned char *alpha_uv = NULL;
	
	FILE *ifile = NULL;
	FILE *afile = NULL;

	unsigned char* u420 = NULL;
	unsigned char* v420 = NULL; 
	unsigned char* u    = NULL;
	unsigned char* v    = NULL; 
	int i = 0;
	int j = 0;

	unsigned char *pAlpha444=NULL;
	unsigned char *pAlpha=NULL;

	int generate_alpha = false;

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	if (font == NULL){
		TRACE("font param is not initialised\n");
		return -1;
	}

	font_size = font->size;

	if(font_size < 8 || font_size > 32){
		TRACE("unsupported font size %d. Should be in range 8-32.\n", font_size);
		return -1;
	}

 	width=font->imgwidth;
 	height=font->imgheight;
 	//dont trust the height from user, calculate on your own as height may be padded.
 	//rows = num of chars/ chars per row
 	//height = rows*fontsize
 	
 	// int newheight=(((font->end - font->start)/font->width)/font->size)*font->size;
 	int newheight=height;
 	printf("new height %d\n", newheight);
	data[0] = ch;
	data[1] = font_size;
	data[2] = font->start;
	data[3] = font->end;
	data[4] = font->width;
	data[5] = width;
	data[6] = newheight;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_FONT_INIT,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data,
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "OVERLAY_FONT_INIT failed");
	
	// ret = usb_send_file(camera, file_name, FWPACKETSIZE, SEND_FONT_FILE);

	char *yuv420p_filename = file_name;

	ifile = fopen(yuv420p_filename, "r");
	if (ifile == NULL)
	{
		fprintf(stderr, "Could not open %s for reading\n", yuv420p_filename);
		return -1;
	}
	else
	{
		#ifdef FILESIZE_CHECK
		fseek(ifile, 0, SEEK_END);
		yuv_filesize = ftell(ifile);
		fseek(ifile, 0, SEEK_SET);
		TRACE("YUV File Size %d bytes\n", yuv_filesize);
		#endif
	}

	if (alpha420p_filename == NULL) {
		TRACE("No alpha masked detected, using global alpha %0x to apply globally\n", alpha);
		generate_alpha = true;
	}	
	else
	{
		afile = fopen(alpha420p_filename, "r");
		if (afile == NULL)
		{
			fprintf(stderr, "Could not open %s for reading\n", alpha420p_filename);
			generate_alpha = true;
		}
		else	
		{
			TRACE("per pixel alpha masked detected\n");
			#ifdef FILESIZE_CHECK
			fseek(afile, 0, SEEK_END);
			alpha_filesize = ftell(afile);
			fseek(afile, 0, SEEK_SET);
			TRACE("Alpha File Size %d bytes\n", alpha_filesize);
			#endif

		}
	}
	
	#ifdef FILESIZE_CHECK
	if(!generate_alpha)
	{
		if(yuv_filesize != alpha_filesize)
		{
			fprintf(stderr, "YUV and Alpha File Size DONOT match!!\n");
			if(ifile)
				fclose(ifile);
			if(afile)
				fclose(afile);
			return -1;
		}
	}
	#endif

	if (width%4 != 0)
	{
		fprintf(stderr, "Picture width must be multiple of 4\n");
		return -1;
	}

	if (height%4 != 0)
	{
		fprintf(stderr, "Picture height must be multiple of 4\n");
		return -1;
	}

	int size = width*height;
	TRACE("FONT: Width %d, Height%d .. SZ %d\n", width, height, size*3/2);

	#ifdef FILESIZE_CHECK
	if(yuv_filesize != (unsigned int)width*height*3/2)
	{
		fprintf(stderr, "YUV file size %d does not match expected size %d for resolution %dx%d\n", yuv_filesize, size*3/2, width, height);
		if(ifile)
			fclose(ifile);

		if(afile)
			fclose(afile);
		return -1;
	}	
	
	if(!generate_alpha && (alpha_filesize != (unsigned int)width*height*3/2))
	{
		fprintf(stderr, "Alpha file size %d does not match expected size %d for resolution %dx%d\n", yuv_filesize, size*3/2, width, height);
		if(ifile)
			fclose(ifile);
		
		if(afile)
			fclose(afile);
		return -1;
	}
	#endif

    //convert yuv to tile here.
	WriteVideoFrame(ifile, width, newheight, width, &y, &uv, &ysize, &uvsize, YUVformat, startframe );
	fclose(ifile);

	if(ysize != 0 && y != NULL) {
		ret = usb_send_buffer(camera, y, ysize, FWPACKETSIZE, OVERLAY_FONT_Y_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_FONT_Y_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/     0 
			);
		CHECK_ERROR(ret < 0, -1, "OVERLAY_FONT_Y_UPDATE_DONE failed");
	}

	free(y);
	y=NULL;

	if(uvsize != 0 && uv != NULL) {	
		ret = usb_send_buffer(camera, uv, uvsize, FWPACKETSIZE, OVERLAY_FONT_UV_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_FONT_UV_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/ 	  0 
			);
		CHECK_ERROR(ret < 0, -1, "OVERLAY_FONT_UV_UPDATE_DONE failed");
	}

	free(uv);
	uv=NULL;
	
	printf("ysize %d, uvsize %d\n", ysize, uvsize);

	size = width*newheight;
	//convert alpha to tile here.
	if(generate_alpha == false)
	{
		pAlpha444 = (unsigned char*)malloc(size*3);
		pAlpha = (unsigned char*)malloc(size*3/2);

		unsigned char *pTempAlpha    = (unsigned char*)malloc(size);

		fread(pTempAlpha, 1, size, afile); // read Y 
		
		memcpy(&pAlpha444[0], pTempAlpha, size); //Y
		memcpy(&pAlpha444[size], pTempAlpha, size); //U 
		memcpy(&pAlpha444[size*2], pTempAlpha, size); //V

		free(pTempAlpha);
		pTempAlpha=NULL;

		// convert alpha 444 to alpha 420
		// copy Y
		memcpy(pAlpha, pAlpha444, size);

        u420 = &pAlpha[size];
        v420 = &pAlpha[(size) + (size/4)]; 
        u    = &pAlpha444[size];
        v    = &pAlpha444[2*size]; 

        i = 0;
        j = 0;
        for (j=0; j<size; j++) 
        {  
            if((j%2 ==0 ) && ((j%(width*2)) < width)) { 
                u420[i]=(u[j]+u[j+width])/2; 
                v420[i]=(v[j]+v[j+width])/2; 
                i++;  
            }  
        }

		free(pAlpha444);
		pAlpha444 = NULL;

		// WriteVideoFrame(afile, params->width, params->height, params->width, &alpha_y, &alpha_uv, &alpha_ysize, &alpha_uvsize, YUVformat, startframe );
		// printf("alpha_ysize %d, alpha_uvsize %d\n", alpha_ysize, alpha_uvsize);
		fclose(afile);
		
	}
	else
	{
		pAlpha444 = (unsigned char*)malloc(size*3);
		if(!pAlpha444)
		{
			printf("pAlpha444: Could not allocate %d bytes\n", size*3);
			return -1;
		}

		pAlpha    = (unsigned char*)malloc(size*3/2);
		if(!pAlpha)
		{
			printf("matte_alpha_y: Could not allocate %d bytes\n", size*3/2);
			free(pAlpha444);
			return -1;
		}

		memset(pAlpha444, 0xFF, size*3);
		memset(pAlpha, 0xFF, size*3/2);


		// unsigned char* u420 = NULL;
		// unsigned char* v420 = NULL; 
		// unsigned char* u    = NULL;
		// unsigned char* v    = NULL; 
		// int i = 0;
		// int j = 0;
		// convert alpha 444 to alpha 420
        // copy Y
        // memcpy(&pAlpha[0], &pAlpha444[0], size);

        // u420 = &pAlpha[size];
        // v420 = &pAlpha[(size) + (size/4)]; 

        // u    = &pAlpha444[size];
        // v    = &pAlpha444[2*size]; 

        // i = 0;
        // j = 0;
        // for (j=0; j<size; j++) 
        // {  
        //     if((j%2 ==0 ) && ((j%(params->width*2)) < params->width)) { 
        //         u420[i]=(u[j]+u[j+params->width])/2; 
        //         v420[i]=(v[j]+v[j+params->width])/2; 
        //         i++;  
        //     }  
        // }

		free(pAlpha444);
		pAlpha444 = NULL;

		
	}

	alpha_y  = (unsigned char*)malloc(ysize);
	if(!alpha_y)
	{
		printf("matte_alpha_y: Could not allocate %d bytes\n", ysize);
		free(pAlpha);
		return -1;
	}

	alpha_uv = (unsigned char*)malloc(uvsize);
	if(!alpha_uv)
	{
		printf("matte_alpha_uv: Could not allocate %d bytes\n", uvsize);
		free(alpha_y);
		free(pAlpha);
		return -1;
	}

	ConvertFrameToTile(pAlpha, width, newheight, width, &alpha_y, &alpha_uv, &alpha_ysize, &alpha_uvsize, YUVformat, startframe );
	
	free(pAlpha);
	pAlpha = NULL;
	
	if(alpha_ysize != 0 && alpha_y != NULL) {
		ret = usb_send_buffer(camera, alpha_y, alpha_ysize, FWPACKETSIZE, OVERLAY_FONT_ALPHA_Y_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_FONT_ALPHA_Y_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/     0 
			);
		CHECK_ERROR(ret < 0, -1, "OVERLAY_FONT_ALPHA_Y_UPDATE_DONE failed");
	}

	free(alpha_y);
	alpha_y=NULL;

	if(alpha_uvsize != 0 && alpha_uv != NULL) {	
		ret = usb_send_buffer(camera, alpha_uv, alpha_uvsize, FWPACKETSIZE, OVERLAY_FONT_ALPHA_UV_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_FONT_ALPHA_UV_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/ 	  0 
			);
		CHECK_ERROR(ret < 0, -1, "OVERLAY_FONT_ALPHA_UV_UPDATE_DONE failed");
	}

	free(alpha_uv);
	alpha_uv=NULL;

	//free up all the memory
	if(y)
		free(y);

	if(uv)
		free(uv);

	if(alpha_y)
		free(alpha_y);

	if(alpha_uv)
		free(alpha_uv);
	
	ret = libusb_control_transfer(camera,
			/* bmRequestType */
	(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
		LIBUSB_RECIPIENT_INTERFACE),
			/* bRequest      */ OVERLAY_FONT_FILE_DONE,
			/* wValue        */ 0,
			/* MSB 4 bytes   */
			/* wIndex        */ 0,
			/* Data          */ NULL,
			/* wLength       */ 0,
			/* timeout*/     0 
	);
	CHECK_ERROR(ret < 0, -1, "OVERLAY_FONT_FILE_DONE failed");
	usleep(100000);
	return ret;
}

int mxuvc_overlay_set_transparency(video_channel_t ch, uint32_t idx, uint8_t alpha)
{
	int ret = 0;
	int data[3]; 
	data[0] = (int)ch;
	data[1] = (int)idx;
	data[2] = alpha;

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_ALPHA,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data,
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "LOGO_CMD_ALPHA failed");
	return ret;
}

int mxuvc_overlay_set_colorkey(video_channel_t ch, uint32_t idx, uint32_t colorkey)
{
	int ret = 0;
	int data[3]; 
	data[0] = (int)ch;
	data[1] = (int)idx;
	data[2] = colorkey;

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}
	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_COLORKEY,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data,
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "LOGO_CMD_COLORKEY failed");
	return ret;
}

int mxuvc_overlay_remove_image(video_channel_t ch, uint32_t idx)
{

	int ret = 0;
	int data[2]; 
	
	if (idx >= NUM_OVERLAY_IMAGE_IDX){
		TRACE("incorrect internal index %d\n", idx);
		return -1;
	}

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	
	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	data[0] = (int)ch;
	data[1] = idx;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_DEINIT,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "LOGO_CMD_DEINIT failed");
	// wait for cmd to execute at fw
	usleep(500000);
	return ret;

}

int mxuvc_overlay_add_image(video_channel_t ch, overlay_image_params_t* params, char* yuv420p_filename, char* alpha420p_filename)
{
	int ret=0;
	int YUVformat=0;
	int startframe=0;
	int ysize = 0;
	int uvsize = 0;
	int alpha_ysize = 0;
	int alpha_uvsize = 0;

	#ifdef FILESIZE_CHECK
	unsigned int yuv_filesize = 0;
	unsigned int alpha_filesize = 0;
	#endif
	
	unsigned char *y = NULL;
	unsigned char *uv = NULL;
	unsigned char *alpha_y = NULL;
	unsigned char *alpha_uv = NULL;
	
	FILE *ifile = NULL;
	FILE *afile = NULL;

	unsigned char* u420 = NULL;
	unsigned char* v420 = NULL; 
	unsigned char* u    = NULL;
	unsigned char* v    = NULL; 
	int i = 0;
	int j = 0;

	unsigned char *pAlpha444=NULL;
	unsigned char *pAlpha=NULL;

	int pp_alpha = 0;
	int generate_alpha = false;

	int data[8];

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL) {
		TRACE("Unitialised camera handle\n");
		return -1;
	}

	if (params == NULL) {
		TRACE("params cannot be NULL\n");
		return -1;
	}

	if (yuv420p_filename == NULL) {
		TRACE("yuv420p_filename cannot be NULL\n");
		return -1;
	}	

	ifile = fopen(yuv420p_filename, "r");
	if (ifile == NULL)
	{
		fprintf(stderr, "Could not open %s for reading\n", yuv420p_filename);
		return -1;
	}
	else
	{
		#ifdef FILESIZE_CHECK
		fseek(ifile, 0, SEEK_END);
		yuv_filesize = ftell(ifile);
		fseek(ifile, 0, SEEK_SET);
		TRACE("YUV File Size %d bytes\n", yuv_filesize);
		#endif
	}


	if (alpha420p_filename == NULL) {
		TRACE("No alpha masked detected, using params->alpha %0x to apply globally\n", params->alpha);
		// generate_alpha = true;
	}	
	else
	{
		afile = fopen(alpha420p_filename, "r");
		if (afile == NULL)
		{
			fprintf(stderr, "Could not open %s for reading\n", alpha420p_filename);
			// generate_alpha = true;
		}
		else	
		{
			TRACE("per pixel alpha masked detected\n");
			#ifdef FILESIZE_CHECK
			fseek(afile, 0, SEEK_END);
			alpha_filesize = ftell(afile);
			fseek(afile, 0, SEEK_SET);
			TRACE("Alpha File Size %d bytes\n", alpha_filesize);
			#endif
			
			pp_alpha = 1;

		}
	}
	
	#if 0
	if(!generate_alpha)
	{
		if(yuv_filesize != alpha_filesize)
		{
			fprintf(stderr, "YUV and Alpha File Size DONOT match!!\n");
			if(ifile)
				fclose(ifile);
			if(afile)
				fclose(afile);
			return -1;
		}
	}
	#endif

	if (params->idx >= NUM_OVERLAY_IMAGE_IDX)
	{
		fprintf(stderr, "idx exceeds maximum supported value\n");
		return -1;
	}

	#if 0
	if (params->width > 640)
	{
		fprintf(stderr, "Maximum picture width supported is 640\n");
		return -1;
	}

	if (params->height > 480)
	{
		fprintf(stderr, "Maximum picture height supported is 480\n");
		return -1;
	}	
	#endif

	if (params->width%4 != 0)
	{
		fprintf(stderr, "Picture width must be multiple of 4\n");
		return -1;
	}

	if (params->height%4 != 0)
	{
		fprintf(stderr, "Picture height must be multiple of 4\n");
		return -1;
	}

	TRACE("Width %d, Height %d, X %d, Y %d\n", params->width, params->height, params->xoff, params->yoff);

	#ifdef FILESIZE_CHECK
	if(yuv_filesize != params->width*params->height*3/2)
	{
		fprintf(stderr, "YUV file size %d does not match expected size %d for resolution %dx%d\n", yuv_filesize, params->width*params->height*3/2, params->width, params->height);
		if(ifile)
			fclose(ifile);

		if(afile)
			fclose(afile);
		return -1;
	}	
	#endif

	#if 0
	if(!generate_alpha && (alpha_filesize != params->width*params->height*3/2))
	{
		fprintf(stderr, "Alpha file size %d does not match expected size %d for resolution %dx%d\n", yuv_filesize, params->width*params->height*3/2, params->width, params->height);
		if(ifile)
			fclose(ifile);
		
		if(afile)
			fclose(afile);
		return -1;
	}
	#endif

	data[0] = (int)ch;
	data[1] = params->idx;

	data[2] = params->xoff;
	data[3] = params->yoff;

	data[4] = params->width;
	data[5] = params->height;
	data[6] = params->alpha;
	data[7] = pp_alpha;
	
	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_INIT,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "LOGO_INIT failed");

	const int size = params->width*params->height;

    //convert yuv to tile here.
	WriteVideoFrame(ifile, params->width, params->height, params->width, &y, &uv, &ysize, &uvsize, YUVformat, startframe );
	fclose(ifile);

	if(ysize != 0 && y != NULL) {
		ret = usb_send_buffer(camera, y, ysize, FWPACKETSIZE, LOGO_CMD_Y_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_Y_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/     0 
			);
		CHECK_ERROR(ret < 0, -1, "LOGO_LUMA_UPDATE_DONE failed");
	}

	free(y);
	y=NULL;

	if(uvsize != 0 && uv != NULL) {	
		ret = usb_send_buffer(camera, uv, uvsize, FWPACKETSIZE, LOGO_CMD_UV_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_UV_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/ 	  0 
			);
		CHECK_ERROR(ret < 0, -1, "LOGO_UV_UPDATE_DONE failed");
	}

	free(uv);
	uv=NULL;
	
	printf("ysize %d, uvsize %d\n", ysize, uvsize);

	//convert alpha to tile here.
	if(pp_alpha != 0)
	{
		pAlpha444 = (unsigned char*)malloc(size*3);
		pAlpha = (unsigned char*)malloc(size*3/2);

		unsigned char *pTempAlpha    = (unsigned char*)malloc(size);

		fread(pTempAlpha, 1, size, afile); // read Y 
		
		memcpy(&pAlpha444[0], pTempAlpha, size); //Y
		memcpy(&pAlpha444[size], pTempAlpha, size); //U 
		memcpy(&pAlpha444[size*2], pTempAlpha, size); //V

		free(pTempAlpha);
		pTempAlpha=NULL;

		// convert alpha 444 to alpha 420
		// copy Y
		memcpy(pAlpha, pAlpha444, size);

        u420 = &pAlpha[size];
        v420 = &pAlpha[(size) + (size/4)]; 
        u    = &pAlpha444[size];
        v    = &pAlpha444[2*size]; 

        i = 0;
        j = 0;
        for (j=0; j<size; j++) 
        {  
            if((j%2 ==0 ) && ((j%(params->width*2)) < params->width)) { 
                u420[i]=(u[j]+u[j+params->width])/2; 
                v420[i]=(v[j]+v[j+params->width])/2; 
                i++;  
            }  
        }

		free(pAlpha444);
		pAlpha444 = NULL;

		// WriteVideoFrame(afile, params->width, params->height, params->width, &alpha_y, &alpha_uv, &alpha_ysize, &alpha_uvsize, YUVformat, startframe );
		// printf("alpha_ysize %d, alpha_uvsize %d\n", alpha_ysize, alpha_uvsize);
		fclose(afile);
		
	}
	else if(generate_alpha)
	{
		pAlpha444 = (unsigned char*)malloc(size*3);
		if(!pAlpha444)
		{
			printf("pAlpha444: Could not allocate %d bytes\n", size*3);
			return -1;
		}

		pAlpha    = (unsigned char*)malloc(size*3/2);
		if(!pAlpha)
		{
			printf("matte_alpha_y: Could not allocate %d bytes\n", size*3/2);
			free(pAlpha444);
			return -1;
		}

		memset(pAlpha444, 0xFF, size*3);
		memset(pAlpha, 0xFF, size*3/2);


		// unsigned char* u420 = NULL;
		// unsigned char* v420 = NULL; 
		// unsigned char* u    = NULL;
		// unsigned char* v    = NULL; 
		// int i = 0;
		// int j = 0;
		// convert alpha 444 to alpha 420
        // copy Y
        // memcpy(&pAlpha[0], &pAlpha444[0], size);

        // u420 = &pAlpha[size];
        // v420 = &pAlpha[(size) + (size/4)]; 

        // u    = &pAlpha444[size];
        // v    = &pAlpha444[2*size]; 

        // i = 0;
        // j = 0;
        // for (j=0; j<size; j++) 
        // {  
        //     if((j%2 ==0 ) && ((j%(params->width*2)) < params->width)) { 
        //         u420[i]=(u[j]+u[j+params->width])/2; 
        //         v420[i]=(v[j]+v[j+params->width])/2; 
        //         i++;  
        //     }  
        // }

		free(pAlpha444);
		pAlpha444 = NULL;
	}

	if(pp_alpha || generate_alpha)
	{
		alpha_y  = (unsigned char*)malloc(ysize);
		if(!alpha_y)
		{
			printf("matte_alpha_y: Could not allocate %d bytes\n", ysize);
			free(pAlpha);
			return -1;
		}

		alpha_uv = (unsigned char*)malloc(uvsize);
		if(!alpha_uv)
		{
			printf("matte_alpha_uv: Could not allocate %d bytes\n", uvsize);
			free(alpha_y);
			free(pAlpha);
			return -1;
		}

		ConvertFrameToTile(pAlpha, params->width, params->height, params->width, &alpha_y, &alpha_uv, &alpha_ysize, &alpha_uvsize, YUVformat, startframe );
	}
	
	if(pAlpha)
		free(pAlpha);
		
	if(alpha_ysize != 0 && alpha_y != NULL) {
		ret = usb_send_buffer(camera, alpha_y, alpha_ysize, FWPACKETSIZE, LOGO_CMD_ALPHA_Y_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_ALPHA_Y_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/     0 
			);
		CHECK_ERROR(ret < 0, -1, "LOGO_ALPHA_Y_UPDATE_DONE failed");
	}

	if(alpha_y)
		free(alpha_y);

	if(alpha_uvsize != 0 && alpha_uv != NULL) {	
		ret = usb_send_buffer(camera, alpha_uv, alpha_uvsize, FWPACKETSIZE, LOGO_CMD_ALPHA_UV_UPDATE);

		ret = libusb_control_transfer(camera,
				/* bmRequestType */
			(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ LOGO_CMD_ALPHA_UV_UPDATE_DONE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/ 	  0 
			);
		CHECK_ERROR(ret < 0, -1, "LOGO_MATTE_ALPHA_UV_UPDATE_DONE failed");
	}

	if(alpha_uv)
		free(alpha_uv);

	//free up all the memory
	if(y)
		free(y);

	if(uv)
		free(uv);

	// wait for cmd to execute at fw
	usleep(500000);
	return ret;
}

int mxuvc_overlay_deinit(void){
	int ret = 0;

	if(camera){
		libusb_close (camera);
		exit_libusb(&ctxt);

		ctxt = NULL;
		camera = NULL;
	}

	return ret;
}

int mxuvc_download_compressed_alphamap(video_channel_t ch, char* compressedalpha_filename)
{
    int ret=0;
    int alpha_filesize = 0;

    FILE *afile = NULL;
    int data[8];

    data[0] = (int)ch;

    afile = fopen(compressedalpha_filename, "rb");
    if (afile == NULL)
    {
        fprintf(stderr, "Could not open %s for reading\n", compressedalpha_filename);
    }
    else
    {
        TRACE("compressed alpha detected\n");
        fseek(afile, 0, SEEK_END);
        alpha_filesize = ftell(afile);
        fseek(afile, 0, SEEK_SET);
        TRACE("Compressed Alpha File Size %d bytes\n", alpha_filesize);
    }
    if ( afile != NULL )
        fclose(afile);

    data[1] = alpha_filesize;

    ret = libusb_control_transfer(camera,
                /* bmRequestType */
        (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ COMPRESSED_ALPHA_CMD_INIT,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&data[0],
                /* wLength       */ sizeof(data),
                /* timeout*/     0
        );
    
    CHECK_ERROR(ret < 0, -1, "COMPRESSED_ALPHA_INIT failed");

    if(alpha_filesize != 0 && compressedalpha_filename != NULL) {

        ret = usb_send_file(camera, compressedalpha_filename, FWPACKETSIZE, COMPRESSED_ALPHA_CMD_UPDATE);
        CHECK_ERROR(ret < 0, -1, "COMPRESSED_ALPHA_CMD_UPDATE failed");

        // making sure the firmware gets time to allocate memory to copy file sent to firmware
        usleep(SLEEP_ALPHAMAP_DOWNLOAD);

        ret = libusb_control_transfer(camera,
                /* bmRequestType */
            (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                LIBUSB_RECIPIENT_INTERFACE),
                /* bRequest      */ COMPRESSED_ALPHA_CMD_UPDATE_DONE,
                /* wValue        */ 0,
                /* MSB 4 bytes   */
                /* wIndex        */ 0,
                /* Data          */ (unsigned char *)&data[0],
                /* wLength       */ sizeof(data),
                /* timeout*/      0
            );
        CHECK_ERROR(ret < 0, -1, "COMPRESSED_ALPHA_UPDATE_DONE failed");
    }else
    {
        fprintf(stderr, "Could not allocate memory or find compresse alpha file\n");
    }

	int i;
	ret = -1;
	compressed_alpha_errorcode_t status = COMPRESSED_ALPHA_STATUS_BUSY;

	// as mask application could take 1-2 secs depending on size and offsets used
	// we poll for the status here so api remains block.
	for(i=0; i<50; i++)
	{
		// lower the value, more we flood the condor fw with dsr interrupts. this is nominal value for most of the cases.
		usleep(POLL_ALPHAMAP_DOWNLOAD_STATUS);

	 	libusb_control_transfer(camera,
			/* bmRequestType */
		(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
			/* bRequest      */ COMPRESSED_ALPHA_CMD_STATUS,
			/* wValue        */ 0,
			/* wIndex        */ 0,
			/* Data          */ (unsigned char*)&status,
			/* wLength       */ sizeof(int),
			/* timeout       */ EP0TIMEOUT
		);
	
		// success
		if((compressed_alpha_errorcode_t)status == COMPRESSED_ALPHA_SUCCESS)
		{
			ret = 0;
			break;
		} //failure, we need this to quit the loop faster.
		else if ((compressed_alpha_errorcode_t)status > COMPRESSED_ALPHA_STATUS_BUSY)
		{
			ret = -1;
			break;
		}
	}
    printf("status of compressed alpha download %d\n",status);

	CHECK_ERROR(ret < 0, -1, "sending COMPRESSED ALPHA map failed");

	return ret;
}

int mxuvc_overlay_add_text(video_channel_t ch, overlay_text_params_t* params, char *str, uint32_t length)
{
	int ret = 0;
	uint32_t position = 0;
	uint32_t lindex = 0;
	printf("Add text at channel %d\n", ch);

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	if (params == NULL){
		TRACE("params cannot be NULL\n");
		return -1;
	}

	if (params->idx >= NUM_OVERLAY_TEXT_IDX){
		TRACE("params->idx is incorrect\n");
		return -1;
	}

	if(str == NULL || length<= 0){
		printf("ERR: %s str/length is invalid\n",__func__);
		return -1;
	}

	if(length > MAX_TEXT_LENGTH){
		printf("ERR: %d max text length exceeded \n", MAX_TEXT_LENGTH);
		return -1;
	        //printf("continue send the text len (%d) to codec\n", length);
	}

	position = (params->xoff << 16) + params->yoff;
	lindex = params->idx;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ ADD_TEXT,
				/* wValue        */ (int)ch,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&position,
				/* wLength       */ sizeof(position),
				/* timeout*/   0 
		);
	CHECK_ERROR(ret < 0, -1, "ADD_TEXT failed");
	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ TEXT,
				/* wValue        */ lindex,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)str,
				/* wLength       */ length,
				/* timeout*/     0 
		);

	CHECK_ERROR(ret < 0, -1, "TEXT failed");
	usleep(200000);
	return ret;
}

int mxuvc_overlay_remove_text(video_channel_t ch, uint32_t idx)
{
	int ret = 0;
	int data[2];
	
	printf("Remove text for ch %d at index 0x%x\n", ch, idx);
	
	if (idx >= NUM_OVERLAY_TEXT_IDX){
		printf("incorrect index %d\n", idx);
		return -1;
	}

	if (camera == NULL){
		printf("camera handle is not initialised\n");
		return -1;
	}

	data[0] = (int)ch; // channel #
	data[1] = idx;       // burnin index

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ REMOVE_TEXT,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/   0 
		);

	CHECK_ERROR(ret < 0, -1, "REMOVE_TEXT failed");
	usleep(200000);
	return ret;	
}

int mxuvc_overlay_show_time(video_channel_t ch)
{
	int ret = 0;
	int data[4]; 
	data[0] = ch;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;

	if(camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}
	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ SHOW_TIME,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "SHOW_TIME failed");

	usleep(200000);
	return ret;
}

int mxuvc_overlay_hide_time(video_channel_t ch)
{
	int ret = 0;
	int data[2]; 
	data[0] = ch;
	data[1] = 0;

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}
	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ HIDE_TIME,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/   0 
		);
	CHECK_ERROR(ret < 0, -1, "HIDE_TIME failed");

	usleep(200000);
	return ret;
}

int mxuvc_overlay_set_time(video_channel_t ch, overlay_text_params_t* params, overlay_time_t* otime)
{
	int ret = 0;
	int data[7];

	if (params == NULL){
		TRACE("params cannot be NULL\n");
		return -1;
	}

	if (otime == NULL){
		TRACE("hhmmss cannot be NULL\n");
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	data[0] = (int)ch;
	data[1] = otime->hh;
	data[2] = otime->mm;
	data[3] = otime->ss;
	data[4] = otime->frame_num_enable;

	data[5] = params->xoff;
	data[6] = params->yoff;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ UPDATE_TIME,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	
	CHECK_ERROR(ret < 0, -1, "UPDATE_TIME failed");

	usleep(200000);
	return ret;	
}

int mxuvc_overlay_add_text_gpu(video_channel_t ch, overlay_text_params_t* params, char *str, uint32_t length)
{
	int ret = 0;
	uint32_t position = 0;
	uint32_t lindex = 0;
	printf("Add text at channel %d\n", ch);

	if(ch >= NUM_MUX_VID_CHANNELS){
		TRACE("ch should be less than value %d\n", NUM_MUX_VID_CHANNELS);
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	if (params == NULL){
		TRACE("params cannot be NULL\n");
		return -1;
	}

	if (params->idx >= NUM_OVERLAY_TEXT_IDX){
		TRACE("params->idx is incorrect\n");
		return -1;
	}

	if(str == NULL || length<= 0){
		printf("ERR: %s str/length is invalid\n",__func__);
		return -1;
	}

	if(length > MAX_TEXT_LENGTH){
		printf("ERR: %d max text length exceeded \n", MAX_TEXT_LENGTH);
		return -1;
	        //printf("continue send the text len (%d) to codec\n", length);
	}

	position = (params->xoff << 16) + params->yoff;
	lindex = params->idx;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_ADD_TEXT,
				/* wValue        */ (int)ch,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&position,
				/* wLength       */ sizeof(position),
				/* timeout*/   0 
		);
	CHECK_ERROR(ret < 0, -1, "OVERLAY_ADD_TEXT failed");
	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_TEXT,
				/* wValue        */ lindex,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)str,
				/* wLength       */ length,
				/* timeout*/     0 
		);

	CHECK_ERROR(ret < 0, -1, "OVERLAY_TEXT failed");
	usleep(500000);
	return ret;
}

int mxuvc_overlay_remove_text_gpu(video_channel_t ch, uint32_t idx)
{
	int ret = 0;
	int data[2];
	
	printf("Remove text for ch %d at index 0x%x\n", ch, idx);
	
	if (idx >= NUM_OVERLAY_TEXT_IDX){
		printf("incorrect index %d\n", idx);
		return -1;
	}

	if (camera == NULL){
		printf("camera handle is not initialised\n");
		return -1;
	}

	data[0] = (int)ch; 	 // channel #
	data[1] = idx;       // burnin index

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_REMOVE_TEXT,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/   0 
		);

	CHECK_ERROR(ret < 0, -1, "OVERLAY_REMOVE_TEXT failed");
	usleep(200000);
	return ret;	
}

int mxuvc_overlay_show_time_gpu(video_channel_t ch)
{
	int ret = 0;
	int data[4]; 
	data[0] = ch;
	data[1] = 5; //index
	data[2] = 0; 
	data[3] = 0;

	if(camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	printf("Show time for ch %d\n", ch);

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_SHOW_TIME,
				/* wValue        */ 8,
				/* MSB 4 bytes   */
				/* wIndex        */ 5,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "OVERLAY_SHOW_TIME failed");

	usleep(500000);
	return ret;
}

int mxuvc_overlay_hide_time_gpu(video_channel_t ch)
{
	int ret = 0;
	int data[2]; 
	data[0] = ch;
	data[1] = 0;

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	printf("Hide time for ch %d\n", ch);

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_HIDE_TIME,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/   0 
		);
	CHECK_ERROR(ret < 0, -1, "OVERLAY_HIDE_TIME failed");

	usleep(500000);
	return ret;
}

int mxuvc_overlay_set_time_gpu(video_channel_t ch, overlay_text_params_t* params, overlay_time_t* otime)
{
	int ret = 0;
	int data[8];

	if (params == NULL){
		TRACE("params cannot be NULL\n");
		return -1;
	}

	if (otime == NULL){
		TRACE("hhmmss cannot be NULL\n");
		return -1;
	}

	if (camera == NULL){
		TRACE("camera handle is not initialised\n");
		return -1;
	}

	data[0] = (int)ch;
	data[1] = otime->hh;
	data[2] = otime->mm;
	data[3] = otime->ss;
	data[4] = otime->frame_num_enable;

	data[5] = params->xoff;
	data[6] = params->yoff;

	if(otime->frame_num_enable == 0)
		data[7] = 8;
	else
		data[7] = 11;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ OVERLAY_UPDATE_TIME,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	
	CHECK_ERROR(ret < 0, -1, "OVERLAY_UPDATE_TIME failed");

	usleep(500000);
	return ret;	
}

int mxuvc_overlay_privacy_add_mask(video_channel_t ch, privacy_params_t* params, uint32_t idx)
{
	int *data = NULL;
	int size = 0;
	int ret = -1;
	privacy_color_t color;

	CHECK_ERROR(params == NULL, -1, "params cannot be NULL");
	CHECK_ERROR(params->shape == NULL, -1, "params->shape cannot be NULL");
	CHECK_ERROR(ch >= NUM_MUX_VID_CHANNELS, -1, "invalid ch");
	CHECK_ERROR(camera == NULL, -1, "camera handle uninitialized");
	CHECK_ERROR(idx >= PRIVACY_IDX_MAX, -1, "invalid mask index");

	size = sizeof(int)*3;

	if(params->color == NULL){
		color.yuva = 0x008080FF; //black + opaque.
	}
	else
	{
		color = *params->color;
	}

	switch(params->type)
	{
		case PRIVACY_SHAPE_RECT:
		{
			size += sizeof(privacy_mask_shape_rect_t);
			data = (int*)malloc(size);
			if(data)
			{
				data[0] = (int)ch;
				data[1] = idx;
				data[2] = color.yuva;
				memcpy((char*)&data[3], (char*)params->shape, sizeof(privacy_mask_shape_rect_t));
			}
			else
				return ret;
		}
		break;

		case PRIVACY_SHAPE_POLYGON:
		{

			privacy_mask_shape_polygon_t p = *((privacy_mask_shape_polygon_t*)params->shape);
			
			privacy_mask_point_t rect_points[4];

			if(p.num_points != sizeof(rect_points)/sizeof(privacy_mask_point_t)){
				TRACE("num points must be %d\n", (int) (sizeof(rect_points)/sizeof(privacy_mask_point_t)));
				return ret;
			}
			unsigned int 	i = 0;
			memcpy((char*)rect_points, (char*)p.points, sizeof(rect_points));

			for (i = 0; i < sizeof(rect_points)/sizeof(privacy_mask_point_t); i++)
			{
				if((rect_points[i].x < 0)|| (rect_points[i].y < 0))
				{
					TRACE("negative data points found!!\n");
					return ret;
				}

			}

			if((rect_points[3].x - rect_points[0].x) != (rect_points[2].x - rect_points[1].x)) {
				TRACE("incorrect data points\n");
				return ret;
			}	
			

			if((rect_points[1].y - rect_points[0].y) != (rect_points[2].y - rect_points[3].y)) {
				TRACE("incorrect data points\n");
				return ret;
			}	

			if(rect_points[0].x != rect_points[3].x ) {
				TRACE("incorrect data points x0 != x3\n");
				return ret;
			}
			
			if(rect_points[1].x != rect_points[2].x ) {
				TRACE("incorrect data points x1 != x2\n");
				return ret;
			}

					
			if(rect_points[0].y != rect_points[1].y ) {
				TRACE("incorrect data points y0 != y1\n");
				return ret;
			}


			if(rect_points[2].y != rect_points[3].y ) {
				TRACE("incorrect data points y2 != y3\n");
				return ret;
			}

			size += sizeof(privacy_mask_shape_rect_t);
			data = (int*)malloc(size);
			if(data)
			{
				data[0] = (int)ch;
				data[1] = idx;
				data[2] = color.yuva;

				privacy_mask_shape_rect_t rect;
				rect.xoff = rect_points[0].x;
				rect.yoff = rect_points[0].y;
				rect.width = rect_points[1].x - rect_points[0].x;
				rect.height = rect_points[3].y - rect_points[0].y;
				printf("rect: x %d, y %d, w %d, h %d, colorkey 0x%x\n", rect.xoff, rect.yoff, rect.width, rect.height, color.yuva);
				memcpy((char*)&data[3], (char*)&rect, sizeof(privacy_mask_shape_rect_t));
			}
			else
				return ret;
		}
		break;
		case PRIVACY_SHAPE_MAX:
		default:
		TRACE("shape type not supported\n");
		return ret;

	}

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ ADDMASK,
				/* wValue        */ params->type,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)data,
				/* wLength       */ size,
				/* timeout*/     0 
		);
	
	if(data)
		free(data);
	
	int i;
	ret = -1;
	unsigned int status = 0;

	// as mask application could take 1-2 secs depending on size and offsets used
	// we poll for the status here so api remains block.
	for(i=0; i<20; i++)
	{
		// lower the value, more we flood the condor fw with dsr interrupts. this is nominal value for most of the cases.
		usleep(500000);

	 	libusb_control_transfer(camera,
			/* bmRequestType */
		(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
			/* bRequest      */ MASK_STATUS,
			/* wValue        */ 0,
			/* wIndex        */ 0,
			/* Data          */ (unsigned char*)&status,
			/* wLength       */ sizeof(int),
			/* timeout       */ EP0TIMEOUT
		);
	
		TRACE("MASK_STATUS %d\n", status);
		
		// success
		if(status == 1)
		{
			ret = 0;
			break;
		}

		//failure, we need this to quit the loop faster.
		if(status == 2)
		{
			ret = -1;
			break;
		}
	}

	CHECK_ERROR(ret < 0, -1, "ADDMASK failed");
	return ret;
}

int mxuvc_overlay_privacy_remove_mask(video_channel_t ch, uint32_t idx)
{
	
	int ret = 0;
	int data[2]; 
	
	CHECK_ERROR(idx >= PRIVACY_IDX_MAX, -1, "incorrect index");
	CHECK_ERROR(ch >= NUM_MUX_VID_CHANNELS, -1, "incorrect ch");
	CHECK_ERROR(camera == NULL, -1, "camera handle uninitialized");

	data[0] = (int)ch;
	data[1] = idx;

	ret = libusb_control_transfer(camera,
				/* bmRequestType */
		(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ REMOVEMASK,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data[0],
				/* wLength       */ sizeof(data),
				/* timeout*/     0 
		);
	CHECK_ERROR(ret < 0, -1, "REMOVEMASK failed");
	// wait for cmd to execute at fw
	usleep(50000);

	return ret;
}
int mxuvc_overlay_privacy_update_color(video_channel_t ch, privacy_color_t* colors, uint32_t idx)
{
	CHECK_ERROR(colors == NULL, -1, "colors cannot be NULL");
	CHECK_ERROR(ch >= NUM_MUX_VID_CHANNELS, -1, "invalid ch");
	CHECK_ERROR(camera == NULL, -1, "camera handle uninitialized");

	return -1;
}
