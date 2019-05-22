package com.uni.common.util;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.view.Window;
import android.view.WindowManager;

/**
 * 窗口工具类
 * Created by ZachLi on 2016/6/4.
 */
public class WindowUtil {

    /**
     * 获取屏幕宽度
     */
    public static int getWidth(Context context) {
        Resources resources = context.getResources();
        DisplayMetrics displayMetrics = resources.getDisplayMetrics();
        return displayMetrics.widthPixels;
    }

    /**
     * 获取屏幕高度
     */
    public static int getHeight(Context context) {
        Resources resources = context.getResources();
        DisplayMetrics displayMetrics = resources.getDisplayMetrics();
        return displayMetrics.heightPixels;
    }

    /**
     * 判断当前是否为竖屏
     *
     * @return 如果当前为竖屏则返回 true;否则返回 false
     */
    public static boolean isHorizontalScreen(Context context) {
        Resources resources = context.getResources();
        Configuration configuration = resources.getConfiguration();
        int orientation = configuration.orientation;

        return orientation == Configuration.ORIENTATION_LANDSCAPE;

    }

    /**
     * 设置全屏显示(隐藏状态栏与ActionBar)
     *
     * @param act
     */
    public static void setFullScreen(Activity act) {
        final int FLAG_FULLSCREEN = WindowManager.LayoutParams.FLAG_FULLSCREEN;
        act.requestWindowFeature(Window.FEATURE_NO_TITLE);
        act.getWindow().setFlags(FLAG_FULLSCREEN, FLAG_FULLSCREEN);
    }

    /**
     * 设置屏幕方向为横屏,该操作会触发{@link Activity#onConfigurationChanged(Configuration)}
     *
     * @param act
     */
    public static void setScreenOrientationToHorizontal(Activity act) {
        act.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
    }

    /**
     * 设置屏幕方向为竖屏,该操作会触发{@link Activity#onConfigurationChanged(Configuration)}
     *
     * @param act
     */
    public static void setScreenOrientationToPortrait(Activity act) {
        act.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
    }

    /**
     * 开启自动旋转
     *
     * @param context
     */
    public static void enableAutoRotate(Context context) {

        int anInt = Settings.System.getInt(context.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 0);

        if (1 != anInt) {
            Settings.System.putInt(context.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 1);
        }

    }
}
