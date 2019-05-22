package com.uni.pano.event;

import com.uni.pano.config.EnumElement;

/**
 * Created by DELL on 2017/3/22.
 */

public class TakePhotoEvent {
    public EnumElement.TAKE_PHOTO_RESULT take_photo_result;
    public String filePath;
    public TakePhotoEvent(EnumElement.TAKE_PHOTO_RESULT takePhotoResult, String filePath){
        take_photo_result = takePhotoResult;
        this.filePath = filePath;
    }
}
