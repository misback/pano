package com.uni.pano.event;

/**
 * Created by DELL on 2017/3/1.
 */

public class DeleteFilesEvent {
    public boolean delete;
    public DeleteFilesEvent(boolean delete){
        this.delete = delete;
    }
}
