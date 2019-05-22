package com.uni.common.file;

import android.os.Looper;
import android.os.Message;

/**
 * Created by ljd on 4/18/16.
 */
public abstract class UploadProgressHandler extends ProgressHandler{

    private static final int UPLOAD_PROGRESS = 0;
    protected ResponseHandler mHandler = new ResponseHandler(this, Looper.getMainLooper());

    @Override
    protected void sendMessage(FileProgress progressBean) {
        mHandler.obtainMessage(UPLOAD_PROGRESS,progressBean).sendToTarget();

    }

    @Override
    protected void handleMessage(Message message){
        switch (message.what){
            case UPLOAD_PROGRESS:
                FileProgress progressBean = (FileProgress)message.obj;
                onProgress(progressBean.getBytesRead(),progressBean.getContentLength(),progressBean.isDone());
        }
    }

}
