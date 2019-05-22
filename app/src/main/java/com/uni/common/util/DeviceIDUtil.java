package com.uni.common.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.text.TextUtils;

import com.uni.common.env.Env;

/**
 * 获得设备唯一识别码DeviceID
 * PS：目前没有处理山寨机MAC地址冲突的情况
 * <p>
 * 注意：
 * 由于NetUtil.getWifiMacAddress(context)函数不太可靠(手机启动从来没有开过wifi的情况下，使用这个函数会返回null)，
 * 所以DeviceID采用(生成后保存到preference，一直沿用)的策略
 */
public class DeviceIDUtil {

    /**
     * DeviceID临时保存在preference的位置
     */
    private final static String PRE_NAME = "DeviceIDUtil";
    private final static String PRE_KEY = "deviceID";

    /**
     * 获取DeviceID
     */
    private static String sDeviceID;

    public static String getDeviceID(Context context) {

        // 先尝试从preference获取
        if (sDeviceID == null) {
            SharedPreferences preferences = Env.getContext().getSharedPreferences(PRE_NAME, 0);
            String deviceID = preferences.getString(PRE_KEY, "");
            if (!TextUtils.isEmpty(deviceID)) {
                sDeviceID = deviceID;
            }
        }

        // 只获取一次
        if (sDeviceID == null) {
            String mac = NetUtil.getWifiMacAddress(context);

            // 去掉mac地址中的:
            if (TextUtils.isEmpty(mac) == false) {
                mac = mac.replace(":", "");
            }

            // 取不到网卡地址，就用SerialId代替
            if (TextUtils.isEmpty(mac) == true) {
                mac = DeviceUtil.getSerialId(context);
            }
            sDeviceID = mac;

            // 保存到preference
            SharedPreferences preferences = Env.getContext().getSharedPreferences(PRE_NAME, 0);
            Editor editor = preferences.edit();
            editor.putString(PRE_KEY, sDeviceID);
            editor.commit();
        }

        return sDeviceID;
    }
}
