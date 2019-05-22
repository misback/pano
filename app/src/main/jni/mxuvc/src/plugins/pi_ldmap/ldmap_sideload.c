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
#include <assert.h>
#include <math.h>


#define PRINTF(str...)      /*printf(str)*/

#define LDM_DEWARP_CTRLOBJ_TYPE                21
#define SWAP16(d)   (((short)(d) << 8) | (((short)(d) >> 8)&0xff) )
//#define SWAP16(d)   (d)

typedef struct __LDM_MAP_PARAM_STRUCT 
{
    int Divisor;
    int Params[22];
}LDM_MAP_PARAM_STRUCT;

typedef struct __LDM_MAP_HEADER_STRUCT
{
    int   Type;
    short InputFrameWidth;
    short InputFrameHeight;
    short DewarpedFrameWidth;   // this is the output from the dewarp engine.
    short DewarpedFrameHeight;  // this is the output from the dewarp engine.
    short MapN;
    short MapWidth;
    short MapHeight;
    LDM_MAP_PARAM_STRUCT    mapParam;
}LDM_MAP_HEADER_STRUCT;

typedef struct
{
    unsigned int State;
    unsigned int FramesCount;
    unsigned int MapType;
    unsigned int MapMaxLen;
    unsigned int MapN;
    unsigned int MapWidth;
    unsigned int MapHeight;
    // pic sizes
    unsigned int SrcWidth;
    unsigned int SrcHeight;
    unsigned int DewarpedWidth;
    unsigned int DewarpedHeight;
    unsigned int OutputWidth;
    unsigned int OutputHeight;
    unsigned int CompositeWidth;
    unsigned int CompositeHeight;
    // Lens Parameters
    unsigned int lensFOV;
    unsigned int lensRadius;
    unsigned int lensHShift;
    unsigned int lensVShift;
    // Map usage status
    unsigned int LastMap;
    unsigned int CurrentMap;
    unsigned int NextMap;
    // 
    unsigned int mapQLen;
    unsigned int mapQRead;
    unsigned int mapQWrite;
    unsigned int hwtime;    //Time taken by hardware to do dewarping
    unsigned int totaltime; //Total time taken by dewarp hardware plus software overhead
     unsigned int mapQBase[4];
    LDM_MAP_HEADER_STRUCT mapQHeaders[4]; // this array contains the map headers of the maps in mapQBase
} LDM_DEWARP_STATUS;


typedef struct
{
    unsigned int obj;
    int type;
    unsigned char *statBlock;
    unsigned int statBlockSize;
    char name[16];
} LDM_CONTROLOBJ_DESC;

typedef struct
{
    void                        *cmdBlock;
    void                        *evBlock;
    void                        *systemControlStatus;
    LDM_CONTROLOBJ_DESC         *ctrlObjects;
    void                        *DebugLog;
    unsigned int                 evReadWritePtrs;
    void                        *ntpBlock;
    void                        *cmdBlockList;
    void                        *ipcCmd;
    void                        *ipcEvt;
} LDM_GLOBAL_POINTER_BLOCK;

extern int ldmap_wait_dewarp_done(void);
extern int ldm_memw(unsigned int codecAddr, unsigned char *buffer, int buffersize);
extern int ldm_memr(unsigned int codecAddr, unsigned char *buffer, int buffersize);
extern int ldm_init(void);
extern int ldm_deinit(void);




#define GPB_ADDR        0xffc

static LDM_GLOBAL_POINTER_BLOCK     gShadowGPB;
static LDM_CONTROLOBJ_DESC          gpShadowQObjTable[32];
static int gbQobjInit = 0;

static int calc_map_wh(int size, int MapN)
{
    int r = ((size + MapN - 1) / MapN + 3);
    return(r);
}

// given map with or height, and dewarp width or height, calculate MapN
static int calc_mapn(int dwsz, int mapsz)
{
    int r;
    // we are dealing with integer math, and based on the assumption that dwsz is integer multiple of MapN
    mapsz = mapsz - 3;
    r = dwsz / mapsz;
//    PRINTF("calc_mapn: dwsz=%d mapsz=%d r=%d\n", dwsz, mapsz, r);
    return(r);
}

void ldm_dumpbuff(unsigned int *pBuffer, unsigned int wsize, unsigned int outAddr)
{
    unsigned int i;
    printf("Memory dump:\n");
    for(i = 0; i < wsize; i+=4)
    {
        printf("0x%x: %08x %08x %08x %08x\n", outAddr, pBuffer[i], pBuffer[i+1], pBuffer[i+2], pBuffer[i+3]);
        outAddr += 16;
    }
    if(i < wsize)
    {
        printf("0x%x:", outAddr);
        for(;i < wsize; i++)
        {
            printf(" %08x", pBuffer[i]);
            outAddr += 4;
        }
        printf("\n");
    }
}

int ldmap_init(void)
{
    unsigned int gpbCodecAddr;
    int ret;

    if(gbQobjInit != 0)
        return(0);      //already initialized, nothing to do

    ret = ldm_init();
    if(ret < 0)
        return ret;

    // get the pointer to the GPB block
    if(ldm_memr(GPB_ADDR, (unsigned char *)&gpbCodecAddr,
              sizeof(unsigned int)) < 0)
    {
        fprintf(stderr,"ERROR: Failed to get GLOBAL_POINTER_BLOCK's pointer .\n");
        return -1;
    }

    // Get the GPB block
    if(ldm_memr(gpbCodecAddr, (unsigned char *)&gShadowGPB,
              sizeof(gShadowGPB)) < 0)
    {
        fprintf(stderr,"ERROR: Failed to get GLOBAL_POINTER_BLOCK.\n");
        return -1;
    }

    // Get obj array
    if(ldm_memr((unsigned long)(gShadowGPB.ctrlObjects), (unsigned char *)gpShadowQObjTable,
              sizeof(gpShadowQObjTable)) < 0)
    {
        fprintf(stderr,"ERROR: Failed to get obj array\n");
        return -1;
    }

#if 0
    unsigned int *p = (unsigned int *)gpShadowQObjTable;
    int i;
    for(i = 0; i < sizeof(gpShadowQObjTable)/sizeof(LDM_CONTROLOBJ_DESC); i++)
    {
        printf("obj 0x%x type %d statBlock 0x%x statBlockSize %d name %s\n", p[i].obj, p[i].type, p[i].statBlock, p[i].statBlockSize, p[i].name);
    }
#endif

    gbQobjInit = 1;

    return(0);
}

int ldmap_deinit(void)
{
    gbQobjInit = 0;
    ldm_deinit();

    return(0);
}

// given an obj name, the following function returns the 
// codec address of its status struct
unsigned int ldm_qobj_get_stat_caddr(const char *objName)
{
    unsigned int i;
    unsigned int caddr = 0;

    if(gbQobjInit == 0)
        return(0);      // not init yet

    for(i = 0; i < sizeof(gpShadowQObjTable)/sizeof(LDM_CONTROLOBJ_DESC); i++)
    {
        if(strncmp(gpShadowQObjTable[i].name, objName, strlen(objName)) == 0)
        {
            // found, calculate its codec addr ...
            caddr = (unsigned long)(gpShadowQObjTable[i].statBlock);
            break;
        }
    }

    PRINTF("ldm_qobj_get_stat_caddr: got add=0x%x for obj %s\n", caddr, objName);
    return(caddr);
}

int ldmap_upload(const char *pObjName, float *pMapx, float *pMapy, short mapw, short maph, short inw, short inh, short dww, short dwh)
{
    unsigned int dwCodecAddr;
    LDM_DEWARP_STATUS dwStat;
    unsigned int nw;
    int x, y;
    short *piMap;

    PRINTF("ldmap_upload: pObjName=%s mapSize=%dx%d inSize=%dx%d dwSize=%dx%d\n", pObjName, mapw, maph, inw, inh, dww, dwh);

    dwCodecAddr = ldm_qobj_get_stat_caddr(pObjName);
    if(dwCodecAddr <= 0)
    {
        printf("ldmap_upload: ERROR, Object %s is invalid!\n", pObjName);
        return(-1);
    }

    // get dw status from h/w
    if(ldm_memr(dwCodecAddr, (unsigned char *)&dwStat, sizeof(dwStat)) < 0)
    {
        fprintf(stderr,"ERROR: Failed to get dewarp data from h/w\n");
        return -2;
    }
 
    printf("ldmap_upload: MapQ: r=%d w=%d len=%d [0x%x 0x%x 0x%x 0x%x]\n", dwStat.mapQRead, dwStat.mapQWrite, dwStat.mapQLen,
            dwStat.mapQBase[0], dwStat.mapQBase[1], dwStat.mapQBase[2], dwStat.mapQBase[3]);

    // verify map validity
    if(dwStat.MapWidth != (unsigned int)mapw || dwStat.MapHeight != (unsigned int)maph)
    {
        printf("ldmap_upload: Warning, map size %dx%d is invalid, should be %dx%d\n", mapw, maph, dwStat.MapWidth, dwStat.MapHeight);
    }
    if(dwStat.SrcWidth != (unsigned int)inw || dwStat.SrcHeight != (unsigned int)inh)
    {
        printf("ldmap_upload: Warning, input size %dx%d is invalid, should be %dx%d\n", inw, inh, dwStat.SrcWidth, dwStat.SrcHeight);
    }
    if(dwStat.DewarpedWidth != (unsigned int)dww || dwStat.DewarpedHeight != (unsigned int)dwh)
    {
        printf("ldmap_upload: Warning, dewarp size %dx%d is invalid, should be %dx%d\n", dww, dwh, dwStat.DewarpedWidth, dwStat.DewarpedHeight);
    }

    // sanity check
    if(dwStat.mapQLen <= 0)
    {
        printf("ldmap_upload: Error, map que is invalid, have you booted and started the dewarp?\n");
        return(-9);
    }
    assert(dwStat.mapQRead  < dwStat.mapQLen);
    assert(dwStat.mapQWrite < dwStat.mapQLen);

    // try to figure out where to load the map
    nw = (dwStat.mapQWrite + 1) % dwStat.mapQLen;
    
    // check que emptiness
    if(dwStat.mapQRead == nw)
    {
        printf("ldmap_upload: ERROR, map que is full\n");
        return(-6);
    }

    // allocate memory for the integer map
    piMap = (short *)malloc(mapw * maph * sizeof(short) * 2);
    if(piMap == NULL)
    {
        printf("ldmap_upload: ERROR, out of memory\n");
        return(-7);
    }

    // convert map to a format that hardware understands
    short *p = piMap;
    for(y = 0; y < maph ;y++)
    {
        for(x = 0; x < mapw; x++)
        {
            short s;
            int i;
//            PRINTF("ldmap_upload: (%d,%d) (%f, %f)\n", x,y, *pMapx, *pMapy);
            i = (int)rint(*pMapx++ * 8.0);
            i = i >= 32767  ? 32767  : i <= -32768 ? -32768 : i;
            s = (short)(i & 0xffff);
            p[0] = SWAP16((short)s);
            i = (int)rint(*pMapy++ * 8.0);
            i = i >= 32767  ? 32767  : i <= -32768 ? -32768 : i;
            s = (short)(i & 0xffff);
            p[1] = SWAP16((short)s);
//            PRINTF("ldmap_upload: (%d,%d) (0x%04x, 0x%04x)\n", x,y, p[0], p[1]);
            p += 2;
        }
    }

    if(ldmap_wait_dewarp_done() < 0)
    {
        printf("ldmap_upload: ERROR, dewarp get status operation failed\n");
        free(piMap);
        return(-3);
    }
    if(ldmap_wait_dewarp_done() != 0)
    {
        printf("ldmap_upload: ERROR, dewarp get status operation timed out\n");
        free(piMap);
        return(-4);
    }
    
    // load it....
    int mLen = 2*x*y*sizeof(short);
    PRINTF("ldmap_upload: Writing map to codec address 0x%x len %d bytes\n", dwStat.mapQBase[dwStat.mapQWrite], mLen);
    int ret = ldm_memw(dwStat.mapQBase[dwStat.mapQWrite], (unsigned char*)piMap, mLen);
    if(ret != mLen)
    {
        printf("ldmap_upload: ERROR, mem write operation failed, ret=%d\n", ret);
        free(piMap);
        return(-8);
    }

    // update the write pointer
    unsigned int caddr = dwCodecAddr + (unsigned char *)&dwStat.mapQWrite - (unsigned char *)&dwStat;
    PRINTF("ldmap_upload: updating write index at codec addr 0x%x\n", caddr);
    ret = ldm_memw(caddr, (unsigned char*)&nw, sizeof(unsigned int));
    if(ret != sizeof(unsigned int))
    {
        printf("ldmap_upload: ERROR, failed to update Que write index, ret=%d\n", ret);
        free(piMap);
        return(-11);
    }
    
    free(piMap);

    return(0);
}


