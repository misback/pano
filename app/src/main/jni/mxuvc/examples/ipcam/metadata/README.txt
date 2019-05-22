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

These examples demonstrate receiving metadata from firmware.

==================
dewarp_metadata.c:
==================

It prints dewarp parameters when there is a change.

To test, do following:

1. Use mxcam boot to download a JSON which has following on channel on which metadata is desired::
   AVC channel: Following in "videoEncParams" 
                "Q_AVCENC_CMP_QMED_ENABLE": 1,
                "Q_AVCENC_CMP_QMED_DEWARP_ENABLE": 1,
   JPEG channel: Following in "params"
                "Q_JPEGENC_CMP_ENABLE_QMED": 1,
                "Q_JPEGENC_CMP_QMED_DEWARP_ENABLE": 1,

2. Build example using "make" command.
3. Run using command:
   
   ./dewarp_metadata

   It will report change of dewarp params per panel on the channel in which it is enabled and display all parameters.
   For example:

        dewarp_metadata changed on channel CH1 Panel 0:
            mapType: 120  divisor=10000
            Params: 7300 42100 1800000 8330000 0 0 376690 1200000 0 0 0 0 0 0 0 0 0 0 0 0 0 0

4. In another terminal, issue any command which will change the ePTZ mode on channel.
   You will see dewarp parameters changed message as mentioned in step 3.


If provided in release, see directory json/test for sample JSON scripts using
eTPZ ROI along with a README.txt showing how to use them.

==================
sensor_metadata.c:
==================

It prints sensor and awb parameters when there is a change.

To test, do following:

1. Use mxcam boot to download a JSON which has following on channel on which metadata is desired::
   AVC channel: Following in "videoEncParams"
                "Q_AVCENC_CMP_QMED_ENABLE": 1,
                "Q_AVCENC_CMP_QMED_SENSOR_ENABLE": 1,

       If AWB is also needed, add following:
                "Q_AVCENC_CMP_QMED_AWB_ENABLE": 1,
       If AWB input state are also needed, add following:
                "Q_AVCENC_CMP_QMED_AWBINPUTSTATS_ENABLE": 1,

   JPEG channel: Following in "params"
                "Q_JPEGENC_CMP_ENABLE_QMED": 1,
                "Q_JPEGENC_CMP_QMED_SENSOR_ENABLE": 1,

       If AWB is also needed, add following:
                "Q_JPEGENC_CMP_QMED_AWB_ENABLE": 1,
       If AWB input state are also needed, add following:
                "Q_JPEGENC_CMP_QMED_AWBINPUTSTATS_ENABLE": 1,

2. Build example using "make" command.
3. Run using command:

   ./sensor_metadata

   It will report change of sensor and AWB params on the channel in which it is enabled.
   For example:

        Sensor Info on channel CH1
        Unclipped EV = 862.25
        Analog gain  = 1.02
        Exposure     = 843
        VTS          = 1125
        HTS          = 2206

   If AWB enabled:

        AWB Info on channel CH1
        AWB final gain:  (R) 0x0100  (G) 0x0155  (B) 0x01e8
        Color temperature: 3311
