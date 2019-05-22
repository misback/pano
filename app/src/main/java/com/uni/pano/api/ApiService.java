package com.uni.pano.api;



import com.uni.pano.entity.Result;
import com.uni.pano.entity.ShareInfo;
import com.uni.pano.entity.Token;
import com.uni.pano.entity.VersionInfo;

import java.io.File;

import okhttp3.MultipartBody;
import okhttp3.RequestBody;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.Field;
import retrofit2.http.FormUrlEncoded;
import retrofit2.http.GET;
import retrofit2.http.Multipart;
import retrofit2.http.POST;
import retrofit2.http.Part;
import retrofit2.http.Path;
import retrofit2.http.Query;
import retrofit2.http.Url;


/**
 * @描述：     @客户端api访问接口
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public interface ApiService {

    @Multipart
    @POST(URLConstants.API_ADD_SHARE)
    Call<Result<ShareInfo>> addShare(
            @Part MultipartBody.Part file,
            @Part("title") RequestBody title,
            @Part("content") RequestBody content,
            @Part("appId") RequestBody appId,
            @Part("fileTime") RequestBody fileTime);

    /**
     * 检查版本更新
     * @param appId
     * @param channelId
     * @param versionCode
     * @param versionName
     * @return
     */
    @FormUrlEncoded
    @POST(URLConstants.API_APP_CHECKVERSION)
    Call<Result<VersionInfo>> checkVersion(
            @Field("appId") String appId,
            @Field("channelId") int channelId,
            @Field("versionCode") int versionCode,
            @Field("versionName") String versionName);

    @Multipart
    @POST("share/ffmpeg")
    Call<Result> ffmpeg(@Part MultipartBody.Part file);

    /**
     * 检查app运行状态
     * @param appId
     * @return
     */
    @FormUrlEncoded
    @POST(URLConstants.API_APP_ISRUNNING)
    Call<Result<Boolean>> isRunning(
            @Field("appId") String appId);

    /**
     * 下载apk
     * @param url
     * @return
     */
    @GET
    Call<ResponseBody> downloadAPK(@Url String url);
}
