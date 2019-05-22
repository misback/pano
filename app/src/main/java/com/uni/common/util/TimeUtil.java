package com.uni.common.util;

import android.util.Log;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

/**
 * 与业务相关的时间工具类
 */
public class TimeUtil {

    /**
     * 判断传入的参数是否是昨天的时间
     *
     * @param time
     * @return
     */
    static final long MILSEC_OND_DAY = 24 * 3600 * 1000;

    /**
     * 转换时间格式，格式yyyy-MM-dd HH:mm:s
     *
     * @param time
     * @return
     */
    public static String convTimeDetail(long time) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Date dt = new Date(time);
        String sDateTime = sdf.format(dt);
        return sDateTime;
    }

    /**
     * 转换时间格式，格式yyyy/MM/dd HH:mm
     *
     * @param time
     * @return
     */
    public static String convTimeYMDHM(long time) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        Date dt = new Date(time);
        String sDateTime = sdf.format(dt);
        return sDateTime;
    }

    /**
     * 转换时间格式，格式 HH:mm
     *
     * @param time
     * @return
     */
    public static String convTimeHM(long time) {
        SimpleDateFormat sdf = new SimpleDateFormat("HH:mm");
        Date dt = new Date(time);
        String sDateTime = sdf.format(dt);
        return sDateTime;
    }

    /**
     * 转换时间格式，格式HH:mm
     *
     * @param time 毫秒
     * @return
     */
    public static String convTimeHourMin(long time) {
        if (time <= 0) {
            return "00:00";
        } else {
            int hour = (int) (time / (60 * 60 * 1000));
            int minute = (int) ((time / (60 * 1000)) % 60);

            String sHour;
            if (hour <= 0) {
                sHour = "00";
            } else if (hour < 10) {
                sHour = "0" + hour;
            } else {
                sHour = "" + hour;
            }

            String sMin;
            if (minute <= 0) {
                sMin = "00";
            } else if (minute < 10) {
                sMin = "0" + minute;
            } else {
                sMin = "" + minute;
            }

            return String.format("%s:%s", sHour, sMin);
        }
    }
    /**
     * 转换时间格式，格式mm:ss
     *
     * @param time 秒
     * @return
     */
    public static String convTimeMinuteSecond(long time) {
        long minute = time / 60;
        long second = time  % 60;

        String minuteStr;
        if (minute <= 0) {
            minuteStr = "00";
        } else if (minute < 10) {
            minuteStr = "0" + minute;
        } else {
            minuteStr = String.valueOf(minute);
        }

        String secondStr;
        if (second <= 0) {
            secondStr = "00";
        } else if (second < 10) {
            secondStr = "0" + second;
        } else {
            secondStr = String.valueOf(second);
        }
        return String.format("%s:%s", minuteStr, secondStr);
    }
    /**
     * 转换时间格式，格式yyyy-MM-dd
     *
     * @param time
     * @return
     */
    public static String convTime(long time) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        Date dt = new Date(time);
        String sDateTime = sdf.format(dt);
        return sDateTime;
    }

    /**
     * 转换时间格式，格式yyyy/MM/dd
     *
     * @param time
     * @return
     */
    public static String convTimeYMD(long time) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        Date dt = new Date(time);
        String sDateTime = sdf.format(dt);
        return sDateTime;
    }

    /**
     * 将指定格式的时间转换为Long
     *
     * @param time
     * @return Long 时间
     */
    public static long convTimeYmdhmsToLong(String time) {

        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        long lTime = 0;

        try {
            Date date = sdf.parse(time);

            lTime = date.getTime();

        } catch (ParseException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        return lTime;
    }

    /**
     * 转换时间格式，pubTime以毫秒为单位 时间显示规则 t<5分钟：刚刚 5分钟≤t＜60分钟：t分钟前 60分钟≤t＜24小时：t小时前
     * 1天≤t＜3小时：t天前 t>=3天：系统时间 PS：瓶子loading页展示时间：以成功发送时间为准
     * PS：聊天页面的信息展示时间：以成功发送时间为准
     *
     * @param pubTime
     * @return
     */
    public static String convTimeEx(long pubTime) {

        // 特殊处理：如果时间小于0，则返回空
        if (pubTime <= 0) {
            return "刚刚";
        }

        long current = System.currentTimeMillis();
        long duration = current - pubTime;
        int minute = (int) (duration / (60 * 1000));
        int hour = (int) (duration / (60 * 60 * 1000));
        int day = (int) (duration / (24 * 60 * 60 * 1000));

        // 如果当前时间错乱，或者pubTime非法导致当前时间比pubTime值小，则直接显示系统时间
        if (minute < 0) {
            return convTimeYMD(pubTime);
        }

        // t<5分钟：刚刚
        if (minute < 5) {
            return "刚刚";
        }

        // 5分钟≤t＜60分钟：t分钟前
        if (minute < 60) {
            return String.format("%d分钟前", minute);
        }

        // 60分钟≤t＜24小时：t小时前
        if (hour < 24) {
            return String.format("%d小时前", hour);
        }

        // 1天≤t＜7天：t天前
        if (day < 7) {
            return String.format("%d天前", day);
        }

        // t>=7天：系统时间
        return convTimeYMD(pubTime);
    }

    /**
     * XX:XX：      当天
     * 昨天：       日期过了一天
     * 星期X:       日期为一周之内，昨天之后
     * XXXX-XX-XX ： 一周之后
     *
     * @param time
     * @return
     */
    public static String convTimeForChatIndex(long curItemTime) {


        long current = System.currentTimeMillis();
        final long DAY = 1 * 24 * 60 * 60 * 1000;
        final long ONE_WEEK = 7;
        final long ONE_DAY = 1;

        long curDay = current / DAY;
        long curItemDay = curItemTime / DAY;

        // 间隔天数
        long spaceDay = curDay - curItemDay;

        String timeStr = "";
        if (spaceDay > ONE_WEEK) {
            // 1周之外，显⽰时间格式为2015/1/27 15:23

            timeStr = TimeUtil.convTimeYMD(curItemTime);
        } else if (spaceDay > ONE_DAY) {
            // 1周之内，显⽰时间格式为 星期三15:23

            Calendar calendar = Calendar.getInstance();
            calendar.setTimeZone(TimeZone.getTimeZone("GMT+8:00"));
            calendar.setTimeInMillis(curItemTime);
            final String[] WEEK_ARRAY = new String[]{"星期天", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
            int index = calendar.get(Calendar.DAY_OF_WEEK) - 1;
            if (index < 0) {
                index = 0;
            }
            index = index % 7;

            String week = WEEK_ARRAY[index];
            timeStr = week;
        } else if (spaceDay > 0) {
            // 昨天消息，显示时间格式为 昨天15:23
            timeStr = "昨天 ";
        } else {
            // 今天消息 ，显示格式为 15:23
            String hourMin = TimeUtil.convTimeHM(curItemTime);
            timeStr = hourMin;
        }

        return timeStr;
    }

    public static boolean isTimeYesterday(long time) {

        long curTime = System.currentTimeMillis();

        long curDay = curTime / MILSEC_OND_DAY;
        long inputDay = time / MILSEC_OND_DAY;

        return curDay > inputDay;
    }

    /**
     * 根据生日时间字符串获取年龄
     *
     * @param timeString yyyy-MM-dd 格式的时间字符串
     * @return
     */
    public static String getAge(String timeString) {

        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        Date date = null;
        try {
            date = dateFormat.parse(timeString);
        } catch (Exception e) {
            date = null;
        }

        Date curDate = new Date(System.currentTimeMillis());

        if (date == null) {
            // 永远的18岁
            return "18";
        } else {
            int num = curDate.getYear() - date.getYear();

            return num + "";
        }
    }

    /**
     * 根据生日时间字符串获取星座
     *
     * @param timeString yyyy-MM-dd 格式的时间字符串
     * @return
     */
    public static String getConstellation(String timeString) {

        final String[] constellationArr = {
                "魔羯座", "水瓶座", "双鱼座", "牡羊座", "金牛座", "双子座", "巨蟹座", "狮子座", "处女座", "天秤座", "天蝎座", "射手座", "魔羯座"
        };

        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        Date date = null;
        try {
            date = dateFormat.parse(timeString);
        } catch (Exception e) {
            return "";
        }

        int m = date.getMonth() + 1;
        int d = date.getDate();

        if (m >= 13 || m <= 0 || d <= 0 || d >= 32) {
            return "";
        }


        final int[] constellationEdgeDay = {
                20, 18, 20, 20, 20, 21, 22, 22, 22, 22, 21, 21
        };
        int month = m;
        int day = d;
        if (day <= constellationEdgeDay[month - 1]) {
            month = month - 1;
        }
        if (month >= 0) {
            return constellationArr[month];
        }
        // default to return 魔羯
        return constellationArr[11];

    }

    /**
     * XX:XX：           当天
     * 昨天 XX:XX：      日期过了一天
     * 星期X XX:XX：     日期为一周之内，昨天之后
     * XXXX-XX-XX XX:XX： 一周之后
     *
     * @param curItemTime
     */
    public static String getChatTime(long curItemTime) {

        long current = System.currentTimeMillis();
        final long DAY = 1 * 24 * 60 * 60 * 1000;
        final long ONE_WEEK = 7;
        final long ONE_DAY = 1;

        long curDay = current / DAY;
        long curItemDay = curItemTime / DAY;

        // 间隔天数
        long spaceDay = curDay - curItemDay;

        String timeStr = "";
        if (spaceDay > ONE_WEEK) {
            // 1周之外，显⽰时间格式为2015/1/27 15:23

            timeStr = TimeUtil.convTimeYMDHM(curItemTime);
        } else if (spaceDay > ONE_DAY) {
            // 1周之内，显⽰时间格式为 星期三15:23            

            Calendar calendar = Calendar.getInstance();
            calendar.setTimeZone(TimeZone.getTimeZone("GMT+8:00"));
            calendar.setTimeInMillis(curItemTime);
            final String[] WEEK_ARRAY = new String[]{"星期天", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
            int index = calendar.get(Calendar.DAY_OF_WEEK) - 1;
            if (index < 0) {
                index = 0;
            }
            index = index % 7;

            String week = WEEK_ARRAY[index];
            String hourMin = TimeUtil.convTimeHM(curItemTime);
            timeStr = week + " " + hourMin;
        } else if (spaceDay > 0) {
            // 昨天消息，显示时间格式为 昨天15:23

            String hourMin = TimeUtil.convTimeHM(curItemTime);
            timeStr = "昨天 " + hourMin;
        } else {
            // 今天消息 ，显示格式为 15:23

            String hourMin = TimeUtil.convTimeHM(curItemTime);
            timeStr = hourMin;
        }

        return timeStr;
    }


    /**
     * 获得当前时间 HH:mm:ss
     *
     * @return
     */
    public static String getCurrentDate() {
        SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
        return sdf.format(new Date());
    }

    /**
     * N秒前：    Time < 1min
     * N分钟前：  1min <= Time < 1h
     * N小时前：  1h <= Time < 24h
     * N天前：    24h <= Time < 1星期
     * XXXX-XX-XX：Time > 1星期
     *
     * @param pubTime
     * @return
     */
    public static String convTimeForPraise(long pubTime) {

        // 特殊处理：如果时间小于0，则返回空

        final int SEC = 1000;
        final int MIN = 60 * 1000;
        final int HOUR = 60 * 60 * 1000;
        final int DAY = 24 * 60 * 60 * 1000;
        long current = System.currentTimeMillis();

        int dSec = (int) (current / SEC - pubTime / SEC);
        if (dSec < 1) {
            return "刚刚";
        }

        if (dSec >= 1 && dSec < 60) {
            return String.format("%d秒前", dSec);
        }

        int dMin = (int) (current / MIN - pubTime / MIN);
        if (dMin >= 1 && dMin < 60) {
            return String.format("%d分钟前", dMin);
        }

        int dHour = (int) (current / HOUR - pubTime / HOUR);
        if (dHour >= 1 && dHour < 24) {
            return String.format("%d小时前", dHour);
        }

        int dDay = daysBetween(new Date(pubTime), new Date(current));
        if (dDay >= 1 && dDay < 7) {
            return String.format("%d天前", dDay);
        }

        return convTimeYMD(pubTime);

    }

    public static int daysBetween(Date smdate, Date bdate) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        try {
            smdate = sdf.parse(sdf.format(smdate));
            bdate = sdf.parse(sdf.format(bdate));
        } catch (ParseException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        Calendar cal = Calendar.getInstance();
        cal.setTime(smdate);
        long time1 = cal.getTimeInMillis();
        cal.setTime(bdate);
        long time2 = cal.getTimeInMillis();
        long between_days = (time2 - time1) / (1000 * 3600 * 24);

        return Integer.parseInt(String.valueOf(between_days));
    }
}
