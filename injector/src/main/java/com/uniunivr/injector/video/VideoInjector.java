package com.uniunivr.injector.video;

/**
 * @描述：     @视频注入工具类
 * @作者：     @蒋诗朋
 * @创建时间： @2016-12-25
 */
public class VideoInjector {

    static {
        System.loadLibrary("MetaVideo");
    }

    public native boolean injectVideo(String fileIn, String fileOut);
}
