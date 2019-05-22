package com.uni.pano.api;


import com.uni.common.config.PathConfig;
import com.uni.common.file.ProgressHandler;
import com.uni.common.file.ProgressHelper;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import okhttp3.OkHttpClient;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.adapter.rxjava.RxJavaCallAdapterFactory;
import retrofit2.converter.gson.GsonConverterFactory;

/**
 * Created by terrence on 17-6-6.
 */

public class DownloadManage {


    public static final void downloadAPK(String baseUrl,String downloadUrl,
                                         final String newVersionName,
                                         final ProgressHandler progressHandler){
        Retrofit.Builder retrofitBuilder = new Retrofit.Builder()
                .addCallAdapterFactory(RxJavaCallAdapterFactory.create())
                .addConverterFactory(GsonConverterFactory.create())
                .baseUrl(baseUrl);
        OkHttpClient.Builder builder = ProgressHelper.addProgress(null);
        ApiService retrofit = retrofitBuilder
                .client(builder.build())
                .build().create(ApiService.class);
        ProgressHelper.setProgressHandler(progressHandler);
        Call<ResponseBody> call   = retrofit.downloadAPK(downloadUrl);
        call.enqueue(new Callback<ResponseBody>() {
            @Override
            public void onResponse(Call<ResponseBody> call, Response<ResponseBody> response) {
                try {
                    InputStream is          = response.body().byteStream();
                    File file               = new File(PathConfig.getTempDownloadDir(),newVersionName);
                    if(!file.getParentFile().exists()){
                        file.getParentFile().mkdirs();
                    }
                    FileOutputStream fos    = new FileOutputStream(file);
                    BufferedInputStream bis = new BufferedInputStream(is);
                    final byte[] buffer     = new byte[1024];
                    int len;
                    while ((len = bis.read(buffer)) != -1) {
                        fos.write(buffer, 0, len);
                        fos.flush();
                    }
                    fos.close();
                    bis.close();
                    is.close();
                    progressHandler.onComplete(file.getPath());
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onFailure(Call<ResponseBody> call, Throwable t) {
                progressHandler.onFailure();
            }
        });

    }
}
