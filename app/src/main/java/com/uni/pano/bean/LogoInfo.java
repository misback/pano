package com.uni.pano.bean;

/**
 * Created by DELL on 2017/3/21.
 */

public class LogoInfo {
    public final static String TAG = LogoInfo.class.getSimpleName();
    private static final long serialVersionUID = -7060210544600464381L;
    public int resId;
    public String logoFileName;
    public boolean isSelected;
    public boolean isPlusBtn=false;
    public LogoInfo(int resId, String logoFileName, boolean isSelected){
        this.resId = resId;
        this.logoFileName = logoFileName;
        this.isSelected = isSelected;
        this.isPlusBtn = false;
    }
    public LogoInfo(int resId, String logoFileName, boolean isSelected, boolean isPlusBtn){
        this.resId = resId;
        this.logoFileName = logoFileName;
        this.isSelected = isSelected;
        this.isPlusBtn = isPlusBtn;
    }
}
