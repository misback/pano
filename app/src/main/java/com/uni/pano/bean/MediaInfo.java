package com.uni.pano.bean;

import android.media.MediaMetadataRetriever;
import android.util.Log;

import com.uni.common.util.DateTimeUtil;
import com.uni.common.util.FileUtil;

import java.io.File;
import java.io.Serializable;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by DELL on 2017/2/27.
 */

public class MediaInfo implements Serializable {
    public final static String TAG = "MediaInfo";
    private static final long serialVersionUID = -7060210544600464481L;
    public boolean valid = false;
    public boolean delete = false;
    public String tag;
    public String name;
    public String filePath;
    public String fileDir;
    public String suffix;
    public int width;
    public int height;
    public long lastModified;
    public long length;
    public int duration=0;//duration
    public MEDIA_TYPE type = MEDIA_TYPE.JPEG;
    public boolean selected = false;
    public int index = 1;
    public boolean isAsset  = false;
    public boolean checked  = false;
    public enum MEDIA_TYPE {
        JPEG,JPG,PNG,MP4
    };
    public MediaInfo(){

    }
    public MediaInfo(File file){
        if (file == null){
            delete = true;
            valid = false;
            return;
        }
        name          =   file.getName();
        filePath      =   file.getAbsolutePath();
        fileDir       =   file.getParent();
        length        =   file.length();
        lastModified  =   file.lastModified();
        tag           =   DateTimeUtil.getModifiedDate(lastModified,"yyyy-MM-dd");

        Log.v(TAG,"tag --->" + tag);
        duration      =   0;
        valid = false;
        width = 0;
        height = 0;
        if(name.endsWith(".mp4")) {
            type = MediaInfo.MEDIA_TYPE.MP4;
            suffix="mp4";
            MediaMetadataRetriever mmr = new MediaMetadataRetriever();
            try {
                mmr.setDataSource(filePath);
                String duration = mmr.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
                String sWidth = mmr.extractMetadata(android.media.MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH);
                String sHeight = mmr.extractMetadata(android.media.MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT);
                this.duration =Integer.parseInt(duration);
                this.width = Integer.parseInt(sWidth);
                this.height = Integer.parseInt(sHeight);
                valid = true;
            }catch (Exception e){
                Log.e("game", e.getLocalizedMessage());
                delete = true;
            }
            mmr.release();
        }else if (name.endsWith(".jpg")){
            type = MediaInfo.MEDIA_TYPE.JPG;
            suffix="jpg";
            valid = true;
        }else if (name.endsWith(".jpeg")){
            type = MediaInfo.MEDIA_TYPE.JPEG;
            suffix="jpeg";
            valid = true;
        }else if (name.endsWith(".png")){
            type = MediaInfo.MEDIA_TYPE.PNG;
            suffix="png";
            valid = true;
        }
        if (length<100){
            valid = false;
        }
    }
    public boolean isSameRes(MediaInfo mediaInfo){
        if (mediaInfo!=null&&isAsset == isAsset && filePath.equals(mediaInfo.filePath)){
            return true;
        }else{
            return false;
        }
    }
    public static class SortComparator implements Comparator {
        @Override
        public int compare(Object lhs, Object rhs) {
            MediaInfo a = (MediaInfo) lhs;
            MediaInfo b = (MediaInfo) rhs;
            return Long.compare(b.lastModified,a.lastModified);
        }
    }
    public static class SortMapComparator implements Comparator<HashMap.Entry<String, MediaInfo>> {

        @Override
        public int compare(HashMap.Entry<String, MediaInfo> o1, HashMap.Entry<String, MediaInfo> o2) {
            MediaInfo a = (MediaInfo) o1.getValue();
            MediaInfo b = (MediaInfo) o2.getValue();
            final int offset= (int)(b.lastModified - a.lastModified);

            if(offset == 0){
                return 0;
            }else if(offset > 1){
                return 1;
            }else{
                return -1;
            }

        }
    }
    public boolean isVideo(){
        return type == MEDIA_TYPE.MP4;
    }
    public boolean isExist(){
        FileUtil.isFileExist(this.filePath);
        return true;
    }
}
