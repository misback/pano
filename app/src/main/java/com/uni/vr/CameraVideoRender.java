package com.uni.vr;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLDisplay;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.view.Surface;

import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.MainApplication;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.ConnectEvent;
import com.uni.pano.event.DisConnectEvent;
import com.uni.pano.event.RecordTimeEvent;
import com.uni.pano.event.TakePhotoEvent;
import com.uni.vr.encoder.AudioMediaEncoder;
import com.uni.vr.encoder.MediaEncoder;
import com.uni.vr.encoder.MediaMuxerWrapper;
import com.uni.vr.encoder.RenderHandler;
import com.uni.vr.encoder.VideoMediaEncoder;

import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by DELL on 2017/3/13.
 */

public class CameraVideoRender implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener, RenderHandler.DrawInterface{
    private final static String TAG 				= CameraVideoRender.class.getSimpleName();
    private long mObjPtr = 0L;
    private final static int FPS                 = 25;
    private final static int TIME_SCALE         = 90000;
    private final static long NANOSECONDSPERSECOND = 1000000000L;
    private final static long NANOSECONDSPERMICROSECOND = 1000000;

    private static long sAnimationInterval = (long) (1.0 / 30 * NANOSECONDSPERSECOND);

    // ===========================================================
    // Fields
    // ===========================================================
    private long mLastTickInNanoSeconds;

    private final static int PHOTO_WIDTH        = 3008;
    private final static int PHOTO_HEIGHT       = 1504;
    private final static int H264_WIDTH         = 1920;
    private final static int H264_HEIGHT        = 960;
    private final static int BIT_RATE           = 4000000;
    private SurfaceTexture mSurface               =   null;
    private volatile boolean updateSurface = false;
    private int width;
    private int height;
    private boolean bDestroy = false;
    private MediaMuxerWrapper mediaMuxerWrapper;
    private volatile boolean isVideoRecording = false;
    private EGLConfig eglConfig;
    private Semaphore semaphoreWriteProtect = new Semaphore(0);  //创建一个信号量
    private volatile int requestConnect = 0;
    private volatile boolean hasConnect = false;
    public CameraVideoRender(AssetManager assetManager){
        mObjPtr = nativeOnCreate(PathConfig.getMediaFileSaveDir(), PathConfig.getFaceDir()+ File.separator + "seeta_fd_frontal_v1.0.bin", PathConfig.getFaceDir()+ File.separator + "seeta_fa_v1.1.bin" , 100, 1.0f, MainApplication.getInstance(), assetManager);
        String logoFilePath = PreferenceModel.getString("logoFileName", "logo_none.png");
        if(logoFilePath.lastIndexOf("/") == -1){
            nativeLoadLogoImage(mObjPtr, assetManager, logoFilePath);
        }else {
            if(FileUtil.isFileExist(logoFilePath)) {
                nativeLoadLogoImage(mObjPtr, null, logoFilePath);
            }else{
                PreferenceModel.putString("logoFileName", "logo_none.png");
                nativeLoadLogoImage(mObjPtr, assetManager, "logo_none.png");
            }
        }
        int zoom = PreferenceModel.getInt("ZOOM", 15);
        nativeUpdateLogoAngle(mObjPtr, zoom);
    }
    @Override
    public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
        EGLDisplay mEGLDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        int[] version = new int[2];
        EGL14.eglInitialize(mEGLDisplay, version, 0, version, 1);
        android.opengl.EGLConfig eglConfig = null;
        int[] configsCount = new int[1];
        android.opengl.EGLConfig[] configs = new android.opengl.EGLConfig[1];
        int[] configSpec = new int[]{
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                EGL14.EGL_DEPTH_SIZE, 0,
                EGL14.EGL_STENCIL_SIZE, 0,
                EGL14.EGL_NONE
        };
        if (!EGL14.eglChooseConfig(mEGLDisplay, configSpec, 0, configs, 0, configs.length, configsCount, 0)) {
            throw new IllegalArgumentException("Failed to choose config: " + GLUtils.getEGLErrorString(EGL14.eglGetError()));
        } else if (configsCount[0] > 0) {
            eglConfig = configs[0];
        }
        nativeInitGL(mObjPtr, H264_WIDTH, H264_HEIGHT, PHOTO_WIDTH, PHOTO_HEIGHT, this, TIME_SCALE, FPS);
        mSurface = new SurfaceTexture(nativeGetTextureId(mObjPtr));
        mSurface.setOnFrameAvailableListener(this);

        synchronized(this) {
            updateSurface = false;
        }
        Surface s = new Surface(mSurface);
        nativeSetSurface(mObjPtr,s);
        s.release();
        this.eglConfig = config;
        semaphoreWriteProtect.release();
    }
    @Override
    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        if(bDestroy){
            return;
        }
        this.width = width;
        this.height = height;
        nativeOnSurfaceChanged(mObjPtr, width, height);
    }
    public void onResume(){
        nativeOnResume(mObjPtr);
    }
    public void onPause(){
        nativeOnPause(mObjPtr);
    }
    private void draw(){
        nativeDrawFrame(mObjPtr);
        synchronized (this) {
            if (isVideoRecording && mediaMuxerWrapper != null) {
                mediaMuxerWrapper.frameAvailableSoon();
            }
        }
    }
    @Override
    public void onDrawFrame(GL10 glUnused) {
        if(bDestroy || mObjPtr == 0L){
            return;
        }
        synchronized(this) {
            if (updateSurface) {
                mSurface.updateTexImage();
                updateSurface = false;
            }
        }
        if (sAnimationInterval <= 1.0 / 60 * NANOSECONDSPERSECOND) {
            draw();
        } else {
            final long now = System.nanoTime();
            final long interval = now - this.mLastTickInNanoSeconds;

            if (interval < sAnimationInterval) {
                try {
                    Thread.sleep((sAnimationInterval - interval) / NANOSECONDSPERMICROSECOND);
                } catch (final Exception e) {
                }
            }
            this.mLastTickInNanoSeconds = System.nanoTime();
            draw();
        }
    }

    public void onDestroy(){
        bDestroy = true;
        if (mediaMuxerWrapper!=null) {
            mediaMuxerWrapper.stopRecording();
            mediaMuxerWrapper = null;
        }
        if(mObjPtr != 0L) {
            nativeOnDestroy(mObjPtr);
            mObjPtr = 0L;
        }
    }
    @Override
    synchronized public void onFrameAvailable(SurfaceTexture surface) {
        if(bDestroy || mObjPtr == 0L){
            return;
        }
        updateSurface = true;
    }
    VideoMediaEncoder videoMediaEncoder;
    String outFilePath;
    public void startRecord(){
        try {
            if (mediaMuxerWrapper == null) {
                outFilePath = PathConfig.getMediaFolder() + File.separator + new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss").format(new Date()) + ".mp4";
                new File(outFilePath);
                mediaMuxerWrapper = new MediaMuxerWrapper(outFilePath, mediaMuxerWrapperListener);
                videoMediaEncoder = new VideoMediaEncoder(mediaMuxerWrapper, mediaEncoderListener, H264_WIDTH, H264_HEIGHT, BIT_RATE);
                mediaMuxerWrapper.addEncoder(videoMediaEncoder, new AudioMediaEncoder(mediaMuxerWrapper, mediaEncoderListener, true));
                mediaMuxerWrapper.prepare();
                mediaMuxerWrapper.startRecording();
            }
        }catch (Exception e){

        }
    }

    public void stopRecord(){
        if (mediaMuxerWrapper != null && mediaMuxerWrapper.isStarted()) {
            isVideoRecording =   false;
            mediaMuxerWrapper.stopRecording();
        }
    }

    private final MediaMuxerWrapper.MediaMuxerWrapperListener mediaMuxerWrapperListener = new MediaMuxerWrapper.MediaMuxerWrapperListener() {
        @Override
        public void onStarted() {
            EventBus.getDefault().post(new RecordTimeEvent(EnumElement.RECORD_STATE.START));
        }
        @Override
        public void onStopped() {
            EventBus.getDefault().post(new RecordTimeEvent(EnumElement.RECORD_STATE.STOP, outFilePath));
            mediaMuxerWrapper = null;
        }

        @Override
        public void onUpdateTime(long seconds) {
            EventBus.getDefault().post(new RecordTimeEvent(EnumElement.RECORD_STATE.UPDATE_TIME, seconds));
        }
    };

    private final MediaEncoder.MediaEncoderListener mediaEncoderListener = new MediaEncoder.MediaEncoderListener() {
        @Override
        public void onPrepared(MediaEncoder encoder) {
            if (encoder instanceof VideoMediaEncoder) {
                isVideoRecording =   true;
                videoMediaEncoder.setEglContext(EGL14.eglGetCurrentContext(), CameraVideoRender.this);
            }
        }

        @Override
        public void onStopped(MediaEncoder encoder) {
            if (encoder instanceof VideoMediaEncoder) {
                isVideoRecording = false;
            }
        }
    };
    public void callbackTakePhoto(int result, String filePath){
        EventBus.getDefault().post(new TakePhotoEvent(EnumElement.TAKE_PHOTO_RESULT.values()[result], filePath));
    }
    public void doConnect(final int vendorId, final int productId, final int fileDescriptor, final String usbfs){
        if(requestConnect < 1 && !hasConnect) {
            requestConnect = 1;
            ExecutorService singleThreadExecutor = Executors.newSingleThreadExecutor();
            singleThreadExecutor.execute(new Runnable() {
                @Override
                public void run() {
                    while (true) {
                        try {
                            semaphoreWriteProtect.acquire();
                            if (requestConnect == 1){
                                int connectResult = nativeConnect(mObjPtr, vendorId, productId, fileDescriptor, usbfs, null);
                                EventBus.getDefault().post(new ConnectEvent(EnumElement.CONNECT_RESULT.values()[connectResult]));
                                hasConnect = (EnumElement.CONNECT_RESULT.SUCCESS == EnumElement.CONNECT_RESULT.values()[connectResult]);
                            }
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        semaphoreWriteProtect.release();
                        break;
                    }
                }
            });
            singleThreadExecutor.shutdown();
        }
    }
    public void doDisConnect(){
        if(requestConnect == 1){
            requestConnect = 0;
            semaphoreWriteProtect.release();
        }
        if(hasConnect){
            int result = nativeDisConnect(mObjPtr);
            EventBus.getDefault().post(new DisConnectEvent(EnumElement.DISCONNECT_RESULT.values()[result]));
            if(EnumElement.DISCONNECT_RESULT.SUCCESS == EnumElement.DISCONNECT_RESULT.values()[result]){
                hasConnect = false;
            }
            stopRecord();
        }
    }

    public void doUpdateViewMode(EnumElement.VIEW_MODE viewMode){
        nativeUpdateViewMode(mObjPtr, viewMode.ordinal());
    }

    public void doUpdateLutFilter(final AssetManager assetManager, final String filePath, final EnumElement.FILTER_TYPE filterType, final int useFaceDetector){
        nativeUpdateLutFilter(mObjPtr, assetManager, filePath, filterType.ordinal(), useFaceDetector);
    }

    public void doTakePhoto(final int ticks){
        nativeTakePhoto(mObjPtr, ticks);
    }
    public void doUpdateRotate(final float xRotateAngle){
        nativeUpdateRotate(mObjPtr, xRotateAngle, 0.f);
    }
    public void doUpdateScale(float angle){
        nativeUpdateScale(mObjPtr, angle);
    }
    public void doUpdateRotate(float yaw, float pitch){
        nativeUpdateRotate(mObjPtr, yaw, pitch);
    }
    public void doUpdateRotateFling(float velocityX, float velocityY){
        nativeUpdateRotateFling(mObjPtr, velocityX, velocityY);
    }

    /** Native methods, implemented in jni folder */
    private static native long nativeOnCreate(String mediaDir, String tDetectModelPath, String tAlignModelPath, int sphereVertexNum, float sphereRadius, Context context, AssetManager assetMgr);
    private static native void nativeOnDestroy(long objPtr);
    private static native void nativeLoadLogoImage(long objPtr, AssetManager assetManager, String filePath);
    //0:开始拍照,1:没有连接相机,2:正在拍照中,3:空间不够...  tickCount为连拍次数
    private static native void nativeTakePhoto(long objPtr, int tickCount);
    private static native void nativeUpdateScale(long objPtr, float scale);
    private static native void nativeUpdateRotate(long objPtr, float xMove, float yMove);
    private static native void nativeUpdateRotateFling(long objPtr, float velocityX, float velocityY);
    private static native void nativeUpdateLutFilter(long objPtr, AssetManager assetManager, String filePath, int filterType, int useFaceDetector);
    private static native void nativeUpdateViewMode(long objPtr, int viewMode);
    private static native void nativeUpdateLogoAngle(long objPtr, float angle);
    private static native void nativeUpdateEffect(long objPtr, String effectFileName, int useFaceDetector);

    private static native int  nativeGetTextureId(long objPtr);
    private static native void nativeInitGL(long objPtr, int renderWidth, int renderHeight, int takePhotoWidth, int takePhotoHeight, CameraVideoRender cameraVideoRender, int nTimeScale, int nFrameRate);
    //0:相机连接不成,1:相机连接成功
    private static native int nativeConnect(long objPtr, final int vendorId, final int productId, final int fileDescriptor, String usbfs, Surface surface);
    private static native int nativeDisConnect(long objPtr);
    private static native void nativeOnSurfaceChanged(long objPtr, int pNewSurfaceWidth, int pNewSurfaceHeight);
    private static native void nativeSetSurface(long objPtr, Surface surface);
    private static native void nativeCallBackFaceDetector(long objPtr, int width, int height, float x, float y, float eyesDistance);
    private static native void nativeDrawPanoramaFrame(long objPtr);
    private static native void nativeDrawFrame(long objPtr);
    private static native void nativeOnResume(long objPtr);
    private static native void nativeOnPause(long objPtr);

    @Override
    public void render() {
        nativeDrawPanoramaFrame(mObjPtr);
    }
}
