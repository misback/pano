package com.uni.pano.utils;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.text.TextUtils;

import com.uni.pano.MainApplication;

import java.util.HashMap;

/**
 * Created by ZachLi on 2016/9/19.
 */
public class CameraUtil {
    public final static String DEFAULT_USBFS = "/dev/bus/usb";
    /**
     * 检测手机是否插入相机
     * note:这个只是检测手机插入了相机，不是连接
     *
     * @param context 上下文
     * @return 如果已经插入则返回true，否则返回false
     */
    public static boolean isAttachCamera(Context context) {
        UsbManager mUsbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        for (UsbDevice device : deviceList.values()) {
            if (device.getVendorId() == MainApplication.VENDOR_ID && device.getProductId() == MainApplication.PRODUCT_ID) {
                return true;
            }
        }
        return false;
    }
    public static UsbDevice getUsbDevice(Context context){
        UsbManager mUsbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        for (UsbDevice device : deviceList.values()) {
            if (device.getVendorId() == MainApplication.VENDOR_ID && device.getProductId() == MainApplication.PRODUCT_ID) {
                return device;
            }
        }
        return null;
    }
    public static String getUsbFsName(UsbDevice usbDevice) {
        String result = null;
        final String name = usbDevice != null ? usbDevice.getDeviceName() : "";
        final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
        if ((v != null) && (v.length > 2)) {
            final StringBuilder sb = new StringBuilder(v[0]);
            for (int i = 1; i < v.length - 2; i++)
                sb.append("/").append(v[i]);
            result = sb.toString();
        }

        if (TextUtils.isEmpty(result)) {
            result = DEFAULT_USBFS;
        }
        return result;
    }
}
