package com.uni.pano.entity;

import com.google.gson.annotations.SerializedName;

/**
 * Created by DELL on 2017/4/6.
 */

public class ShareInfo {
    @SerializedName("shareUrl")
    public String shareUrl;

    @SerializedName("thumbUrl")
    public String thumbUrl;
    @Override
    public String toString() {
        return "ShareInfo{" +
                "shareUrl='" + shareUrl + '\''+
                "thumbUrl='" + thumbUrl + '\''+
                '}';
    }
}
