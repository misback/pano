package com.uni.common.file;


import android.os.Looper;
import android.os.Message;

/**
 * Created by ljd on 4/12/16.
 */
public abstract class DownloadProgressHandler extends ProgressHandler{

    /**
     * 下载进度
     */
    public static final int DOWNLOAD_PROGRESS = 1;

    /**
     * 下载完成
     */
    public static final int DOWNLOAD_COMPLETE = 2;

    protected ResponseHandler mHandler = new ResponseHandler(this, Looper.getMainLooper());

    @Override
    protected void sendMessage(FileProgress progressBean) {
        mHandler.obtainMessage(DOWNLOAD_PROGRESS,progressBean).sendToTarget();
    }


    @Override
    protected void handleMessage(Message message){
        switch (message.what){
            //时时更新下载进度
            case DOWNLOAD_PROGRESS:
                FileProgress progressBean = (FileProgress)message.obj;
                onProgress(progressBean.getBytesRead(),progressBean.getContentLength(),progressBean.isDone());
                break;
        }
    }


}
