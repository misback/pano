package com.uni.pano.bean;

import java.io.File;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;

/**
 * Created by DELL on 2017/2/27.
 */

public class FolderInfo implements Serializable {
    public final static String TAG = FolderInfo.class.getSimpleName();
    private static final long serialVersionUID = -7060210544600464581L;
    public String name;
    public String filePath;
    public int size;
    public FolderInfo(){

    }
    public FolderInfo(File file){
        name = file.getName();
        filePath = file.getAbsolutePath();
    }
    public FolderInfo(String name, String filePath){
        this.name = name;
        this.filePath = filePath;
    }
    public boolean isSame(FolderInfo folderInfo){
        if (folderInfo!=null&& folderInfo.filePath.equals(filePath)){
            return true;
        }else{
            return false;
        }
    }
}
