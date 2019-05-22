package com.uni.pano.entity;

import com.google.gson.annotations.SerializedName;

public class Token {

    @SerializedName("bucket")
    public String bucket;//上传到七牛的空间

    @SerializedName("fileName")
    public String fileName;//上传到七牛的key（即文件名）

    @SerializedName("thumb")
    public String thumb;//缩略图

    @SerializedName("token")
    public String token;

    @SerializedName("domain")
    public String domain;

    @SerializedName("shareUrl")
    public String shareUrl;

    @SerializedName("shareId")
    public int shareId;

    @Override
    public String toString() {
        return "Token{" +
                "bucket='" + bucket + '\'' +
                ", fileName='" + fileName + '\'' +
                ", thumb='" + thumb + '\'' +
                ", token='" + token + '\'' +
                ", domain='" + domain + '\'' +
                ", shareUrl='" + shareUrl + '\'' +
                ", shareId=" + shareId  +
                '}';
    }
}
