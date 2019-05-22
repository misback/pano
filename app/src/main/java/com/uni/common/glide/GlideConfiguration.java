package com.uni.common.glide;

import android.content.Context;

import com.bumptech.glide.Glide;
import com.bumptech.glide.GlideBuilder;
import com.bumptech.glide.MemoryCategory;
import com.bumptech.glide.load.DecodeFormat;
import com.bumptech.glide.load.engine.bitmap_recycle.LruBitmapPool;
import com.bumptech.glide.load.engine.cache.ExternalCacheDiskCacheFactory;
import com.bumptech.glide.load.engine.cache.LruResourceCache;
import com.bumptech.glide.load.engine.cache.MemorySizeCalculator;
import com.bumptech.glide.module.GlideModule;
import com.uni.common.glide.config.GlideCacheConfig;
import com.uni.common.image.SmartImageSizeModel;
import com.uni.common.image.SmartImageSizeModelFactory;


import java.io.InputStream;

/**
 * @描述：     @缓存配置
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class GlideConfiguration implements GlideModule {

    @Override
    public void applyOptions(Context context, GlideBuilder builder) {
        builder.setDecodeFormat(DecodeFormat.PREFER_RGB_565);
        MemorySizeCalculator calculator = new MemorySizeCalculator(context);
        builder.setMemoryCache(new LruResourceCache(calculator.getMemoryCacheSize()));
        builder.setBitmapPool(new LruBitmapPool(calculator.getBitmapPoolSize()));
        builder.setDiskCache(new ExternalCacheDiskCacheFactory(context,
                GlideCacheConfig.GLIDE_CACHE_DIR, GlideCacheConfig.GLIDE_CACHE_SIZE));
    }

    @Override
    public void registerComponents(Context context, Glide glide) {
        glide.setMemoryCategory(MemoryCategory.HIGH);
        glide.register(SmartImageSizeModel.class, InputStream.class, new SmartImageSizeModelFactory());
    }
}
