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

/*******************************************************************************
* @(#) $Id: mxuvc_dewarp.h 57869 2016-04-28 13:03:40Z bsmith $
*******************************************************************************/
#ifndef MXUVC_DEWARP_H
#define MXUVC_DEWARP_H

/** @file
 * @brief dewarp mode specific structures use by mxuvc API
 */

/** @addtogroup dewarp_control
  * @{
  */

/** structure associated to mode EMODE_WM_ZCL */
typedef struct
{
    /** TBD */
    int Phi0;
    /** TBD */
    int Rx;
    /** TBD */
    int Ry;
    /** TBD */
    int Rz;
    /** in degrees */
    int Gshift;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed)) STRUCT_Q_EPTZ_MODE_WM_ZCL;

/** structure associated to mode EMODE_WM_ZCLCYLINDER */
typedef struct
{
    /** The unit is Micro, i.e. millionth */
    int CylinderHeightMicro;
}__attribute__((packed)) STRUCT_Q_EPTZ_MODE_WM_ZCLCYLINDER;

/** structure associated to mode EMODE_WM_ZCLStretch */
typedef struct
{
    /** Horizontal Pan in Pixels */
    int HPan;
    /** Vertical   Pan in Pixels */
    int VPan;
    /** TBD */
    int Zoom;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed)) STRUCT_Q_EPTZ_MODE_WM_ZCLSTRETCH;

/** structure associated to mode EMODE_WM_1PanelEPTZ */
typedef struct
{
    /** Horizontal Pan in degrees */
    int HPan;
    /** Vertical   Pan in degrees */
    int VPan;
    /** in degrees */
    int Tilt;
    /** TBD */
    int Zoom;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
    /** X coordinate of the rectangle center as a ratio of the width of the video channel */
    int XCenter;
    /** Y coordinate of the rectangle center as a ratio of the height of the video channel */
    int YCenter;
    /** Distance from center to top of rectangle as a ratio of the height of the video channel */
    int HeightFromCenter;
}__attribute__((packed)) STRUCT_Q_EPTZ_MODE_WM_1PANELEPTZ;

/** structure associated to mode EMODE_WM_Sweep_1PanelEPTZ */
typedef struct
{
    /** TBD */
    int HPanStart;
    /** TBD */
    int VPanStart;
    /** TBD */
    int ZoomStart;
    /** TBD */
    int TiltStart;
    /** TBD */
    int HPanInc;
    /** TBD */
    int VPanInc;
    /** TBD */
    int TiltInc;
    /** TBD */
    int ZoomInc;
    /** in the number of maps */
    int Period;
    /** common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed)) STRUCT_Q_EPTZ_MODE_SWEEP_WM_1PANELEPTZ;

/** structure associated to mode EMODE_WM_Magnify */
typedef struct
{
    /** TBD */
    int zoom;
    /** TBD */
    int radius;
    /** TBD */
    int xCenter;
    /** TBD */
    int yCenter;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed)) STRUCT_Q_MODE_MAGNIFY;

/** structure associated to mode EMODE_TM_1CircPanoViewPan */
typedef struct
{
    /** in degrees */
    int ArchAngleCenter;
    /** in degrees */
    int ArchAngle;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed))STRUCT_Q_EPTZ_MODE_TM_1CIRCPANOVIEWPAN;

/** structure associated to mode EMODE_CM_CircPanoViewPan */
typedef struct
{
    /** r0, range is [0..Radius1Mili), unit is in milis (i.e. it will be divided by 1000 by the codec) */
	int Radius0Mili;
    /** r1, range is (Radius0Mili..1], unit is in milis (i.e. it will be divided by 1000 by the codec) */
	int Radius1Mili;
    /** Pano size, range is [0...360) degrees */
	int Phi1;
    /** Pano shift, range is [0...360) degrees */
	int PhiShift1;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed))STRUCT_Q_EPTZ_MODE_CM_CIRCPANOVIEWPAN;

/** structure associated to mode EMODE_WM_ROI_1PanelEPTZ .
 This structure is used when a rectangular region of intereset (ROI)
 is selected in ChannelX and is then applied as EMODE_WM_1PanelEPTZ on ChannelY.
 ChannelY will be used as channel parameter in the mxuvc API.
 */
typedef struct
{
    /** X coordinate of the rectangle center in pixels */
    int XCenter;
    /** Y coordinate of the rectangle center in pixels */
    int YCenter;
    /** Distance from center to top of rectangle */
    int HeightFromCenter;
    /** Source channel (ChannelX) */
    int SourceChannel;
    /** Source panel (ChannelX) */
    int SourcePanel;
}__attribute__((packed)) STRUCT_Q_EPTZ_MODE_WM_ROI_1PANELEPTZ;

/** structure associated to mode EMODE_CUSTOM_CAM360  */
typedef STRUCT_Q_EPTZ_MODE_WM_1PANELEPTZ        STRUCT_Q_EPTZ_MODE_CAM360;

/** structure associated to mode EMODE_TM_360CircPanoView_1PanelNonPerspPan */
typedef struct
{
    /** in degrees - TBD */
    int TopPanelArchAngleCenter;
    /** in degrees - TBD */
    int TopPanelArchSectorAngle;
    /** Normalized minimum radios, in Micro units */
    int TopPanelMinRadiusMicro;
    /** Normalized maximum radios, in Micro units */
    int TopPanelMaxRadiusMicro;
    /** TBD */
    int Flip;
    /** Normalized minimum radios, in Micro units */
    int BotPanelMinRadiusMicro;
    /** Normalized maximum radios, in Micro units */
    int BotPanelMaxRadiusMicro;
    /** Common divisor for all fields in this structure.
      Every field in this structure will be divided by this value, unless it's 0. */
    int Divisor;
}__attribute__((packed))STRUCT_Q_EPTZ_MODE_TM_360CIRCPANOVIEW_1PANELNONPERSPPAN;


/** All dewarp mode specific structures are abstracted under union to allow for
 a single/generic data type for the dewarp api.

 User needs to create an instance of this union and initialize the mode specific
 structure parameters. Mechanism to identify the member structure definition is
 provided in mxuvc_dewarp.h. Note that ALL parameters required by the structure
 needs to be updated before setting the same into FW.

Example:
FOR Mode = EMODE_WM_ZCL

@code

dewarp_params_t params; // create union instance.

params.eptz_mode_wm_zcl.Phi0 = 55; //Populate mode specific
parameters.
params.eptz_mode_wm_zcl.Rx = 1;
params.eptz_mode_wm_zcl.Ry = 1;
params.eptz_mode_wm_zcl.Rz = 1;
params.eptz_mode_wm_zcl.GShift = 2;

@endcode

 */
typedef union {
    /** structure associated to mode EMODE_WM_ZCL */
    STRUCT_Q_EPTZ_MODE_WM_ZCL                            	eptz_mode_wm_zcl;
    /** structure associated to mode EMODE_WM_ZCLCylinder */
    STRUCT_Q_EPTZ_MODE_WM_ZCLCYLINDER                    	eptz_mode_wm_zclcylinder;
    /** structure associated to mode EMODE_WM_ZCLStretch */
    STRUCT_Q_EPTZ_MODE_WM_ZCLSTRETCH                     	eptz_mode_wm_zclstretch;
    /** structure associated to mode EMODE_WM_1PanelEPTZ */
    STRUCT_Q_EPTZ_MODE_WM_1PANELEPTZ                     	eptz_mode_wm_1paneleptz;
    /** structure associated to mode EMODE_WM_Sweep_1PanelEPTZ */
    STRUCT_Q_EPTZ_MODE_SWEEP_WM_1PANELEPTZ               	eptz_mode_sweep_wm_1panelptz;
    /** structure associated to mode EMODE_WM_Magnify */
    STRUCT_Q_MODE_MAGNIFY                                	eptz_mode_magnify;
    /** structure associated to mode EMODE_TM_1CircPanoViewPan */
    STRUCT_Q_EPTZ_MODE_TM_1CIRCPANOVIEWPAN                	eptz_mode_tm_1circpanoviewpan;
    /** structure associated to mode EMODE_CM_CircPanoViewPan */
    STRUCT_Q_EPTZ_MODE_CM_CIRCPANOVIEWPAN                       eptz_mode_cm_circpanoviewpan;
    /** structure associated to mode EMODE_WM_ROI_1PanelEPTZ */
    STRUCT_Q_EPTZ_MODE_WM_ROI_1PANELEPTZ                     	eptz_mode_wm_roi_1paneleptz;
    /** structure associated to mode EMODE_CUSTOM_CAM360 */
    STRUCT_Q_EPTZ_MODE_CAM360                                   eptz_mode_cam360;
    /** structure associated to mode EMODE_TM_360CircPanoView_1PanelNonPerspPan */
    STRUCT_Q_EPTZ_MODE_TM_360CIRCPANOVIEW_1PANELNONPERSPPAN    	eptz_mode_tm_360circpanoview_1panelnonperspan;
} dewarp_params_t;

/**@}*/

#endif  /* MXUVC_DEWARP_H */
/*******************************************************************************
* vi: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab nosmarttab:
*******************************************************************************/
