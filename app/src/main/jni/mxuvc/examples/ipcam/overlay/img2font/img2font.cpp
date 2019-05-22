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
#include <string.h>

void usage(char * cmd)
{
    printf("usage %s <font size> <infile.gray> <outfile.bin>\n", cmd);
}

int main (int argc, char * argv[])
{
    FILE * infile = NULL;
    FILE * outfile = NULL;
    unsigned char fontChar[32*32];
    int fontsize;
    int fontbuflen;

    if (argc < 4) {
	usage(argv[0]);
	return -1;
    }

    fontsize = atoi(argv[1]);

    if (fontsize != 8 && fontsize != 16 && fontsize != 32) {
        printf("ERROR: font size must be 8, 16 or 32.\n");
        return -1;
    }

    fontbuflen = fontsize * fontsize;

    infile = fopen(argv[2], "rb");
    if (infile == NULL)
	return -1;

    // process 128 characters
    outfile = fopen(argv[3], "wb");
    if (outfile == NULL) {
	fclose(infile);
	return -1;
    }

    for (int i = 0;i < 128;i++) {
	unsigned char * pDot = fontChar;

	fread(fontChar, 1, fontbuflen, infile);

	// convert font
	for (int k = 0;k < fontbuflen;k++) {
	    switch (pDot[k]) {
	    case 0x00:
		pDot[k] = 0x02;
		break;
	    case 0xFF:
		pDot[k] = 0x01;
		break;
	    default:
		pDot[k] = 0x00;
		break;
	    }
	}

	// go through 4 pixels at a time
	for (int j = 0;j < fontsize;j++) {
	    for (int k = 0;k < (fontsize / 4);k++) {
		unsigned char pxOut = 0;

		pxOut = pDot[0] << 6 | pDot[1] << 4 | pDot[2] << 2 | pDot[3];
                fputc(pxOut, outfile);

		pDot += 4;
	    }
	}

    }

    fclose(infile);
    fclose(outfile);

    return 0;
}
