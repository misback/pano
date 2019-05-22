package com.uni.pano.api;

/**
 * @描述：     @文件下载监听
 * @作者：     @蒋诗朋
 * @创建时间： @2017-05-10
 */
public interface ProgressListener {
    /**
     * @param progress     已经下载或上传字节数
     * @param total        总字节数
     * @param done         是否完成
     */
    void onProgress(long progress, long total, boolean done);
}