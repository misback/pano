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
#include <stdint.h>
#include <sys/time.h>

#include "common.h"
#include "mxuvc.h"

FILE* debug_recfd;

int debug_getusleep()
{
	static struct timeval prev_tv = {0, 0};
	struct timeval tv;
	struct timezone tz;
	int usleep = 0;

	gettimeofday(&tv,&tz);
	if (prev_tv.tv_sec == 0 && prev_tv.tv_usec == 0)
		usleep = 0;
	else 
		usleep = (1000000*tv.tv_sec + tv.tv_usec) 
				- (1000000*prev_tv.tv_sec + prev_tv.tv_usec);

	prev_tv = tv;
	return usleep;
}

int mxuvc_debug_startrec(char *outfile)
{
	TRACE("Starting debug recording. Output saved in %s\n", outfile);
	debug_recfd = fopen(outfile, "w");
	CHECK_ERROR(debug_recfd == NULL, -1,
		"Cannot activate record debug feature: "
		"unable to open %s", outfile);

	return 0;
}
int mxuvc_debug_stoprec()
{
	TRACE("Stopping debug recording.\n");
	if (debug_recfd)
		fclose(debug_recfd);

	return 0;
}

