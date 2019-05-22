package com.uni.pano.event;

import com.uni.pano.bean.MediaInfo;

/**
 * Created by DELL on 2017/3/1.
 */

public class DeleteFileEvent {
    public MediaInfo mediaInfo;
    public DeleteFileEvent(MediaInfo mediaInfo){
        this.mediaInfo = mediaInfo;
    }
}
