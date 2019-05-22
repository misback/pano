package com.uni.common.util;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Environment;
import android.os.StatFs;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigDecimal;
import java.text.DecimalFormat;

/**
 * 转换文件大小显示
 */
public class FileSizeUtil {

    public static final String UNIT_BIT = "B";
    public static final String UNIT_KB = "K";
    public static final String UNIT_MB = "M";
    public static final String UNIT_GB = "G";
    private static final int BASE_B = 1; // 转换为字节基数
    private static final int BASE_KB = 1024; // 转换为KB
    private static final int BASE_MB = 1024 * 1024; // 转换为M的基数
    private static final int BASE_GB = 1024 * 1024 * 1024;

    /**
     * 字节数转出字符串
     *
     * @param byteNum
     * @return
     */
    public static String byteConvert(long byteNum) {
        double tmp = byteNum / (1000 * 1024 * 1024 * 1.0);
        double f;
        String res;
        if (tmp >= 1.0) {
            // 大于等于1000M 用G表示
            f = byteNum / (1024 * 1024 * 1024 * 1.0);
            BigDecimal b = new BigDecimal(f);
            double doubleRes = b.setScale(2, BigDecimal.ROUND_DOWN)
                    .doubleValue();
            res = Double.toString(doubleRes) + "GB";
        } else {
            // 小于1000M
            tmp = byteNum / (1000 * 1024 * 1.0);
            if (tmp >= 1.0) {
                // 大于等于1000K
                f = byteNum / (1024 * 1024 * 1.0);
                BigDecimal b = new BigDecimal(f);
                double doubleRes = b.setScale(1, BigDecimal.ROUND_DOWN)
                        .doubleValue();
                res = Double.toString(doubleRes) + "MB";
            } else {
                // 小于1M
                tmp = byteNum / (1000 * 1.0);
                if (tmp >= 1.0) {
                    // 大于等于1000B
                    f = byteNum / (1024 * 1.0);
                    BigDecimal b = new BigDecimal(f);
                    double doubleRes = b.setScale(1, BigDecimal.ROUND_DOWN)
                            .doubleValue();
                    res = Double.toString(doubleRes) + "KB";
                } else {
                    // 小于1000B
                    res = byteNum + "B";
                }
            }
        }
        return res;
    }


    /**
     * 速度转为字符串
     *
     * @param speed     速度，以B为单位
     * @param precision 保留小数点位数
     * @return String[2] 其中ret[0]是数字 ret[1]是单位
     */
    public static String[] convertSpeeds(long speed, int precision) {
        String[] ret = new String[2];
        String str = convertFileSize(speed, 0);
        // 单位是一个字节
        String unit = str.substring(str.length() - 1);
        ret[0] = str.substring(0, str.lastIndexOf(unit));
        // ret[1] = unit + "/s";
        // 调整单位格式统一设置为B/s、KB/s、MB/s、GB/s
        if (unit.equals(UNIT_BIT)) {
            ret[1] = unit + "/s";
        } else {
            ret[1] = unit + "B/s";
        }

        return ret;
    }

    /**
     * 小数转换为百分值
     *
     * @param value   待转换
     * @param scale   小数位数 10.1% 10.01%
     * @param zeroStr 0的返回值
     * @return e.g. 10.5
     */
    public static String convertPercent(float value, int scale, String zeroStr) {
        BigDecimal b = new BigDecimal(value * 100);
        value = b.divide(new BigDecimal(1), scale, BigDecimal.ROUND_HALF_UP).floatValue();
        if (Float.compare(value, (float) Math.pow(10, -scale)) < 0) {
            return zeroStr;
        } else {
            return String.valueOf(value);
        }
    }


    /**
     * 文件大小转换
     *
     * @param file_size
     * @param precision
     * @return
     */
    public static String convertFileSize(long file_size, int precision) {
        long int_part = 0;
        double fileSize = file_size;
        double floatSize = 0L;
        long temp = file_size;
        int i = 0;
        int base = 1;
        String baseUnit = "M";
        String fileSizeStr = null;
        int indexMid = 0;

        while (temp / 1000 > 0) {
            int_part = temp / 1000;
            temp = int_part;
            i++;
        }
        // if(temp < 1000) {
        // i = 0; //1000B
        // } else if(temp < 1000 * 1024) {
        // i = 1; //1000K
        // } else if(temp < 1000 * 1024 * 1024) {
        // i = 2; //1000M
        // } else if(temp < 1000 * 1024 * 1024) {
        // i = 3; //1000G
        // }

        switch (i) {
            case 0:
                // B
                base = BASE_B;
                baseUnit = UNIT_BIT;
                break;

            case 1:
                // KB
                base = BASE_KB;
                baseUnit = UNIT_KB;
                break;

            case 2:
                // MB
                base = BASE_MB;
                baseUnit = UNIT_MB;
                break;

            case 3:
                // GB
                base = BASE_GB;
                baseUnit = UNIT_GB;
                break;

            case 4:
                base = BASE_GB;
                baseUnit = UNIT_GB;
                // TB
                break;
            default:
                break;
        }
        BigDecimal filesizeDecimal = new BigDecimal(fileSize);
        BigDecimal baseDecimal = new BigDecimal(base);
        floatSize = filesizeDecimal.divide(baseDecimal, precision, BigDecimal.ROUND_HALF_UP).doubleValue();
        fileSizeStr = Double.toString(floatSize);
        if (precision == 0) {
            indexMid = fileSizeStr.indexOf('.');
            if (-1 == indexMid) {
                // 字符串中没有这样的字符
                return fileSizeStr + baseUnit;
            }
            return fileSizeStr.substring(0, indexMid) + baseUnit;
        }

        // baseUnit = UNIT_BIT;
        if (baseUnit.equals(UNIT_BIT)) {
            int pos = fileSizeStr.indexOf('.');
            fileSizeStr = fileSizeStr.substring(0, pos);
        }

        if (baseUnit.equals(UNIT_KB)) {
            int pos = fileSizeStr.indexOf('.');
            if (pos != -1) {
                fileSizeStr = fileSizeStr.substring(0, pos + 2);
            } else {
                fileSizeStr = fileSizeStr + ".0";
            }
        }

        return fileSizeStr + baseUnit;
    }
    private static final int ERROR = -1;

    /**
     * SDCARD是否存
     */
    public static boolean externalMemoryAvailable() {
        return android.os.Environment.getExternalStorageState().equals(
                android.os.Environment.MEDIA_MOUNTED);
    }
    /**
     * 获取手机内部剩余存储空间
     *
     * @return
     */
    public static long getAvailableInternalMemorySize() {
        File path = Environment.getDataDirectory();
        StatFs stat = new StatFs(path.getPath());
        long blockSize = stat.getBlockSize();
        long availableBlocks = stat.getAvailableBlocks();
        return availableBlocks * blockSize;
    }

    /**
     * 获取手机内部总的存储空间
     *
     * @return
     */
    public static long getTotalInternalMemorySize() {
        File path = Environment.getDataDirectory();
        StatFs stat = new StatFs(path.getPath());
        long blockSize = stat.getBlockSize();
        long totalBlocks = stat.getBlockCount();
        return totalBlocks * blockSize;
    }
    /**
     * 获取SDCARD总的存储空间
     *
     * @return
     */
    public static long getAvailableExternalMemorySize() {
        if (externalMemoryAvailable()) {
            File path = Environment.getExternalStorageDirectory();
            StatFs stat = new StatFs(path.getPath());
            long blockSize = stat.getBlockSize();
            long totalBlocks = stat.getAvailableBlocks();
            return totalBlocks * blockSize;
        } else {
            return 0;
        }
    }

    public static long getAvailableMemorySize(String fileDir) {
        StatFs stat = new StatFs(fileDir);
        long blockSize = stat.getBlockSize();
        long totalBlocks = stat.getAvailableBlocks();
        return totalBlocks * blockSize;
    }
    /**
     * 获取SDCARD总的存储空间
     *
     * @return
     */
    public static long getTotalExternalMemorySize() {
        if (externalMemoryAvailable()) {
            File path = Environment.getExternalStorageDirectory();
            StatFs stat = new StatFs(path.getPath());
            long blockSize = stat.getBlockSize();
            long totalBlocks = stat.getBlockCount();
            return totalBlocks * blockSize;
        } else {
            return 0;
        }
    }

    /**
     * 获取系统总内存
     *
     * @param context 可传入应用程序上下文。
     * @return 总内存大单位为B。
     */
    public static long getTotalMemorySize(Context context) {
        String dir = "/proc/meminfo";
        try {
            FileReader fr = new FileReader(dir);
            BufferedReader br = new BufferedReader(fr, 2048);
            String memoryLine = br.readLine();
            String subMemoryLine = memoryLine.substring(memoryLine.indexOf("MemTotal:"));
            br.close();
            return Integer.parseInt(subMemoryLine.replaceAll("\\D+", "")) * 1024l;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return 0;
    }

    /**
     * 获取当前可用内存，返回数据以字节为单位。
     *
     * @param context 可传入应用程序上下文。
     * @return 当前可用内存单位为B。
     */
    public static long getAvailableMemory(Context context) {
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
        am.getMemoryInfo(memoryInfo);
        return memoryInfo.availMem;
    }

    private static DecimalFormat fileIntegerFormat = new DecimalFormat("#0");
    private static DecimalFormat fileDecimalFormat = new DecimalFormat("#0.#");

    /**
     * 单位换算
     *
     * @param size 单位为B
     * @param isInteger 是否返回取整的单位
     * @return 转换后的单位
     */
    public static String formatFileSize(long size, boolean isInteger) {
        DecimalFormat df = isInteger ? fileIntegerFormat : fileDecimalFormat;
        String fileSizeString = "0M";
        if (size < 1024 && size > 0) {
            fileSizeString = df.format((double) size) + "B";
        } else if (size < 1024 * 1024) {
            fileSizeString = df.format((double) size / 1024) + "K";
        } else if (size < 1024 * 1024 * 1024) {
            fileSizeString = df.format((double) size / (1024 * 1024)) + "M";
        } else {
            fileSizeString = df.format((double) size / (1024 * 1024 * 1024)) + "G";
        }
        return fileSizeString;
    }

}
