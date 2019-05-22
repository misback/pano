package com.uni.pano.event;

import com.uni.pano.bean.FolderInfo;

import java.util.Map;

/**
 * Created by DELL on 2017/4/2.
 */

public class OtherFilesUpdateEvent {
    public Map<String, FolderInfo> folderInfoMap;
    public OtherFilesUpdateEvent(Map<String, FolderInfo> folderInfoMap){
        this.folderInfoMap = folderInfoMap;
    }
}
