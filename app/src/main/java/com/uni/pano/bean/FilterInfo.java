package com.uni.pano.bean;

import com.uni.pano.config.EnumElement;

/**
 * Created by DELL on 2017/3/21.
 */

public class FilterInfo {
    public final static String TAG = FilterInfo.class.getSimpleName();
    private static final long serialVersionUID = -7060210544600464381L;
    public String filterName = "";
    public int resId;
    public String filterFileName = "";
    public boolean isSelected;
    public boolean useFaceDetector = false;
    public EnumElement.FILTER_TYPE filterType = EnumElement.FILTER_TYPE.NONE;
    public FilterInfo(String filterName, int resId, String filterFileName, boolean isSelected){
        this.filterName = filterName;
        this.resId = resId;
        this.filterFileName = filterFileName;
        this.isSelected = isSelected;
    }
    public FilterInfo(String filterName, int resId, String filterFileName, boolean isSelected, boolean useFaceDetector){
        this.filterName = filterName;
        this.resId = resId;
        this.filterFileName = filterFileName;
        this.isSelected = isSelected;
        this.useFaceDetector = useFaceDetector;
    }
    public FilterInfo(String filterName, int resId, String filterFileName, boolean isSelected, EnumElement.FILTER_TYPE filterType){
        this.filterName = filterName;
        this.resId = resId;
        this.filterFileName = filterFileName;
        this.isSelected = isSelected;
        this.filterType = filterType;
    }
}
