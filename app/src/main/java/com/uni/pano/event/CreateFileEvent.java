package com.uni.pano.event;

/**
 * Created by DELL on 2017/3/23.
 */

public class CreateFileEvent {
    public String fileName;
    public CreateFileEvent(String name){
        fileName = name;
    }
}
