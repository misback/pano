package com.uni.vr;

import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.provider.MediaStore;
import android.util.AttributeSet;
import android.view.MotionEvent;

import com.umeng.socialize.utils.Log;
import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.ScreenShotEvent;

import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.io.FileNotFoundException;

public class PanoramaImageView extends GLSurfaceView {
    public final static String TAG  =   PanoramaImageView.class.getSimpleName();
    private PanoramaImageRender mPanoramaRender;
    protected String mFilePath;
    protected AssetManager mAssetManager;
    public PanoramaImageView(Context context) {
        this(context, null);
    }
    public PanoramaImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mPanoramaRender = new PanoramaImageRender(context.getAssets());
        setRenderer(mPanoramaRender);
        initGesture();
    }
    @Override
    public void onPause() {
        mPanoramaRender.onPause();
        this.setRenderMode(RENDERMODE_WHEN_DIRTY);
        super.onPause();
    }
    @Override
    public void onResume() {
        super.onResume();
        this.setRenderMode(RENDERMODE_CONTINUOUSLY);
        mPanoramaRender.onResume();
    }
    public void onDestroy(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.onDestroy();
            }
        });
    }

    public void runOnGLThread(final Runnable pRunnable) {
        this.queueEvent(pRunnable);
    }
    public void loadImage(String filePath, AssetManager assetManager){
        mFilePath       =   filePath;
        mAssetManager   =   assetManager;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.loadImage(mFilePath, mAssetManager);
            }
        });
    }
    private EnumElement.RENDER_MODE render_mode = EnumElement.RENDER_MODE.SINGLE;
    public void doUpdateRenderMode(EnumElement.RENDER_MODE renderMode){
        render_mode = renderMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doUpdateRenderMode(render_mode);
            }
        });
    }
    private EnumElement.VIEW_MODE view_mode = EnumElement.VIEW_MODE.FISH;
    public void doUpdateViewMode(EnumElement.VIEW_MODE viewMode){
        view_mode = viewMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doUpdateViewMode(view_mode);
            }
        });

    }
    private EnumElement.OPTION_MODE option_mode = EnumElement.OPTION_MODE.FINGER;
    public void doUpdateOptionMode(EnumElement.OPTION_MODE optionMode){
        option_mode = optionMode;
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doUpdateOptionMode(option_mode);
            }
        });
    }
    public void doUpdateLutFilter(final AssetManager assetManager, final String filePath, final EnumElement.FILTER_TYPE filterType){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doUpdateLutFilter(assetManager, filePath, filterType);
            }
        });
    }

    public void doLoadLogoImage(final AssetManager assetManager, final String logoFilePath){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doLoadLogoImage(assetManager, logoFilePath);
            }
        });
    }

    public void doScreenShot(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doScreenShot();
            }
        });
    }

    public void doSaveFilterImage(){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doSaveFilterImage();
            }
        });
    }

    public void doUpdateLogoAngle(final float angle){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doUpdateLogoAngle(angle);
            }
        });
    }

    public void doUpdateRotate(final float rotateX, final float rotateY, final boolean bNotAnimation){
        runOnGLThread(new Runnable() {
            @Override
            public void run() {
                mPanoramaRender.doUpdateRotate(rotateX, rotateY, bNotAnimation);
            }
        });
    }
    public void doUpdateSensorRotate(float[] rotationMatrix){
        if(option_mode == EnumElement.OPTION_MODE.GYROSCOPE) {
            mPanoramaRender.doUpdateSensorRotate(rotationMatrix);
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
                mPanoramaRender.doUpdateScale(angle);
                return true;
            }

            @Override
            public boolean onDrag(float yaw, float pitch) {
                mPanoramaRender.doUpdateRotate(yaw, pitch, 0);
                return true;
            }

            @Override
            public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
                mPanoramaRender.doUpdateRotateFling(-velocityX/100.f, -velocityY/100.f);
                return true;
            }

            private void eCallOnClick() {
                if (mOnClickListener != null) {
                    mOnClickListener.onClick(PanoramaImageView.this);
                }
            }
        }));
    }
}