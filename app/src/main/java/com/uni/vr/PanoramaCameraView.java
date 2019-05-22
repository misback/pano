package com.uni.vr;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.Surface;

import com.uni.pano.config.EnumElement;
import com.uni.pano.event.VideoGLViewCreateEvent;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

public class PanoramaCameraView extends GLSurfaceView {
    public final static String TAG  =   PanoramaCameraView.class.getSimpleName();
    private PanoramaCameraRender mPanoramaCameraRender;
    protected String mFilePath;
    protected AssetManager mAssetManager;
    int videoWidth;
    int videoHeight;
    public PanoramaCameraView(Context context) {
        this(context, null);
    }
    public PanoramaCameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mPanoramaCameraRender = new PanoramaCameraRender(context.getAssets());
        setRenderer(mPanoramaCameraRender);
        initGesture();
    }
    @Override
    public void onResume() {
        super.onResume();
        this.setRenderMode(RENDERMODE_CONTINUOUSLY);
        EventBus.getDefault().register(this);
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.onResume();
            }
        });
    }
    @Override
    public void onPause() {
        EventBus.getDefault().unregister(this);
        this.setRenderMode(RENDERMODE_WHEN_DIRTY);
        super.onPause();
    }

    public void onDestroy(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.onDestroy();
            }
        });
    }
    
    public void runOnGLThread(final Runnable pRunnable) {
        this.queueEvent(pRunnable);
    }
    public void startCamera(Context context){
        mPanoramaCameraRender.startCamera(context);
    }

//    @Subscribe(threadMode = ThreadMode.MAIN)
//    public void onEventMainThread(VideoGLViewCreateEvent videoGLViewCreateEvent) {
//
//        runOnGLThread(new Runnable() {
//            @Override
//            public void run() {
//                SurfaceTexture st = mPanoramaCameraRender.getSurfaceTexture();
//                Surface s = new Surface(st);
//                mPanoramaCameraRender.loadVideo(s, mAssetManager, mFilePath, videoWidth, videoHeight);
//                s.release();
//            }
//        });
//    }
    private EnumElement.RENDER_MODE render_mode = EnumElement.RENDER_MODE.SINGLE;
    public void doUpdateRenderMode(EnumElement.RENDER_MODE renderMode){
        render_mode = renderMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doUpdateRenderMode(render_mode);
            }
        });
    }
    private EnumElement.VIEW_MODE view_mode = EnumElement.VIEW_MODE.FISH;
    public void doUpdateViewMode(EnumElement.VIEW_MODE viewMode){
        view_mode = viewMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doUpdateViewMode(view_mode);
            }
        });
    }
    private EnumElement.OPTION_MODE option_mode = EnumElement.OPTION_MODE.FINGER;
    public void doUpdateOptionMode(EnumElement.OPTION_MODE optionMode){
        option_mode = optionMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doUpdateOptionMode(option_mode);
            }
        });
    }
    public void doSetPlaying(final boolean isPlaying){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doSetPlaying(isPlaying);
            }
        });
    }
    public void doUpdateProgress(final float progress){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doUpdateProgress(progress);
            }
        });
    }
    public void doRestart(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doRestart();
            }
        });
    }
    public void doUpdateLutFilter(final AssetManager assetManager, final String filePath, final EnumElement.FILTER_TYPE filterType){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doUpdateLutFilter(assetManager, filePath, filterType);
            }
        });
    }

    public void doLoadLogoImage(final AssetManager assetManager, final String logoFilePath){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doLoadLogoImage(assetManager, logoFilePath);
            }
        });
    }

    public void doScreenShot(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doScreenShot();
            }
        });
    }

    public void doUpdateLogoAngle(final float angle){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.doUpdateLogoAngle(angle);
            }
        });
    }

    public void startSaveFilter(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.startSaveFilter();
            }
        });
    }

    public void stopSaveFilter(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaCameraRender.stopSaveFilter();
            }
        });
    }
    public void doUpdateSensorRotate(float[] rotationMatrix){
        if(option_mode == EnumElement.OPTION_MODE.GYROSCOPE) {
            mPanoramaCameraRender.doUpdateSensorRotate(rotationMatrix);
        }
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
                mPanoramaCameraRender.doUpdateScale(angle);
                return true;
            }

            @Override
            public boolean onDrag(float yaw, float pitch) {
                mPanoramaCameraRender.doUpdateRotate(yaw, pitch);
                return true;
            }

            @Override
            public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
                mPanoramaCameraRender.doUpdateRotateFling(-velocityX/100.f, -velocityY/100.f);
                return true;
            }

            private void eCallOnClick() {
                if (mOnClickListener != null) {
                    mOnClickListener.onClick(PanoramaCameraView.this);
                }
            }
        }));
    }
}