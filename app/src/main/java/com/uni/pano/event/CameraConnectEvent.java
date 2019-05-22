package com.uni.pano.event;

/**
 * Created by DELL on 2017/3/10.
 */

public class CameraConnectEvent {
    public static final String ACTION_USB_DEVICE_ATTACHED = "android.hardware.usb.action.USB_DEVICE_ATTACHED";
    public static final String ACTION_USB_DEVICE_DETACHED = "android.hardware.usb.action.USB_DEVICE_DETACHED";
    public static final String ACTION_DEVICE_PERMISSION = "com.uni.pano.USB_PERMISSION";
    public String type;
    public CameraConnectEvent(String _type){
        type = _type;
    }
}
