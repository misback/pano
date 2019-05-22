package com.uni.pano.event;

import com.uni.pano.bean.MediaInfo;

import java.util.List;

/**
 * Created by DELL on 2017/4/2.
 */

public class PlayMediaInfoListUpdateEvent {
    public List<MediaInfo> mediaInfoList;
    public PlayMediaInfoListUpdateEvent(List<MediaInfo> mediaInfoList){
        this.mediaInfoList = mediaInfoList;
    }
}
