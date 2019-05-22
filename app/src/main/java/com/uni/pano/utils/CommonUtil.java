package com.uni.pano.utils;

import android.app.Activity;
import android.app.Service;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.hardware.usb.UsbDevice;
import android.media.MediaMetadataRetriever;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Environment;
import android.os.StatFs;
import android.os.Vibrator;
import android.provider.MediaStore;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.annotation.RawRes;
import android.system.Os;
import android.text.TextUtils;
import android.util.DisplayMetrics;

import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.bean.FolderInfo;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.MainApplication;
import com.uni.pano.event.ConnectEvent;
import com.uni.pano.logutils.Constants;

import java.io.File;
import java.io.FileFilter;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Pattern;

import static com.umeng.socialize.common.SocializeConstants.OS;

public class CommonUtil {

    private static String[] months = {"Jan", "Feb", "March", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    public static final int ALBUM_ALL = 0;
    public static final int ALBUM_VIDEO = 1;
    public static final int ALBUM_COLLECTION = 2;

    public static boolean isEnglish() {
        int language = PreferenceModel.getInt(Constants.PREFERENCE_KEY_LANGUAGE, Constants.PREFERENCE_KEY_LANGUAGE_VALUE_FOLLOW_SYSTEM);

        if (language == Constants.PREFERENCE_KEY_LANGUAGE_VALUE_CHINESE) {
            return false;
        } else if (language == Constants.PREFERENCE_KEY_LANGUAGE_VALUE_ENGLISH) {
            return true;
        } else {
            if ("zh".equals(Locale.getDefault().getLanguage())) {
                return false;
            } else {
                return true;
            }
        }
    }
    public static boolean isHaveSensorRotate(Context context){
        SensorManager sensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        if (sensorManager != null) {
            return sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)!=null;
        }
        return false;
    }
    public static boolean isOpenRotateScreenInSetting(Context context){
        return Settings.System.getInt(context.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 0) == 1;
    }
    public static void openRotateScreenInSetting(Context context){
        //0为关闭 1为开启
        if(!isOpenRotateScreenInSetting(context)) {
            Settings.System.putInt(context.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 1);
        }
    }
    public static void listHashMapMediaInfo(String folderPath, HashMap<String, MediaInfo> hashMapMediaInfo){
        MediaMetadataRetriever mmr = new MediaMetadataRetriever();
        File mediaDir = new File(folderPath);
        File[] files = mediaDir.listFiles(new FileFilter() {
            @Override
            public boolean accept(File pathname) {
                String  name    =   pathname.getName();
                if (name.endsWith(".png") || name.endsWith(".jpg") || name.endsWith(".jpeg")){
                    return true;
                }else if(name.endsWith(".mp4")){
                    return true;
                }else{
                    return pathname.isDirectory();
                }
            }
        });
        for (File file:files){
            if (file.isDirectory()){
                listHashMapMediaInfo(file.getAbsolutePath(), hashMapMediaInfo);
            }else {
                MediaInfo mediaInfo     =   new MediaInfo(file);
                if (hashMapMediaInfo.containsKey(mediaInfo.filePath)){
                    continue;
                }
                if (mediaInfo.valid){
                    hashMapMediaInfo.put(mediaInfo.filePath, mediaInfo);
                }else{
                    file.delete();
                    continue;
                }
            }
        }
        mmr.release();
    }
    public static void listMediaInfoInFolder(String folderPath, List<MediaInfo> arrayMediaInfo){
        File mediaDir = new File(folderPath);
        File[] files = mediaDir.listFiles(new FileFilter() {
            @Override
            public boolean accept(File pathname) {
                String  name    =   pathname.getName();
                if (name.endsWith(".png") || name.endsWith(".jpg") || name.endsWith(".jpeg")){
                    return true;
                }else if(name.endsWith(".mp4")){
                    return true;
                }else{
                    return pathname.isDirectory();
                }
            }
        });
        for (File file:files){
            if (file.isDirectory()){
                listMediaInfo(file.getAbsolutePath(), arrayMediaInfo);
            }else {
                MediaInfo mediaInfo     =   new MediaInfo(file);
                if (mediaInfo.valid){
                    arrayMediaInfo.add(mediaInfo);
                }else{
                    file.delete();
                }
            }
        }
        if (arrayMediaInfo.size()>2){
            Collections.sort(arrayMediaInfo, new MediaInfo.SortComparator());
        }
    }

    public static final void listMediaInfo(String folderPath, List<MediaInfo> arrayMediaInfo){
        final File mediaDir = new File(folderPath);
        File[] files = mediaDir.listFiles(new FileFilter() {
            @Override
            public boolean accept(File pathname) {
                String  name    =   pathname.getName();
                if (name.endsWith(".png") || name.endsWith(".jpg") || name.endsWith(".jpeg")){
                    return true;
                }else if(name.endsWith(".mp4")){
                    return true;
                }else{
                    return pathname.isDirectory();
                }
            }
        });

        if(null != files){
            for (File file:files){
                MediaInfo mediaInfo     =   new MediaInfo(file);
                if (mediaInfo.valid){
                    arrayMediaInfo.add(mediaInfo);
                }else{
                    file.delete();
                }
            }

            //排序
            if (arrayMediaInfo.size()>=2){
                Collections.sort(arrayMediaInfo, new MediaInfo.SortComparator());
            }
        }

    }

    public static List<MediaInfo> listMediaInfo(String folderPath){
        List<MediaInfo> arrayMediaInfo = new ArrayList<>();
        File mediaDir = new File(folderPath);
        File[] files = mediaDir.listFiles(new FileFilter() {
            @Override
            public boolean accept(File pathname) {
                String  name    =   pathname.getName();
                if (name.endsWith(".png") || name.endsWith(".jpg") || name.endsWith(".jpeg")){
                    return true;
                }else if(name.endsWith(".mp4")){
                    return true;
                }else{
                    return pathname.isDirectory();
                }
            }
        });
        for (File file:files){
            MediaInfo mediaInfo     =   new MediaInfo(file);
            if (mediaInfo.valid){
                arrayMediaInfo.add(mediaInfo);
            }else{
                file.delete();
            }
        }
        if (arrayMediaInfo.size()>2){
            Collections.sort(arrayMediaInfo, new MediaInfo.SortComparator());
        }
        return arrayMediaInfo;
    }


    public static void listMediaInfo(Context context, List<MediaInfo> mediaInfoList, String fileDir, int minHeight){
        {
            Uri mImageUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
            ContentResolver mContentResolver = context.getContentResolver();
            //只查询jpeg和png的图片
            Cursor mCursor = mContentResolver.query(mImageUri, null,
                    MediaStore.Images.Media.MIME_TYPE + "=? or "
                            + MediaStore.Images.Media.MIME_TYPE + "=? or "
                            + MediaStore.Images.Media.MIME_TYPE + "=?",
                    new String[]{"image/jpeg","image/jpg","image/png"}, null);

            if (mCursor == null) {
                return;
            }
            while (mCursor.moveToNext()) {
                int width = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Images.Media.WIDTH));
                int height = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Images.Media.HEIGHT));
                if (width != (height*2) || height<minHeight){
                    continue;
                }
                File file = new File(mCursor.getString(mCursor.getColumnIndex(MediaStore.Images.Media.DATA)));
                if (file==null || !fileDir.equals(file.getParent())){
                    continue;
                }
                MediaInfo mediaInfo = new MediaInfo(file);
                if(mediaInfo.valid) {
                    mediaInfoList.add(mediaInfo);
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
        }
        {
            Uri videoUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            ContentResolver mContentResolver = context.getContentResolver();
            //只查询jpeg和png的图片
            Cursor mCursor = mContentResolver.query(videoUri, null,
                    MediaStore.Video.Media.MIME_TYPE + "=?",
                    new String[]{"video/mp4"}, null);

            if (mCursor == null) {
                return;
            }
            while (mCursor.moveToNext()) {
                int width = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Video.Media.WIDTH));
                int height = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Video.Media.HEIGHT));
                if (width != (height*2) || height<minHeight){
                    continue;
                }
                File file = new File(mCursor.getString(mCursor.getColumnIndex(MediaStore.Video.Media.DATA)));
                if (file==null || !fileDir.equals(file.getParent())){
                    continue;
                }
                MediaInfo mediaInfo = new MediaInfo(file);
                if(mediaInfo.valid) {
                    mediaInfoList.add(mediaInfo);
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
        }
    }

    public static void listAllPanoramaMediaInfo(Context context, List<MediaInfo> mediaInfoList, int minHeight){
        {
            Uri mImageUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
            ContentResolver mContentResolver = context.getContentResolver();
            //只查询jpeg和png的图片
            Cursor mCursor = mContentResolver.query(mImageUri, null,
                    MediaStore.Images.Media.MIME_TYPE + "=? or "
                            + MediaStore.Images.Media.MIME_TYPE + "=? or "
                            + MediaStore.Images.Media.MIME_TYPE + "=?",
                    new String[]{"image/jpeg","image/jpg","image/png"}, null);

            if (mCursor == null) {
                return;
            }
            while (mCursor.moveToNext()) {
                int width = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Images.Media.WIDTH));
                int height = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Images.Media.HEIGHT));
                if (width != (height*2) || height<minHeight){
                    continue;
                }
                File file = new File(mCursor.getString(mCursor.getColumnIndex(MediaStore.Images.Media.DATA)));
                if(file==null){
                    continue;
                }
                MediaInfo mediaInfo = new MediaInfo(file);
                if(mediaInfo.valid) {
                    mediaInfoList.add(mediaInfo);
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
        }
        {
            Uri videoUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            ContentResolver mContentResolver = context.getContentResolver();
            //只查询jpeg和png的图片
            Cursor mCursor = mContentResolver.query(videoUri, null,
                    MediaStore.Video.Media.MIME_TYPE + "=?",
                    new String[]{"video/mp4"}, null);

            if (mCursor == null) {
                return;
            }
            while (mCursor.moveToNext()) {
                int width = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Video.Media.WIDTH));
                int height = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Video.Media.HEIGHT));
                if (width != (height*2) || height<minHeight){
                    continue;
                }
                File file = new File(mCursor.getString(mCursor.getColumnIndex(MediaStore.Video.Media.DATA)));
                if(file==null){
                    continue;
                }
                MediaInfo mediaInfo = new MediaInfo(file);
                if(mediaInfo.valid) {
                    mediaInfoList.add(mediaInfo);
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
        }
    }

    public static void listFolderInfo(Context context, Map<String, FolderInfo> folderInfoMap, int minHeight){
        {
            Uri mImageUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
            ContentResolver mContentResolver = context.getContentResolver();
            //只查询jpeg和png的图片
            Cursor mCursor = mContentResolver.query(mImageUri, null,
                    MediaStore.Images.Media.MIME_TYPE + "=? or "
                            + MediaStore.Images.Media.MIME_TYPE + "=? or "
                            + MediaStore.Images.Media.MIME_TYPE + "=?",
                    new String[]{"image/jpeg","image/jpg","image/png"}, null);

            if (mCursor == null) {
                return;
            }
            while (mCursor.moveToNext()) {
                int width = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Images.Media.WIDTH));
                int height = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Images.Media.HEIGHT));
                if (width != (height*2) || height<minHeight){
                    continue;
                }
                File file = new File(mCursor.getString(mCursor.getColumnIndex(MediaStore.Images.Media.DATA)));
                if(file==null){
                    continue;
                }
                MediaInfo mediaInfo = new MediaInfo(file);
                if (mediaInfo.valid) {
                    File folder = file.getParentFile();
                    if (!folderInfoMap.containsKey(mediaInfo.fileDir)) {
                        folderInfoMap.put(mediaInfo.fileDir, new FolderInfo(folder));
                    }
                    folderInfoMap.get(mediaInfo.fileDir).size++;
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
        }
        {
            Uri videoUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            ContentResolver mContentResolver = context.getContentResolver();
            //只查询jpeg和png的图片
            Cursor mCursor = mContentResolver.query(videoUri, null,
                    MediaStore.Video.Media.MIME_TYPE + "=?",
                    new String[]{"video/mp4"}, null);

            if (mCursor == null) {
                return;
            }
            while (mCursor.moveToNext()) {
                int width = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Video.Media.WIDTH));
                int height = mCursor.getInt(mCursor.getColumnIndex(MediaStore.Video.Media.HEIGHT));
                if (width != (height*2) || height<minHeight){
                    continue;
                }
                File file = new File(mCursor.getString(mCursor.getColumnIndex(MediaStore.Video.Media.DATA)));
                if(file==null){
                    continue;
                }
                MediaInfo mediaInfo = new MediaInfo(file);
                if (mediaInfo.valid) {
                    File folder = file.getParentFile();
                    if (!folderInfoMap.containsKey(mediaInfo.fileDir)) {
                        folderInfoMap.put(mediaInfo.fileDir, new FolderInfo(folder));
                    }
                    folderInfoMap.get(mediaInfo.fileDir).size++;
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
        }
    }


    public static String getThumbnailUrl(String path) {
        File file = new File(path);
        String thumbnailDir = file.getParentFile().getAbsolutePath() + File.separator + "thumbnail" + File.separator;
        if (path.endsWith(".jpg")) {
            return thumbnailDir + file.getName();
        } else if (path.endsWith(".mp4")) {
            return thumbnailDir + file.getName().replace(".mp4", ".jpg");
        }
        return null;
    }

    public static boolean rotationSensorCheck(Context context) {
        SensorManager sm = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        if (sm == null) return false;
        return sm.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR) != null;
    }

    public static String formatNumber(String iniNum, int split ) {
        StringBuffer tmp = new StringBuffer().append(iniNum.replaceAll(",", "")).reverse();
        String retNum = Pattern.compile("(\\d{" + split + "})(?=\\d)").matcher(tmp.toString()).replaceAll("$1,");
        return new StringBuffer().append(retNum).reverse().toString();
    }

    /**
     * 获取顶部状态栏高度
     *
     * @return 顶部状态栏高度
     */
    public static int getStatusBarHeight(@NonNull Context context) {
        Class<?> c;
        Object obj;
        java.lang.reflect.Field field;
        int x;
        int statusBarHeight = 0;
        try {
            c = Class.forName("com.android.internal.R$dimen");
            obj = c.newInstance();
            field = c.getField("status_bar_height");
            x = Integer.parseInt(field.get(obj).toString());
            statusBarHeight = context.getResources().getDimensionPixelSize(x);
            return statusBarHeight;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return statusBarHeight;
    }

    public static void Vibrate(Context context, long milliseconds) {
        try {
            Vibrator vib = (Vibrator) context.getSystemService(Service.VIBRATOR_SERVICE);
            if(vib != null)
                vib.vibrate(milliseconds);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static double getSDAvailableSize() {
        double k = 1024f;
        File sdcardDir = Environment.getExternalStorageDirectory();
        StatFs sf = new StatFs(sdcardDir.getPath());
        long blockSize = sf.getBlockSize();
        long availCount = sf.getAvailableBlocks();
        return ((blockSize * availCount) / k) / k;
    }


    public static String copyAlaphFile(Context context,@RawRes int id,  String fileName){
        InputStream inputStream = null;
        FileOutputStream fileOutputStream = null;
        String ret = null;
        try {
            File file = new File(context.getApplicationContext().getFilesDir().getAbsolutePath()+ File.separator+fileName);
            if(!file.exists()) {
                inputStream =  context.getResources().openRawResource(id);
                fileOutputStream = new FileOutputStream(file);
                int length = -1;
                byte[] buf = new byte[1024];
                while ((length = inputStream.read(buf)) != -1)
                {
                    fileOutputStream.write(buf, 0, length);
                }
                fileOutputStream.flush();
            }
            ret = file.getAbsolutePath();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if(inputStream != null) {
                    inputStream.close();
                }
                if(fileOutputStream != null) {
                    fileOutputStream.close();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            return ret;
        }
    }
    public static boolean isPortrait(Activity context)
    {
        DisplayMetrics dm = new DisplayMetrics();
        context.getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        return height>width;
    }
    /**
     *
     * @param myContext
     * @param ASSETS_NAME 要复制的文件名
     * @param savePath 要保存的路径
     * @param saveName 复制后的文件名
     *  testCopy(Context context)是一个测试例子。
     */

    public static void copy(Context myContext, String ASSETS_NAME, String savePath, String saveName) {
        String filename = savePath + "/" + saveName;
        File dir = new File(savePath);
        // 如果目录不中存在，创建这个目录
        if (!dir.exists()) {
            dir.mkdir();
        }
        try {
            if (!(new File(filename)).exists()) {
                InputStream is = myContext.getResources().getAssets().open(ASSETS_NAME);
                FileOutputStream fos = new FileOutputStream(filename);
                byte[] buffer = new byte[7168];
                int count = 0;
                while ((count = is.read(buffer)) > 0) {
                    fos.write(buffer, 0, count);
                }
                fos.close();
                is.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
