package com.uni.vr;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.MainApplication;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.ConnectEvent;
import com.uni.pano.event.DisConnectEvent;
import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class CameraVideoView extends GLSurfaceView {
    public final static String TAG  =   CameraVideoView.class.getSimpleName();
    private CameraVideoRender mCameraVideoRender;
    public CameraVideoView(Context context) {
        this(context, null);
    }
    public CameraVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mCameraVideoRender = new CameraVideoRender(context.getAssets());
        setRenderer(mCameraVideoRender);
        initGesture();
    }

    @Override
    public void onResume() {
        super.onResume();
        this.setRenderMode(RENDERMODE_CONTINUOUSLY);
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.onResume();
            }
        });
    }
    @Override
    public void onPause() {
        doStopRecord();
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.onPause();
            }
        });
        this.setRenderMode(RENDERMODE_WHEN_DIRTY);
        super.onPause();
    }
    public void onDestroy(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.onDestroy();
            }
        });
    }
    public void runOnGLThread(final Runnable pRunnable) {
        this.queueEvent(pRunnable);
    }

    private EnumElement.VIEW_MODE view_mode = EnumElement.VIEW_MODE.FISH;
    public void doUpdateViewMode(EnumElement.VIEW_MODE viewMode){
        view_mode = viewMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.doUpdateViewMode(view_mode);
            }
        });

    }

    public void doUpdateLutFilter(final AssetManager assetManager, final String filePath, final EnumElement.FILTER_TYPE filterType, final int useFaceDetector){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.doUpdateLutFilter(assetManager, filePath, filterType, useFaceDetector);
            }
        });
    }

    public void doConnect(final int vendorId, final int productId, final int fileDescriptor, final String usbfs){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.doConnect(vendorId, productId, fileDescriptor, usbfs);
            }
        });
    }
    public void doDisConnect(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.doDisConnect();
            }
        });
    }
    public void doTakePhoto(final int ticks){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.doTakePhoto(ticks);
            }
        });
    }
    public void doUpdateRotate(final float xRotateAngle){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.doUpdateRotate(xRotateAngle);
            }
        });
    }

    public void doStartRecord(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.startRecord();
            }
        });
    }

    public void doStopRecord(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mCameraVideoRender.stopRecord();
            }
        });
    }

    private OnClickListener mOnClickListener;
    @Override
    public void setOnClickListener(OnClickListener l) {
        mOnClickListener = l;
    }
    public void initGesture(){
        setLongClickable(true);
        setOnTouchListener(new TouchTracker(getContext(), new TouchTracker.OnGestureListener() {
            @Override
            public boolean onClick() {
                eCallOnClick();
                return true;
            }

            @Override
            public boolean onScale(float angle) {
                mCameraVideoRender.doUpdateScale(angle);
                return true;
            }

            @Override
            public boolean onDrag(float yaw, float pitch) {
                mCameraVideoRender.doUpdateRotate(yaw, pitch);
                return true;
            }

            @Override
            public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
                mCameraVideoRender.doUpdateRotateFling(-velocityX/100.f, -velocityY/100.f);
                return true;
            }

            private void eCallOnClick() {
                if (mOnClickListener != null) {
                    mOnClickListener.onClick(CameraVideoView.this);
                }
            }
        }));
    }
}