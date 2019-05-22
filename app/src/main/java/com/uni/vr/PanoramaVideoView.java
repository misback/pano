package com.uni.vr;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.Surface;

import com.uni.pano.config.EnumElement;
import com.uni.pano.event.VideoGLViewCreateEvent;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

public class PanoramaVideoView extends GLSurfaceView {
    public final static String TAG  =   PanoramaVideoView.class.getSimpleName();
    private PanoramaVideoRender mPanoramaVideoRender;
    protected String mFilePath;
    protected AssetManager mAssetManager;
    int videoWidth;
    int videoHeight;
    public PanoramaVideoView(Context context) {
        this(context, null);
    }
    public PanoramaVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mPanoramaVideoRender = new PanoramaVideoRender(context.getAssets());
        setRenderer(mPanoramaVideoRender);
        initGesture();
    }
    @Override
    public void onResume() {
        super.onResume();
        this.setRenderMode(RENDERMODE_CONTINUOUSLY);
        EventBus.getDefault().register(this);
        doSetPlaying(true);
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.onResume();
            }
        });
    }
    @Override
    public void onPause() {
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.onPause();
            }
        });
        doSetPlaying(false);
        EventBus.getDefault().unregister(this);
        this.setRenderMode(RENDERMODE_WHEN_DIRTY);
        super.onPause();
    }

    public void onDestroy(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.onDestroy();
            }
        });
    }
    public void registerOnProgressCallBack(final Activity activity){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.registerOnProgressCallBack(activity);
            }
        });
    }
    public void loadVideo(final String filePath, final AssetManager assetManager, final int width, final int height){
        mFilePath       =   filePath;
        mAssetManager   =   assetManager;
        videoWidth = width;
        videoHeight = height;
        final SurfaceTexture st = mPanoramaVideoRender.getSurfaceTexture();
        if(st != null){
            runOnGLThread(new Runnable() {
                @Override
                public void run() {
                    Surface s = new Surface(st);
                    mPanoramaVideoRender.loadVideo(s, mAssetManager, mFilePath, width, height);
                    s.release();
                }
            });
        }
    }
    public void runOnGLThread(final Runnable pRunnable) {
        this.queueEvent(pRunnable);
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onEventMainThread(VideoGLViewCreateEvent videoGLViewCreateEvent) {

        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                SurfaceTexture st = mPanoramaVideoRender.getSurfaceTexture();
                Surface s = new Surface(st);
                mPanoramaVideoRender.loadVideo(s, mAssetManager, mFilePath, videoWidth, videoHeight);
                s.release();
            }
        });
    }
    private EnumElement.RENDER_MODE render_mode = EnumElement.RENDER_MODE.SINGLE;
    public void doUpdateRenderMode(EnumElement.RENDER_MODE renderMode){
        render_mode = renderMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doUpdateRenderMode(render_mode);
            }
        });
    }
    private EnumElement.VIEW_MODE view_mode = EnumElement.VIEW_MODE.FISH;
    public void doUpdateViewMode(EnumElement.VIEW_MODE viewMode){
        view_mode = viewMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doUpdateViewMode(view_mode);
            }
        });
    }
    private EnumElement.OPTION_MODE option_mode = EnumElement.OPTION_MODE.FINGER;
    public void doUpdateOptionMode(EnumElement.OPTION_MODE optionMode){
        option_mode = optionMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doUpdateOptionMode(option_mode);
            }
        });
    }
    public void doSetPlaying(final boolean isPlaying){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doSetPlaying(isPlaying);
            }
        });
    }
    public void doUpdateProgress(final float progress){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doUpdateProgress(progress);
            }
        });
    }
    public void doRestart(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doRestart();
            }
        });
    }
    public void doUpdateLutFilter(final AssetManager assetManager, final String filePath, final EnumElement.FILTER_TYPE filterType){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doUpdateLutFilter(assetManager, filePath, filterType);
            }
        });
    }

    public void doLoadLogoImage(final AssetManager assetManager, final String logoFilePath){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doLoadLogoImage(assetManager, logoFilePath);
            }
        });
    }

    public void doScreenShot(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doScreenShot();
            }
        });
    }

    public void doUpdateLogoAngle(final float angle){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.doUpdateLogoAngle(angle);
            }
        });
    }

    public void startSaveFilter(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.startSaveFilter();
            }
        });
    }

    public void stopSaveFilter(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaVideoRender.stopSaveFilter();
            }
        });
    }
    public void doUpdateSensorRotate(float[] rotationMatrix){
        if(option_mode == EnumElement.OPTION_MODE.GYROSCOPE) {
            mPanoramaVideoRender.doUpdateSensorRotate(rotationMatrix);
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
                mPanoramaVideoRender.doUpdateScale(angle);
                return true;
            }

            @Override
            public boolean onDrag(float yaw, float pitch) {
                mPanoramaVideoRender.doUpdateRotate(yaw, pitch);
                return true;
            }

            @Override
            public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
                mPanoramaVideoRender.doUpdateRotateFling(-velocityX/100.f, -velocityY/100.f);
                return true;
            }

            private void eCallOnClick() {
                if (mOnClickListener != null) {
                    mOnClickListener.onClick(PanoramaVideoView.this);
                }
            }
        }));
    }
}