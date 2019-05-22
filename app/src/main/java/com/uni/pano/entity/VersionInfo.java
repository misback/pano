package com.uni.pano.entity;

import com.google.gson.annotations.SerializedName;

/**
 * @描述：     @版本信息
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class VersionInfo {

    @SerializedName("downloadPrefixUrl")
    public String downloadPrefixUrl;

    @SerializedName("downloadSuffixUrl")
    public String downloadSuffixUrl;

    @SerializedName("publish")
    public boolean publish;

    @SerializedName("upgrade")
    public boolean upgrade;

    @SerializedName("recommend")
    public boolean recommend;

    @SerializedName("fileSize")
    public String  fileSize;

    @SerializedName("fileName")
    public String  fileName;

    @SerializedName("newVersionName")
    public String  newVersionName;

    @SerializedName("updateContent")
    public String  updateContent;

}
