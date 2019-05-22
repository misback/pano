package com.uni.pano.event;

import com.uni.pano.config.EnumElement;

/**
 * Created by DELL on 2017/3/23.
 */

public class RecordVideoEvent {
    public EnumElement.RECORD_VIDEO_RESULT record_video_result;
    public String filePath;
    public RecordVideoEvent(EnumElement.RECORD_VIDEO_RESULT recordVideoResult, String filePath){
        this.record_video_result = recordVideoResult;
        this.filePath = filePath;
    }
}
