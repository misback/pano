package com.uni.common.util;


import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

import com.uni.pano.R;

/**
 * 获取当前应用的基本信息
 */
public class AppInfoUtil {


    /**
     * 获取客户端版本号
     */
    private static String sVersion;
    /**
     * 获取客户端版本号
     */
    private static int sVersionCode;

    public static String getVersion(Context context) {
        // 只获取一次
        if (sVersion == null) {
            try {
                PackageManager pm = context.getPackageManager();
                String packageName = context.getPackageName();
                PackageInfo pinfo = pm.getPackageInfo(packageName, PackageManager.GET_CONFIGURATIONS);
                sVersion = pinfo.versionName;
            } catch (NameNotFoundException e) {
                sVersion = "1.0.0.0"; // 异常情况
            }
        }
        return sVersion;
    }

    public static int getVersionCode(Context context) {
        // 只获取一次
        if (sVersionCode == 0) {
            try {
                PackageManager pm = context.getPackageManager();
                String packageName = context.getPackageName();
                PackageInfo pinfo = pm.getPackageInfo(packageName, PackageManager.GET_CONFIGURATIONS);
                sVersionCode = pinfo.versionCode;
            } catch (NameNotFoundException e) {
                sVersionCode = 0; // 异常情况
            }
        }
        return sVersionCode;
    }

    /**
     * 获得packageName
     */
    public static String getPackageName(Context context) {
        return context.getPackageName();
    }

    /**
     * 获取应用名称
     */
    public static String getAppName(Context context) {
        String appName = context.getString(R.string.app_name);
        return appName;
    }


}
