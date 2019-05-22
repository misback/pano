package com.uni.common.log;

import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;


/**
 * 日志管理
 */
public class CLog {

    private static final String TAG = CLog.class.getSimpleName();
    /**
     * 将日志输出到文件
     *
     * @param tag
     * @param content
     */
    private static FileOutputStream logFile = null;
    private static long mLogTime;

    /**
     * 一般log
     *
     * @param tag
     * @param content
     */
    public static void log(String tag, String content) {
        if (tag == null || content == null) {
            return;
        }

        if (CLogConfig.PRINT_LOG) {
            Log.d(tag, content);
        }
    }

    /**
     * 错误log
     *
     * @param tag
     * @param content
     */
    public static void logError(String tag, String content) {
        if (tag == null || content == null) {
            return;
        }

        if (CLogConfig.PRINT_LOG) {
            Log.e(tag, content);
        }
    }

    public static void logToFile(String tag, String content) {
        if (tag == null || content == null) {
            return;
        }

        if (!CLogConfig.PRINT_LOG) {
            return;
        }

        // 创建日志文件
        if (logFile == null) {

            try {
                String path = CLogConfig.getLogDir();
                File pathFile = new File(path);
                if (!pathFile.exists() || !pathFile.isDirectory()) {
                    pathFile.mkdirs();
                }
                String fileName = path + "/" + "log-" + getDateTime() + ".log";
                logFile = new FileOutputStream(new File(fileName));
            } catch (Exception e) {
                log(TAG, "an error occured while create log file..." + e.toString());
            }
        }

        // 写日志
        try {
            if (logFile != null) {

                String log = String.format("%s %s: %s\n", getMillTimeEx(), tag, content);
                logFile.write(log.getBytes());
                logFile.flush();
                // logFile.close();
            }
        } catch (Exception e) {
            log(TAG, "an error occured while writing log file..." + e.toString());
        }
    }

    /**
     * 辅助函数：获取当前时间
     *
     * @return
     */
    public static String getMillTimeEx() {
        SimpleDateFormat format = new SimpleDateFormat("HH:mm:ss.SSS");
        return format.format(new Date());
    }

    /**
     * 辅助函数：获取当前时间
     *
     * @return
     */
    public static String getDateTime() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH-mm-ss");
        return format.format(new Date());
    }

    /**
     * 用来调试时间间隔
     *
     * @param time
     */
    public static void logTime(String log, long time) {
        log("Time", log + ": " + (time - mLogTime));
        mLogTime = time;
    }
}
