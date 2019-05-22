package com.uni.common.util;

import android.app.Activity;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.net.Uri;
import android.os.Environment;
import android.text.TextUtils;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;


/**
 * 文件操作辅助类
 */
public class FileUtil {

    /**
     * 确保目录存在
     *
     * @param path
     * @return
     */
    public static boolean ensureDir(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }

        boolean ret = false;

        File file = new File(path);
        if (!file.exists() || !file.isDirectory()) {
            try {
                ret = file.mkdirs();
            } catch (SecurityException se) {
                se.printStackTrace();
            }
        } else {
            ret = true;
        }

        return ret;
    }

    /**
     * 确保文件所在的目录存在
     *
     * @param path 文件全路径
     * @return
     */
    public static boolean ensureFileParentDir(String path) {
        String parentDir = getFileParentAbsPath(path);
        return ensureDir(parentDir);
    }

    /**
     * 确保文件存在
     *
     * @param path
     * @return
     */
    public static boolean ensureFile(String path) {
        if (null == path) {
            return false;
        }

        boolean ret = false;

        File file = new File(path);
        if (!file.exists() || !file.isFile()) {
            try {
                file.createNewFile();
                ret = true;
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            ret = true;
        }

        return ret;
    }

    /**
     * 获取文件后缀名
     *
     * @param fileName
     * @return
     */
    public static String getFileSuffix(String fileName) {
        String fileType = null;
        if (fileName != null) {
            int idx = fileName.lastIndexOf(".");
            if (idx > 0) {
                fileType = fileName.substring(idx + 1, fileName.length())
                        .toLowerCase();
            }
        }
        return fileType;
    }

    /**
     * 获取文件名
     *
     * @param filePath
     * @return
     */
    public static String getFileNameFromPath(String filePath) {
        String name = null;
        if (filePath != null) {
            int idx = filePath.lastIndexOf("/");
            if (idx > 0) {
                name = filePath.substring(idx + 1, filePath.length())
                        .toLowerCase();
            } else {
                name = filePath;
            }
        }
        return name;
    }

    /**
     * 返回文件的所在的目录的绝对路径
     *
     * @param filePath
     * @return 返回文件的所在的目录的绝对路径, 不含最后的斜杠分隔符
     */
    public static String getFileParentAbsPath(String filePath) {
        File file = new File(filePath);
        return file.getParent();
    }

    /**
     * 判断两个路径是否相等 大小写不敏感 : 存储卡的文件系统一般为FAT, 大小写不敏感
     *
     * @param pathSrc
     * @param pathDst
     * @return
     */
    public static boolean isPathEqual(final String pathSrc, final String pathDst) {
        if (pathSrc == null || pathDst == null) {
            return false;
        }

        String path1 = pathSrc.endsWith("/") ? pathSrc : pathSrc + "/";
        String path2 = pathDst.endsWith("/") ? pathDst : pathDst + "/";
        boolean isEqual = path1.equalsIgnoreCase(path2);
        return isEqual;
    }

    /**
     * 压缩文件到zip. 如果耗时可以放在子线程里进行
     *
     * @param srcFilePath
     * @return 如果成功，zip文件名，失败null
     */
    public static String zipFile(final String srcFilePath) {
        if (srcFilePath == null)
            return null;

        File srcFile = new File(srcFilePath);
        if (!srcFile.exists())
            return null;
        String destFileName = null;
        try {
            FileInputStream srcInput = new FileInputStream(srcFile);
            BufferedInputStream srcBuffer = new BufferedInputStream(srcInput);
            byte[] buf = new byte[1024];
            int len;
            destFileName = srcFilePath + ".zip";
            File destFile = new File(destFileName);
            if (destFile.exists())
                destFile.delete();

            FileOutputStream destFileStream = new FileOutputStream(destFileName);
            BufferedOutputStream destBuffer = new BufferedOutputStream(
                    destFileStream);
            ZipOutputStream zipStream = new ZipOutputStream(destBuffer);// 压缩包
            ZipEntry zipEntry = new ZipEntry(srcFile.getName());// 这是压缩包名里的文件名
            zipStream.putNextEntry(zipEntry);// 写入新的 ZIP 文件条目并将流定位到条目数据的开始处

            while ((len = srcBuffer.read(buf)) != -1) {
                zipStream.write(buf, 0, len);
                zipStream.flush();
            }

            srcBuffer.close();
            zipStream.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }

        return destFileName;
    }

    /**
     * 获取文件类型（后缀）
     *
     * @param name
     * @param defaultValue
     * @return
     */
    public static String getFileTypeByName(String name, String defaultValue) {
        String type = defaultValue;
        if (name != null) {
            int idx = name.lastIndexOf(".");
            if (idx != -1) {
                type = name.substring(idx + 1, name.length());
            }
        }
        return type;
    }

    /**
     * 判断文件是否存在
     */
    public static boolean isFileExist(String path) {

        if (TextUtils.isEmpty(path)) {
            return false;
        }

        File file = new File(path);
        return file.isFile();
    }

    /**
     * 判断文件夹是否存在
     */
    public static boolean isDirExist(String path) {

        if (TextUtils.isEmpty(path)) {
            return false;
        }

        File file = new File(path);
        return file.isDirectory();
    }

    /**
     * 获取文件大小，单位字节
     */
    public static long getFileSize(String path) {

        if (TextUtils.isEmpty(path)) {
            return 0;
        }

        long size = 0;
        File file = new File(path);
        if (file.isFile()) {
            size = file.length();
        }
        return size;
    }

    /**
     * 删除文件
     *
     * @param path
     * @return
     */
    public static boolean deleteFile(String path) {
        if (null == path) {
            return false;
        }
        boolean ret = false;

        File file = new File(path);
        if (file.exists()) {
            ret = file.delete();
        }
        return ret;
    }

    /**
     * Java：判断文件的编码
     *
     * @param sourceFile 需要判断编码的文件
     * @return String 文件编码
     */
    public static String getFilecharset(File sourceFile) {
        String charset = null;
        byte[] first3Bytes = new byte[3];
        try {
            // boolean checked = false;

            BufferedInputStream bis = new BufferedInputStream(
                    new FileInputStream(sourceFile));
            bis.mark(0);

            int read = bis.read(first3Bytes, 0, 3);
            System.out.println("字节大小：" + read);

            if (read == -1) {
                bis.close();
                return null;
            }

            if (first3Bytes[0] == (byte) 0x5c
                    && first3Bytes[1] == (byte) 0x75) {
                charset = "ANSI";   // 文件编码为 ANSI
            } else if (first3Bytes[0] == (byte) 0xFF
                    && first3Bytes[1] == (byte) 0xFE) {

                charset = "UTF-16LE"; // 文件编码为 Unicode
                // checked = true;
            } else if (first3Bytes[0] == (byte) 0xFE
                    && first3Bytes[1] == (byte) 0xFF) {

                charset = "UTF-16BE"; // 文件编码为 Unicode big endian
                // checked = true;
            } else if (first3Bytes[0] == (byte) 0xEF
                    && first3Bytes[1] == (byte) 0xBB
                    && first3Bytes[2] == (byte) 0xBF) {

                charset = "UTF-8"; // 文件编码为 UTF-8
                // checked = true;
            } else {
                charset = "GBK";
            }


            bis.reset();

			/*
             * if (!checked) { int loc = 0;
			 * 
			 * while ((read = bis.read()) != -1) { loc++; if (read >= 0xF0)
			 * break; if (0x80 <= read && read <= 0xBF) // 单独出现BF以下的，也算是GBK
			 * break; if (0xC0 <= read && read <= 0xDF) { read = bis.read(); if
			 * (0x80 <= read && read <= 0xBF) // 双字节 (0xC0 - 0xDF) // (0x80 // -
			 * 0xBF),也可能在GB编码内 continue; else break; } else if (0xE0 <= read &&
			 * read <= 0xEF) {// 也有可能出错，但是几率较小 read = bis.read(); if (0x80 <=
			 * read && read <= 0xBF) { read = bis.read(); if (0x80 <= read &&
			 * read <= 0xBF) { charset = "UTF-8"; break; } else break; } else
			 * break; } } // System.out.println( loc + " " +
			 * Integer.toHexString( read ) // ); }
			 */

            bis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }

        return charset;
    }

    /**
     * @return String
     */
    public static String getFileStr(String path) {
        if (!FileUtil.isFileExist(path)) {
            return "";
        }
        BufferedReader reader = null;
        String laststr = "";
        try {
            FileInputStream fileInputStream = new FileInputStream(path);
            InputStreamReader inputStreamReader = new InputStreamReader(fileInputStream, "utf-8");
            reader = new BufferedReader(inputStreamReader);
            String tempString = null;
            while ((tempString = reader.readLine()) != null) {
                laststr += tempString;
            }
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return laststr;
    }

    public static void writeFileStr(String filePath, String sets) {
        try {
            FileWriter fw = new FileWriter(filePath);
            PrintWriter out = new PrintWriter(fw);
            out.write(sets);
            out.println();
            fw.close();
            out.close();
        } catch (Exception e) {

        }
    }

    /**
     * 拷贝文件
     *
     * @param fromFile 源文件
     * @param toFile   目标文件
     * @param rewrite  是否重写
     */
    public static int copyfile(String fromPath, String toPath, Boolean rewrite) {

        File fromFile = new File(fromPath);
        File toFile = new File(toPath);

        if (!fromFile.exists()) {
            return -1;
        }
        if (!fromFile.isFile()) {
            return -1;
        }
        if (!fromFile.canRead()) {
            return -1;
        }
        if (!toFile.getParentFile().exists()) {
            toFile.getParentFile().mkdirs();
        }

        // 目标文件已存在
        if (toFile.exists()) {

            // 重写，则删除之前的文件
            if (rewrite) {
                toFile.delete();
            }
            // 否则失败
            else {
                return -1;
            }
        }

        // 当文件不存时，canWrite一直返回的都是false
        // if (!toFile.canWrite()) {
        // MessageDialog.openError(new Shell(),"错误信息","不能够写将要复制的目标文件" +
        // toFile.getPath());
        // Toast.makeText(this,"不能够写将要复制的目标文件", Toast.LENGTH_SHORT);
        // return ;
        // }
        try {
            FileInputStream fosFrom = new FileInputStream(fromFile);
            FileOutputStream fosTo = new FileOutputStream(toFile);
            byte bt[] = new byte[1024];
            int c;
            while ((c = fosFrom.read(bt)) > 0) {
                fosTo.write(bt, 0, c); // 将内容写到新文件当中
            }
            fosFrom.close();
            fosTo.close();

            return 0;
        } catch (Exception ex) {
            // Log.e("readfile", ex.getMessage());
            return 1;
        }
    }

    /**
     * 判断一个本地文件是否是图片
     *
     * @param path
     * @return
     */
    public static boolean isFilePicture(String absolutePath) {
        boolean isPicture = false;

        if (!TextUtils.isEmpty(absolutePath)) {
            String[] tmpStrs = absolutePath.split(".");
            if (tmpStrs.length > 1) {

                final String[] PIC_SUFFIX = {
                        "jpg",
                        "png",
                        "bmp",
                        "jpeg",
                        "gif"
                };

                String suffix = tmpStrs[tmpStrs.length - 1];

                for (int i = 0; i < PIC_SUFFIX.length; i++) {
                    if (PIC_SUFFIX[i].equalsIgnoreCase(suffix)) {
                        isPicture = true;
                        break;
                    }
                }
            }
        }

        // 尝试解码看是否是一张图片
        if (!isPicture) {

            Options options = new Options();
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeFile(absolutePath, options);
            if (options.outHeight > 0 && options.outWidth > 0) {
                isPicture = true;
            }
        }

        return isPicture;
    }

    /**
     * 判断一个本地文件是否是图片
     *
     * @param path
     * @return 注意：这个方法不靠谱
     */
    @Deprecated
    public static boolean isFilePicture(Activity activity, Uri uri) {
        String absolutePath = UriUtil.getAbsoluteImagePath(activity, uri);
        return isFilePicture(absolutePath);
    }

    /**
     *
     */
    public static Uri getUri(String path) {
        File file = new File(path);
        return Uri.fromFile(file);
    }


    /**
     * 判断是否存在SD卡
     *
     * @return
     */
    public static boolean isSDCardExist() {
        return Environment.MEDIA_MOUNTED.equalsIgnoreCase(Environment
                .getExternalStorageState());
    }

    //根据时间戳生成文件
    public static File genFileByCurrentTimeMillis(String fileDir){
        SimpleDateFormat format = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
        return new File(fileDir+File.separator+format.format(new Date())+".mp4");
    }
}
