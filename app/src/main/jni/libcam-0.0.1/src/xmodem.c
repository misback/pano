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

#if !defined(_WIN32)
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>
#include <string.h>
#include <unistd.h>

#define XM_SOH  	0x01
#define XM_EOT  	0x04
#define XM_ACK  	0x06
#define XM_NAK  	0x15
#define XM_CAN  	0x18
#define XM_CTRLZ 	0x1A

#define XM_DLY 		10
#define RETRY 		5
#define MAX_BTL_SIZE 	(128*1024)

/* enable for debug prints */
#if 0
#define DEBUG_LOG(args...) printf("xmodem " args)
#else
#define DEBUG_LOG(args...)
#endif

int devfd = -1;

static unsigned short crc16table[256]= {  
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,  
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,  
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,  
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,  
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,  
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,  
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,  
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,  
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,  
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,  
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,  
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,  
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,  
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,  
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,  
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,  
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,  
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,  
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,  
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,  
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,  
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,  
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,  
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,  
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,  
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,  
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,  
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,  
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,  
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,  
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,  
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0  
};  
    
static unsigned short crc16(unsigned char *buf, int len)  
{  
    int i;
    unsigned short crc = 0;  
    for( i = 0; i < len; i++)  
        crc = (crc<<8) ^ crc16table[((crc>>8) ^ *(char *)buf++)&0x00FF];  
#if 0
    for( counter = 0; counter < len; counter++)  {
	x = crc >> 8 ^ buf[counter];
	x ^= x>>4;
	crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x << 5)) ^ ((unsigned short)x);
    }
#endif
    return crc;  
}  

static int configure_serial (const char *dev)
{
	struct termios ttys;
	int mcs = 0;
	int n = 0;

	devfd = open(dev, O_RDWR|O_NDELAY|O_NOCTTY|O_NONBLOCK);
	if (devfd < 0) return devfd;
	
	n = fcntl(devfd, F_GETFL, 0);
	fcntl(devfd, F_SETFL, n & ~O_NDELAY);

	//fcntl(fd, F_SETFL, 0);
	tcgetattr(devfd, &ttys);

	cfsetospeed(&ttys, (speed_t) B115200); 
	cfsetispeed(&ttys, (speed_t) B115200); 
	
	ttys.c_cflag = (ttys.c_cflag & ~CSIZE) | CS8;

	ttys.c_iflag = IGNBRK;
	ttys.c_lflag = 0;
	ttys.c_oflag = 0;

	ttys.c_cflag |= CLOCAL | CREAD;
	ttys.c_cflag &= ~CRTSCTS;

	ttys.c_cc[VMIN] = 0;
	ttys.c_cc[VTIME] = 1;

	ttys.c_iflag &= ~(IXON|IXOFF|IXANY);

	ttys.c_cflag &= ~(PARENB | PARODD);
	ttys.c_cflag &= ~CSTOPB;

	tcsetattr(devfd, TCSANOW, &ttys);
	
	ioctl(devfd, TIOCMGET, &mcs);
	mcs |= TIOCM_RTS;
	ioctl(devfd, TIOCMSET, &mcs);

	//tcflush(fd, TCOFLUSH);
	return devfd;
}

static unsigned char serial_rx_byte(int timeout) 
{
	unsigned char ch = 0;

	while (timeout--) {
		if (read(devfd, &ch, 1))
			break;
	}

	DEBUG_LOG("read byte is 0x%x\n", ch);	
	return ch;
}

static void serial_tx_byte(unsigned int ch) 
{
	DEBUG_LOG("Write:: 0x%x\n", ch);
	write(devfd, &ch, 1);
}

static void serial_flushinput(void)
{
	return;
// blocks for longer time if nothing coming;
//	DEBUG_LOG("going for flush\n");
//	while (serial_rx_byte(XM_DLY) > 0)
//		continue;
}

static void xmodem_flush()
{
	serial_tx_byte(XM_CAN);
	serial_tx_byte(XM_CAN);
	serial_tx_byte(XM_CAN);
	serial_flushinput();
}

static int xmodem_send_buffer(unsigned char *src, int ssize)
{
	unsigned char xmitbuf[1030]; 
	int bufsz, crc = -1;
	unsigned char pktno = 1;
	int i, c, len = 0;
	int retry;

	printf ("Waiting for CCC... sequence\n");
	for(;;) {
		for( retry = 0; retry < RETRY; ++retry) {
			if ((c = serial_rx_byte((XM_DLY)<<1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					printf("Starting Transmit...\n");
					goto start_xmit;

				case XM_NAK:
					crc = 0;
					printf("got NAK retry\n");
					goto start_xmit;

				case XM_CAN:
					printf("got CANCEL\n");
					if ((c = serial_rx_byte(XM_DLY)) == XM_CAN) {
						serial_tx_byte(XM_ACK);
						serial_flushinput();
						return -1;
					}
					break;
				default:
					break;
				}
			}
		}

		xmodem_flush();
		return -1; 
	
		for(;;) {
		start_xmit:
			xmitbuf[0] = XM_SOH; bufsz = 128;
			xmitbuf[1] = pktno;
			xmitbuf[2] = ~pktno;
			c = ssize - len;
			if (c > bufsz) c = bufsz;
			if (c >= 0) {
				memset (&xmitbuf[3], 0, bufsz);
				if (c == 0) {
					xmitbuf[3] = XM_CTRLZ;
				}
				else {
					memcpy (&xmitbuf[3], &src[len], c);
					if (c < bufsz) xmitbuf[3+c] = XM_CTRLZ;
				}
				if (crc) {
					unsigned short ccrc = crc16(&xmitbuf[3], bufsz);
					xmitbuf[bufsz+3] = (ccrc>>8) & 0xFF;
					xmitbuf[bufsz+4] = ccrc & 0xFF;
				}
				else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz+3; ++i) {
						ccks += xmitbuf[i];
					}
					xmitbuf[bufsz+3] = ccks;
				}
				for (retry = 0; retry < RETRY; ++retry) {
					DEBUG_LOG("Retry count %d\n", retry);
					for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
						serial_tx_byte(xmitbuf[i]);
					}
					if ((c = serial_rx_byte(XM_DLY)) >= 0 ) {
						switch (c) {
						case XM_ACK:
					
							++pktno;
							len += bufsz;
							DEBUG_LOG("Got Ack pktno %d\n", pktno);
							printf("\rTransmitting... %% %d", (len * 100 / ssize));
							goto start_xmit;
						case XM_CAN:
							printf("got CANCEL\n");
							if ((c = serial_rx_byte(XM_DLY)) == XM_CAN) {
								serial_tx_byte(XM_ACK);
								serial_flushinput();
								return -1; 
							}
							break;
						case XM_NAK:
							printf("got NAK, retry %d\n", retry);
						default:
							break;
						}
					}
				}
				xmodem_flush();
				return -1; /* xmit error */
			}
			else {
				DEBUG_LOG("sending EOT\n");
				for (retry = 0; retry < 10; ++retry) {
					serial_tx_byte(XM_EOT);
					if ((c = serial_rx_byte((XM_DLY)<<1)) == XM_ACK) break;
				}
				return (c == XM_ACK)?len:-1;
			}
		}
	}
}

int xmodem_transmit(const char *dev, unsigned char *buf, unsigned long size)
{
	int is_bootloader = 0;
	
	devfd = configure_serial(dev);
	if (devfd < 0) {
		printf("Unable to open serial device: %s\n", dev);
		return -1;
	}

	// check u-boot imagic number at offset 8
	// mboot detection logic
	if ((buf[8] == 0x27) && 
		(buf[9] == 0x5) && 
		(buf[10] == 0x19) && 
		(buf[11] == 0x56)) {
      		buf += 8;
		size -= 8;
		is_bootloader = 1;
        } 
	
	if (xmodem_send_buffer(buf, size) < 0) {
		close(devfd);
		return -1;
	}

	if (is_bootloader) {
		unsigned char usbboot[] = {0x75,0x73,0x62,0x62,0x6f,0x6f,0x74,0xd,0xa};
		unsigned char spiboot[] = {0x73,0x70,0x69,0x62,0x6f,0x6f,0x74,0xd,0xa};
		unsigned int i = 0;

		for (i=0; i < sizeof(usbboot); i++) {
			unsigned int c = serial_rx_byte(XM_DLY);

			if ((c == usbboot[i]) || (c == spiboot[i])) {
				continue;
			} else {
				close(devfd);
				return -1;
			} /* else */
		} /* for */
		DEBUG_LOG("Got correct \"boot\" string\n");
	} /* if */

	
	close(devfd);
	return 0;
}

#else

/* to avoid  windows compilation error */
int xmodem_transmit(const char *dev, unsigned char *buf, unsigned long size)
{
	printf("Xmodem function not implemented!!! %s, %p, %ld\n", dev, buf, size);
	return -1;
}

#endif
