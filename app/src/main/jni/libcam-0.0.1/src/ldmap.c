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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>

#include "libmxcam.h"

#if !defined(_WIN32)
#define DPRINTF(str...)	printf(str)
#else
#define DPRINTF(...)
#endif

#define SWAP16(d)   (((short)(d) << 8) | (((short)(d) >> 8)&0xff) )
#define MAX_GRIDMAP_FILE_SIZE	(64*1024)

#if defined(_WIN32)
int rint(double x)
{
    int temp; temp = (x >= 0. ? (int)(x + 0.5) : (int)(x - 0.5));
    return temp;
}
#endif

static int txt_to_hw_gridmap(const char *mapfile, short **pi_map, unsigned int *size)
{
    FILE *fp;
    int mapw;
    int maph;
    float *pmapx;
    float *pmapy;
    char pstr[256];
    char *ps;
    int map_type;
    int dww;
    int dwh;
    int inw;
    int inh;
    int t, x, y;
	short *p;
    float *px;
    float *py;


#if !defined(_WIN32)
    fp = fopen(mapfile, "rt");
#else
    fopen_s(&fp,mapfile, "rb");
#endif

 	// get map type
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    map_type = strtol(ps, NULL, 0);
    printf("MapType = %d\n", map_type);
   
    // get map size
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    maph = strtol(ps, NULL, 0);
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    mapw = strtol(ps, NULL, 0);
    printf("MapSize=%dx%d\n", mapw, maph);
   
    // Get dw size
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    dww = strtol(ps, NULL, 0);
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    dwh = strtol(ps, NULL, 0);
    printf("DewarpSize=%dx%d\n", dww, dwh);

    // Get input size
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    inw = strtol(ps, NULL, 0);
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    inh = strtol(ps, NULL, 0);
    printf("InputSize=%dx%d\n\n", inw, inh);

    // read out unused fields
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);
    ps = fgets(pstr, sizeof(pstr), fp);
    assert(ps);

   // alloc memory for map
    pmapx = malloc(mapw * maph * sizeof(float));
    if(pmapx == NULL)
    {
        printf("Error, out of memory, MapSize=%dx%d\n", mapw, maph);
        return (-1);
    }
    pmapy = malloc(mapw * maph * sizeof(float));
    if(pmapy == NULL)
    {
    	free(pmapx);
        printf("Error, out of memory, MapSize=%dx%d\n", mapw, maph);
        return (-1);
    }

    // read map from file
    for(t = 0; t < (mapw * maph); t++)
    {
        char *pTok;

        ps = fgets(pstr, sizeof(pstr), fp);
        if(ps == NULL)
        {
            printf("Error, map file is too short!\n");
            exit(-6);
        }
        pTok = strtok(pstr, " \n\t");
        assert(pTok);
#if !defined(_WIN32)
        pmapx[t] = strtof(pTok, NULL);
#else
		pmapx[t] = atof(pTok);
#endif
        pTok = strtok(NULL, " \n\t");
        assert(pTok);
#if !defined(_WIN32)
        pmapy[t] = strtof(pTok, NULL);
#else
		pmapy[t] = atof(pTok);
#endif
//      PRINTF("%d. (%f, %f)\n", t, pMapx[t], pMapy[t]);
    } 

	*pi_map = (short *)malloc(mapw * maph * sizeof(short) * 2);
    if(*pi_map == NULL)
    {
    	free(pmapx);
    	free(pmapy);
        printf("ERR: out of memory!!!\n");
        return (-1);
    }

	*size = (mapw * maph * sizeof(short) * 2);
	
    // convert map to a format that hardware understands
    p = *pi_map;
    px = pmapx;
    py = pmapy;

    for(y = 0; y < maph; y++)
    {
        for(x = 0; x < mapw; x++)
        {
            short s;
            int i;
//            PRINTF("ldmap_upload: (%d,%d) (%f, %f)\n", x,y, *pMapx, *pMapy);
            i = (int)rint(*pmapx++ * 8.0);
            i = i >= 32767  ? 32767  : i <= -32768 ? -32768 : i;
            s = (short)(i & 0xffff);
            p[0] = SWAP16((short)s);
            i = (int)rint(*pmapy++ * 8.0);
            i = i >= 32767  ? 32767  : i <= -32768 ? -32768 : i;
            s = (short)(i & 0xffff);
            p[1] = SWAP16((short)s);
//            PRINTF("ldmap_upload: (%d,%d) (0x%04x, 0x%04x)\n", x,y, p[0], p[1]);
            p += 2;
        }
    }


	free(px);
	free(py);
	fclose(fp);
	
	return MXCAM_OK;
}

int mxcam_ldmap_fill_map(const char *input[], unsigned char **buffer, unsigned int *total_size)
{
	short *pi_map0 = NULL, *pi_map1 = NULL;
	unsigned int map_size0 = 0, map_size1 = 0;
    unsigned int map_count = 0;
    unsigned char *mbuf = NULL;

	const char *gridmap0 = input[0];
	const char *gridmap1 = input[2];
	const char *object0 = input[1]; 
	const char *object1 = input[3]; 

	*total_size = 4 * sizeof(long) + 16;
	

	if(txt_to_hw_gridmap(gridmap0, &pi_map0, &map_size0) != MXCAM_OK)
		return !MXCAM_OK;
			
	*total_size += map_size0;
	map_count++;

	if (gridmap1 != NULL) {
		if (txt_to_hw_gridmap(gridmap1, &pi_map1, &map_size1) != MXCAM_OK)
			return !MXCAM_OK;
			
		*total_size += (map_size1 + 20);
		map_count++;
	}


    if(*total_size >= MAX_GRIDMAP_FILE_SIZE - (map_count * 20 + 16)){ 
        printf("ERR: Max supported 2 Grid Maps size %d Bytes\n", MAX_GRIDMAP_FILE_SIZE);
		return !MXCAM_OK;
    }

	printf("Map size %d and count %d\n", *total_size, map_count);
		
    *buffer = (unsigned char *)malloc(*total_size);
    if(*buffer == NULL){
		printf("ERR: out of memory\n");
		return !MXCAM_OK;
    }

	mbuf = *buffer;

	// 1st 4 bytes: "MAP" string
	strcpy((char *)mbuf, "MAP");
	mbuf += 4;
	// 2nd 4 bytes: Num of map count
	memcpy(mbuf, &map_count, sizeof(long));
	mbuf += 4;
	// 3rd 4 bytes: total size (including headers)
	memcpy(mbuf, total_size, sizeof(long));
	mbuf += 4;
	// 4th 4 bytes: Map-0 size
	printf("MAP-0 size is %d\n", map_size0);
	memcpy(mbuf, &map_size0, sizeof(long));
	mbuf += 4;
	// next 16 bytes: Name of the Object on Map-0
	strcpy((char *)mbuf, object0);
	mbuf += 16;
	memcpy(mbuf, (char *)pi_map0, map_size0);
	free(pi_map0);

	mbuf += map_size0;
	
	if (gridmap1 != NULL) {
		memcpy(mbuf, &map_size1, sizeof(long));
		mbuf += 4;
		strcpy((char *)mbuf, object1);
		mbuf += 16;
		memcpy(mbuf, (char *)pi_map1, map_size1);
		free(pi_map1);
		mbuf += map_size1;
		printf("MAP-1 size is %d\n", map_size1);
	}	
	
    return MXCAM_OK;
}

