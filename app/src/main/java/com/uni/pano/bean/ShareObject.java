package com.uni.pano.bean;

/**
 * @描述：     @分享对像实体
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class ShareObject {
    /**
     * 分享图标
     */
    public int icon;

    /**
     * 分享中文名称
     */
    public String text;


    public ShareObject(int icon, String text) {
        this.icon = icon;
        this.text = text;
    }
}
