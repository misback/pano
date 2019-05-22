package com.uni.common.glide.config;

/**
 * @描述：     @glide缓存配置文件
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class GlideCacheConfig {

    // 图片缓存最大容量，150M，根据自己的需求进行修改
    public static final int   GLIDE_CACHE_SIZE = 150 * 1024 * 1024;

    // 图片缓存子目录
    public static final String GLIDE_CACHE_DIR = "image_cache";

}
