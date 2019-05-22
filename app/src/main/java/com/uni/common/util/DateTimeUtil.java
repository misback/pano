package com.uni.common.util;

import android.annotation.SuppressLint;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

/**
 * 时间，日志之间的格式和转换工具
 */
public class DateTimeUtil {

    public static final int secondOneDay = 24 * 3600 * 1000;
    private static final String TAG = DateTimeUtil.class.getSimpleName();

    public static String getDate() {
        // Calendar cd = Calendar.getInstance();
        // int year = cd.get(Calendar.YEAR);
        // int month = cd.get(Calendar.MONTH);
        // int day = cd.get(Calendar.DAY_OF_MONTH);

        SimpleDateFormat format = new SimpleDateFormat("yyyy.MM.dd");
        return format.format(new Date());
        // return String.format("%d.%d.%d", year, month, day);
    }

    public static String getMillTime() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH-mm-ss.SSS");
        return format.format(new Date());
    }

    public static String getDateTime() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH-mm-ss");
        return format.format(new Date());
    }

    public static String mToStr(int m) {
        m = m / 1000;
        int hour = m / 3600;
        int minute = (m - 3600 * hour) / 60;
        int second = m % 60;
        return String.format("%02d:%02d:%02d", hour, minute, second);

    }

    @SuppressLint("DefaultLocale")
    public static String getFormatTime(int mCurSec) {
        String formattedTime = "00:00:00";
        if (mCurSec == 0) {
            return formattedTime;

        }
        int hour = mCurSec / 60 / 60;
        int leftSec = mCurSec % (60 * 60);
        int min = leftSec / 60;
        int sec = leftSec % 60;
        String strHour = String.format("%d", hour);
        String strMin = String.format("%d", min);
        String strSec = String.format("%d", sec);

        if (hour < 10) {
            strHour = "0" + hour;
        }
        if (min < 10) {
            strMin = "0" + min;
        }
        if (sec < 10) {
            strSec = "0" + sec;
        }
        formattedTime = strHour + ":" + strMin + ":" + strSec;
        return formattedTime;
    }

    public static String getTimeDiscribe(String timeString) {

        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Date date = null;
        try {
            date = dateFormat.parse(timeString);
        } catch (Exception e) {
            return "";
        }
        long time = System.currentTimeMillis() - date.getTime();
        if (time <= 0) {
            return "";
        }

        long second = time / 1000;
        long minute = second / 60;
        long hour = minute / 60;
        long day = hour / 24;

        if (day >= 30) {
            return "一个月";
        }
        if (day > 0) {
            return "" + day + "天";
        }
        if (hour > 0) {
            return "" + hour + "小时";
        }
        if (minute > 0) {
            return "" + minute + "分钟";
        }
        return "1分钟";
    }

    /**
     * 将时间转换为对应的描述
     *
     * @param longString long 类型 Time 的字符串类型
     * @return
     */
    public static String getDiscribe(String longString) {

        Long longTime = Long.parseLong(longString);
        Date date = null;
        try {
            date = new Date(longTime);
        } catch (Exception e) {
            return "";
        }
        long time = System.currentTimeMillis() - date.getTime();
        if (time <= 0) {
            return "";
        }

        long second = time / 1000;
        long minute = second / 60;
        long hour = minute / 60;
        long day = hour / 24;

        if (day >= 30) {
            return "一个月";
        }
        if (day > 0) {
            return "" + day + "天";
        }
        if (hour > 0) {
            return "" + hour + "小时";
        }
        if (minute > 0) {
            return "" + minute + "分钟";
        }
        return "1分钟";
    }

    /**
     * 将 yyyy-MM-dd hh:mm:ss 格式的时间字符串转成 long时间
     *
     * @param strTime
     * @return
     */
    public static long getMillTimeByString(String strTime) {

        long timeMill = 0;

        SimpleDateFormat[] formats = new SimpleDateFormat[]{new SimpleDateFormat("yyyy-MM-dd hh:mm:ss"),
                new SimpleDateFormat("yyyy-MM-dd")};

        for (int i = 0; i < formats.length; i++) {

            try {
                timeMill = formats[i].parse(strTime).getTime();
                // 没有异常，返回
                break;
            } catch (ParseException e) {
                e.printStackTrace();
            }
        }

        return timeMill;
    }

    public static String[] formatTime(long time) {
        long hour = 0;
        long minute = 0;
        final long HOUR_TIME = 3600;
        final long MINUTE_TIME = 60;
        if (time > HOUR_TIME) {
            hour = time / HOUR_TIME;
            time = time - HOUR_TIME * hour;
        }
        if (time > MINUTE_TIME) {
            minute = time / MINUTE_TIME;
            time = time - MINUTE_TIME * minute;
        }
        if (time > 30) {
            minute += 1;
        }
        String[] strs = new String[]{hour + "", "小时", minute + "", "分钟"};
        return strs;
    }

    public static final String getModifiedDate(long time) {
        String result = null;
        Date date = new Date(time);
        SimpleDateFormat formatter;
        formatter = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        result = formatter.format(date);
        return result;
    }

    public static final String getModifiedDate(long time,String format) {
        String result = null;
        Date date = new Date(time);
        SimpleDateFormat formatter;
        formatter = new SimpleDateFormat(format);
        result = formatter.format(date);
        return result;
    }

    public static final String getDurtaionDate(long time) {
        String result = null;
        Date date = new Date(time);
        SimpleDateFormat formatter;
        formatter = new SimpleDateFormat("mm:ss");
        result = formatter.format(date);
        return result;
    }
    /**
     * 大于一天显示日期，小于一天只显示小时分钟
     *
     * @param time
     * @return
     */
    public static final String getDate(long time) {
        String result = null;

        Date date = new Date(time);
        SimpleDateFormat formatter;

        long curTime = System.currentTimeMillis();
        long timeOver = time % secondOneDay;
        long curTimeOver = curTime % secondOneDay;
        if (curTime - time >= secondOneDay || timeOver > curTimeOver) {
            formatter = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        } else {
            formatter = new SimpleDateFormat("HH:mm:ss");
        }
        result = formatter.format(date);

        return result;
    }

    /**
     * 获取时间已经过去的天数,如timeMS距当前时间在一天内则返回1，在两天内则返回2，以此类推
     *
     * @param timeMS
     * @return
     */
    public static final int getTimePassDay(long timeMS) {
        long passTime = System.currentTimeMillis() - timeMS;
        final long SECOND_PER_DAY = 24 * 3600 * 1000;

        if (passTime > 0) {
            // 默认临界值处直接算多一天
            return (int) (passTime / SECOND_PER_DAY + 1);
        } else {
            return 0;
        }
    }

    /**
     * 返回unix时间戳 (1970年至今的秒数)
     * @return
     */
    public static long getUnixStamp(){
        return System.currentTimeMillis()/1000;
    }

    /**
     * 得到昨天的日期
     * @return
     */
    public static String getYesterDate() {
        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.DATE,-1);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        String yestoday = sdf.format(calendar.getTime());
        return yestoday;
    }

    /**
     * 得到今天的日期
     * @return
     */
    public static  String getTodayDate(){
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        String date = sdf.format(new Date());
        return date;
    }
}
