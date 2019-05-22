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
#ifndef SMRECTANGLE_H
#define SMRECTANGLE_H

#define MAX_SMRECTANGLES  25

typedef struct
{
    unsigned char top_left_mbx, top_left_mby;
    unsigned char bot_right_mbx, bot_right_mby;
} smcoordinates_t;

typedef struct
{
    //Basic parameters
    int id;
    smcoordinates_t location;

    //Advanced parameters
    int trackingCount;
    int overlapid;
    int childCount;

    smcoordinates_t parent_location;
} smrectangle_t;

#endif  /* SMRECTANGLE_H */
/*******************************************************************************
* vi: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab nosmarttab:
*******************************************************************************/

