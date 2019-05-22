package com.uni.pano.event;

import com.uni.pano.bean.MediaInfo;

import java.util.List;

/**
 * Created by DELL on 2017/4/2.
 */

public class GalleryMediaInfoListUpdateEvent {
    public List<MediaInfo> mediaInfoList;
    public GalleryMediaInfoListUpdateEvent(List<MediaInfo> mediaInfoList){
        this.mediaInfoList = mediaInfoList;
    }
}
