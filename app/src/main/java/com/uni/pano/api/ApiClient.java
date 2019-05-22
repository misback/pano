package com.uni.pano.api;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.uni.common.util.NetUtil;
import com.uni.common.util.NetWorkUtil;
import com.uni.common.util.UriUtil;
import com.uni.pano.BuildConfig;
import com.uni.pano.MainApplication;

import java.io.File;
import java.io.IOException;
import java.net.CookieManager;
import java.net.CookiePolicy;
import java.util.concurrent.TimeUnit;

import okhttp3.Cache;
import okhttp3.CacheControl;
import okhttp3.Interceptor;
import okhttp3.JavaNetCookieJar;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.logging.HttpLoggingInterceptor;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

/**
 * Created by DELL on 2017/3/4.
 */

public class ApiClient {
    public static Retrofit retrofit = null;
    public static Retrofit retrofit() {
        if (retrofit == null) {
            OkHttpClient.Builder builder = new OkHttpClient.Builder();
            /**
             *设置缓存，代码略
             */
            final File chachefile = new File(MainApplication.getInstance().getExternalCacheDir(), "HttpCache");
            final Cache cache = new Cache(chachefile, 1024 * 1024 * 50);//缓存文件
            Interceptor cacheInterceptor = new Interceptor() {
                @Override
                public Response intercept(Chain chain) throws IOException {
                    Request request = chain.request();
                    if (!NetWorkUtil.isNetConnected(MainApplication.getInstance())) {
                        request = request.newBuilder()
                                .cacheControl(CacheControl.FORCE_CACHE)
                                .build();
                    }
                    Response response = chain.proceed(request);
                    if (NetWorkUtil.isNetConnected(MainApplication.getInstance())) {
                        int maxAge = 0;

                        // 有网络时 设置缓存超时时间0个小时
                        response.newBuilder()
                                .header("Cache-Control", "public, max-age=" + maxAge)
                                .removeHeader("Pragma")// 清除头信息，因为服务器如果不支持，会返回一些干扰信息，不清除下面无法生效
                                .build();
                    } else {
                        //无网络时，设置超时为4周
                        int maxStale = 60 * 60 * 24 * 28;
                        response.newBuilder()
                                .header("Cache-Control", "public, only-if-cached, max-stale=" + maxStale)
                                .removeHeader("Pragma")
                                .build();
                    }

                    return response;
                }
            };
            //builder.cache(cache).addInterceptor(cacheInterceptor);
            /**
             *  公共参数，代码略
             */
//            Interceptor addQueryParameterInterceptor = new Interceptor() {
//                @Override
//                public Response intercept(Chain chain) throws IOException {
//                    Request originalRequest = chain.request();
//                    Request request;
//                    String method = originalRequest.method();
//                    Headers headers = originalRequest.headers();
//                    HttpUrl modifiedUrl = originalRequest.url().newBuilder()
//                            // Provide your custom parameter here
//                            .addQueryParameter("platform", "adnroid")
//                            .addQueryParameter("version", "1.2.0")
//                            .build();
//                    request = originalRequest.newBuilder().url(modifiedUrl).build();
//                    return chain.proceed(request);
//                }
//            };
//            //公共参数
//            builder.addInterceptor(addQueryParameterInterceptor);
            /**
             * 设置头，代码略
             */
            Interceptor headerInterceptor = new Interceptor() {

                @Override
                public Response intercept(Chain chain) throws IOException {
                    Request orignaRequest = chain.request();
                    Request request = orignaRequest.newBuilder()
                            .header("clientOS",URLConstants.getClientOs())
                            .header("clientVersion", URLConstants.getClientVersion())
                            .header("clientVersionCode", URLConstants.getClientVersionCode())
                            .header("MAC",URLConstants.getMac())
                            //.header("Content-Type", "application/x-www-form-urlencoded")
                            //.header("Content-Type", "Multipart/form-data")
                            //.method(orignaRequest.method(), orignaRequest.body())
                            .build();

                    return chain.proceed(request);
                }
            };
            builder.addInterceptor(headerInterceptor);
            /**
             * Log信息拦截器，代码略
             */
            if (BuildConfig.DEBUG) {
                //log信息拦截器
                HttpLoggingInterceptor httpLoggingInterceptor = new HttpLoggingInterceptor();
                httpLoggingInterceptor.setLevel(HttpLoggingInterceptor.Level.BODY);
                //设置Debug Log模式
                //builder.addInterceptor(httpLoggingInterceptor);
            }
            /**
             * 设置cookie，代码略
             */
            CookieManager cookieManager = new CookieManager();
            cookieManager.setCookiePolicy(CookiePolicy.ACCEPT_ALL);
            builder.cookieJar(new JavaNetCookieJar(cookieManager));
            /**
             * 设置超时和重连，代码略
             */
            builder.connectTimeout(15, TimeUnit.SECONDS);
            builder.readTimeout(20, TimeUnit.SECONDS);
            builder.writeTimeout(20, TimeUnit.SECONDS);
            //错误重连
            builder.retryOnConnectionFailure(true);
            //以上设置结束，才能build(),不然设置白搭
            OkHttpClient okHttpClient = builder.build();
            Gson gson = new GsonBuilder()
                    //配置你的Gson
                    .setDateFormat("yyyy-MM-dd hh:mm:ss")
                    .create();

            retrofit = new Retrofit.Builder()
                    .baseUrl(URLConstants.getBaseUrl())
                    //设置Json转换器
                    .addConverterFactory(GsonConverterFactory.create(gson))
                    //RxJava适配器
                    //.addCallAdapterFactory(RxJavaCallAdapterFactory.create())
                    .client(okHttpClient)
                    .build();
        }
        return retrofit;
    }


}