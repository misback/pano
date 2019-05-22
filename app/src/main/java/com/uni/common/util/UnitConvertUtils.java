package com.uni.common.util;

/**
 * 单位转换工具类
 * Created by Chenxiran on 2016/5/26.
 */
public class UnitConvertUtils {

    /**
     * 将毫秒时间转化为00:00:00格式
     *
     * @param time 毫秒时间
     * @return
     */
    public static String formatTimeForHour(long time) {

        long hour = (time / (1000 * 60 * 60)) % 60;
        long minute = (time / (1000 * 60)) % 60;
        long second = (time / 1000) % 60;

        String str = String.format("%02d:%02d:%02d", hour, minute, second);
        return str;
    }
    /**
     * 将毫秒时间转化为00:00:00格式
     *
     * @param time 毫秒时间
     * @return
     */
    public static String formatSecondTimeForHour(long time) {

        long hour = (time / (60 * 60)) % 60;
        long minute = (time / 60) % 60;
        long second = (time) % 60;

        String str = String.format("%02d:%02d:%02d", hour, minute, second);
        return str;
    }
    /**
     * 将毫秒时间转化为00:00格式
     *
     * @param time 毫秒时间
     * @return
     */
    public static String formatTime(long time) {

        long minute = (time / (1000 * 60)) % 60;
        long second = (time / 1000) % 60;

        String str = String.format("%02d:%02d", minute, second);
        return str;
    }
}
