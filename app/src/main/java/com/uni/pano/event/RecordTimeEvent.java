package com.uni.pano.event;

import com.uni.pano.config.EnumElement;

/**
 * Created by DELL on 2017/4/18.
 */

public class RecordTimeEvent {
    public EnumElement.RECORD_STATE recordState = EnumElement.RECORD_STATE.START;
    public String filePath = "";
    public long seconds = 0;
    public RecordTimeEvent(EnumElement.RECORD_STATE recordState){
        this.recordState = recordState;
    }
    public RecordTimeEvent(EnumElement.RECORD_STATE recordState, String filePath){
        this.recordState = recordState;
        this.filePath = filePath;
    }
    public RecordTimeEvent(EnumElement.RECORD_STATE recordState, long seconds){
        this.recordState = recordState;
        this.seconds = seconds;
    }
}
