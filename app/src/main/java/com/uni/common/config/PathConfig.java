package com.uni.common.config;

import android.content.Context;
import android.os.Environment;
import android.text.TextUtils;

import com.uni.common.env.Env;
import com.uni.common.util.FileUtil;
import com.uni.pano.R;
import com.uni.pano.MainApplication;

import java.io.File;

/**
 * 相关路径配置
 */
public class PathConfig {

    // PS：注意程序中所有出现的路径，结尾都不带 "/"

    /**
     * 获得cache的根目录
     *
     * @return
     */
    public static String getCacheDir() {

        Context context = Env.getContext();

        // 先尝试选择外部缓存
        File file = context.getExternalCacheDir();
        if (file != null) {
            String extCacheDir = file.getAbsolutePath();
            FileUtil.ensureDir(extCacheDir);
            if (FileUtil.isDirExist(extCacheDir)) {
                return extCacheDir;
            }
        }

        // 没有外部缓存，就用内部缓存
        String intcacheDir = context.getCacheDir().getAbsolutePath();
        FileUtil.ensureDir(intcacheDir);
        return intcacheDir;

        // PS: obb用于存放应用下载回来的数据包，这里不使用
        //String obbDir = context.getObbDir().getAbsolutePath();
    }

    /**
     * 获得文件存储目录
     *
     * @return
     */
    public static String getFilesDir() {
        File cacheFile = new File(getCacheDir());
        File file = new File(cacheFile.getParentFile(), "files");
        String filePath = file.getAbsolutePath();
        FileUtil.ensureDir(filePath);
        if (FileUtil.isDirExist(filePath)) {
            return filePath;
        }

        //  如果路径创建失败则直接使用缓存的上一级路径
        return cacheFile.getParentFile().getAbsolutePath();
    }


    public static String getInnerNanoDir() {
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            return Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + MainApplication.getInstance().getPackageName();
        }
        return null;
    }

    //
    public static final String getMediaFolder(){
        String mediaFolder;
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            mediaFolder =  Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + MainApplication.getInstance().getPackageName();
        }else{
            mediaFolder =  MainApplication.getInstance().getFilesDir().getAbsolutePath() + File.separator + MainApplication.getInstance().getPackageName();
        }
        if (!TextUtils.isEmpty(mediaFolder)) {
            FileUtil.ensureDir(mediaFolder);
            if (FileUtil.isDirExist(mediaFolder)) {
                return mediaFolder;
            }
        }
        return MainApplication.getInstance().getCacheDir().getAbsolutePath();
    }
    /**
     * 保存Screen Shot的目录
     */
    public static final String getScreenShotDir() {
        String screenShotDir =  getCacheDir() + File.separator + "screenshot";

        if (!FileUtil.isDirExist(screenShotDir)) {
            FileUtil.ensureDir(screenShotDir);
        }
        return screenShotDir;
    }
    /**
     * 保存Face的目录
     */
    public static final String getFaceDir() {
        String faceDir =  getCacheDir() + File.separator + "face";

        if (!FileUtil.isDirExist(faceDir)) {
            FileUtil.ensureDir(faceDir);
        }
        return faceDir;
    }
    /**
     * 保存selflogo的目录
     */
    public static final String getSelfLogoDir() {
        String selfLogoDir =  getCacheDir() + File.separator + "selfLogo";

        if (!FileUtil.isDirExist(selfLogoDir)) {
            FileUtil.ensureDir(selfLogoDir);
        }
        return selfLogoDir;
    }
    /**
     * 保存相册文件的目录
     */
    public static final String getMediaFileSaveDir() {
        return getMediaFolder();
    }

    /**
     * 获取下载临时目录
     * @return
     */
    public static final String getTempDownloadDir(){
        return getMediaFileSaveDir() + File.separator + "temp";
    }

}
