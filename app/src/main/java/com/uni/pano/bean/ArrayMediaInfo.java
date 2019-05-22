package com.uni.pano.bean;

import android.media.MediaMetadataRetriever;
import android.util.Log;

import com.uni.common.util.FileUtil;

import java.io.File;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;

/**
 * Created by DELL on 2017/2/27.
 */

public class ArrayMediaInfo implements Serializable {
    public final static String TAG = ArrayMediaInfo.class.getSimpleName();
    private static final long serialVersionUID = -7060210544600464481L;
    public List<MediaInfo> mediaInfoList = null;
    public ArrayMediaInfo(List<MediaInfo> mediaInfoList){
        this.mediaInfoList = mediaInfoList;
    }
}
