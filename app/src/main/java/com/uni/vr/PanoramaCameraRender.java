package com.uni.vr;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.media.ImageReader;
import android.opengl.EGL14;
import android.opengl.EGLDisplay;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.MainApplication;
import com.uni.pano.activities.CameraActivity;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.RecordTimeEvent;
import com.uni.pano.event.ScreenShotEvent;
import com.uni.pano.event.VideoGLViewCreateEvent;
import com.uni.vr.encoder.AudioMediaEncoder;
import com.uni.vr.encoder.MediaEncoder;
import com.uni.vr.encoder.MediaMuxerWrapper;
import com.uni.vr.encoder.RenderHandler;
import com.uni.vr.encoder.VideoMediaEncoder;

import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;


/**
 * Created by DELL on 2017/3/13.
 */

public class PanoramaCameraRender implements GLSurfaceView.Renderer, RenderHandler.DrawInterface{
    private final static String TAG 				= 	PanoramaCameraRender.class.getSimpleName();
    private SurfaceTexture mSurfaceTexture1             =   null;
    private SurfaceTexture mSurfaceTexture2             =   null;
    private Surface mSurface1             =   null;
    private Surface mSurface2             =   null;
    private long mObjPtr = 0L;
    private volatile boolean updateSurface1 = false;
    private volatile boolean updateSurface2 = false;
    private MediaMuxerWrapper mediaMuxerWrapper;
    private volatile boolean isVideoRecording = false;
    private final static long NANOSECONDSPERSECOND = 1000000000L;
    private final static long NANOSECONDSPERMICROSECOND = 1000000;
    private final static int BIT_RATE           = 4000000;
    private static long sAnimationInterval = (long) (1.0 / 30 * NANOSECONDSPERSECOND);
    private long mLastTickInNanoSeconds;
    private float width;
    private float height;
    int videoWidth;
    int videoHeight;
    private boolean bDestroy    =   false;
    public PanoramaCameraRender(AssetManager assetManager){
        mObjPtr = nativeOnCreate(MainApplication.getInstance(), PathConfig.getMediaFileSaveDir(), this);
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
        nativeInitGL(mObjPtr, width,height);
        mSurfaceTexture1 = new SurfaceTexture(nativeGetTextureId1(mObjPtr));
        mSurfaceTexture1.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                if(bDestroy || mObjPtr == 0L){
                    return;
                }
                updateSurface1 = true;
            }
        });
        mSurfaceTexture2 = new SurfaceTexture(nativeGetTextureId2(mObjPtr));
        mSurfaceTexture2.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                if(bDestroy || mObjPtr == 0L){
                    return;
                }
                updateSurface2 = true;
            }
        });
        synchronized(this) {
            updateSurface1 = false;
            updateSurface2 = false;
        }
        mSurface1 = new Surface(mSurfaceTexture1);
        mSurface2 = new Surface(mSurfaceTexture2);
        EventBus.getDefault().post(new VideoGLViewCreateEvent());
    }
    public void onDestroy(){
        bDestroy = true;
        stopSaveFilter();
        if (mediaMuxerWrapper!=null) {
            mediaMuxerWrapper.stopRecording();
            mediaMuxerWrapper = null;
        }
        if (mObjPtr != 0L) {
            nativeOnDestroy(mObjPtr);
            mObjPtr = 0L;
        }
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
    private void draw(){
        nativeDrawFrame(mObjPtr);
       // Log.e("gae", "1111111111111");
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
            if (updateSurface1) {
                mSurfaceTexture1.updateTexImage();
                updateSurface1 = false;
            }
            if (updateSurface2) {
                mSurfaceTexture2.updateTexImage();
                updateSurface2 = false;
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
    //1:YES 0:NO
    public void callback(final String fileName, int isScreen){
        if(isScreen == 1) {
            EventBus.getDefault().post(new ScreenShotEvent(fileName));
        }else{

        }
    }
    public void callbackAudioBuffer(ByteBuffer byteBuffer, int size){
        if (isVideoRecording && mediaMuxerWrapper!=null) {
            audioMediaEncoder.filterEncode(byteBuffer, size);
        }
    }
    public void onResume(){
        nativeOnResume(mObjPtr);
    }
    public void onPause(){
        nativeOnPause(mObjPtr);
        stopSaveFilter();
    }
    public void registerOnProgressCallBack(Activity activity){
        this.nativeSetOnProgressCallback(mObjPtr, activity);
    }
    public void loadVideo(Surface surface, AssetManager assetManager, String filePath, int width, int height){
        videoWidth = width;
        videoHeight = height;
        nativeLoadVideo(mObjPtr, surface, assetManager, filePath, width, height);
    }
    public void doUpdateRenderMode(EnumElement.RENDER_MODE renderMode){
        nativeUpdateRenderMode(mObjPtr, renderMode.ordinal());
    }
    public void doUpdateViewMode(EnumElement.VIEW_MODE viewMode){
        nativeUpdateViewMode(mObjPtr, viewMode.ordinal());
    }
    public void doUpdateOptionMode(EnumElement.OPTION_MODE optionMode){
        nativeUpdateOptionMode(mObjPtr, optionMode.ordinal());
    }
    public void doSetPlaying(final boolean isPlaying){
        nativeSetPlaying(mObjPtr, isPlaying);
        if(!isPlaying) {
            stopSaveFilter();
        }
    }
    public void doUpdateProgress(final float progress){
        nativeUpdateProgress(mObjPtr, progress);
    }
    public void doRestart(){
        nativeRestart(mObjPtr);
    }
    public void doUpdateLutFilter(final AssetManager assetManager, final String filePath, final EnumElement.FILTER_TYPE filterType){
        nativeUpdateLutFilter(mObjPtr, assetManager, filePath, filterType.ordinal());
    }
    public void doLoadLogoImage(final AssetManager assetManager, final String logoFilePath){
        if(logoFilePath.lastIndexOf("/") == -1){
            nativeLoadLogoImage(mObjPtr, assetManager, logoFilePath);
        }else {
            nativeLoadLogoImage(mObjPtr, null, logoFilePath);
        }
    }
    public void doScreenShot(){
        nativeScreenShot(mObjPtr, PathConfig.getScreenShotDir());
    }
    public void doUpdateLogoAngle(final float angle){
        nativeUpdateLogoAngle(mObjPtr, angle);
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
    public void doUpdateSensorRotate(float[] sRotateMat){
        nativeUpdateSensorRotate(mObjPtr, sRotateMat);
    }

    VideoMediaEncoder videoMediaEncoder;
    AudioMediaEncoder audioMediaEncoder;
    String outFilePath;
    public void startSaveFilter(){
        try {
            if (mediaMuxerWrapper == null) {
                outFilePath = PathConfig.getMediaFolder() + File.separator + new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss").format(new Date()) + ".mp4";
                new File(outFilePath);
                mediaMuxerWrapper = new MediaMuxerWrapper(outFilePath, mediaMuxerWrapperListener);
                videoMediaEncoder = new VideoMediaEncoder(mediaMuxerWrapper, mediaEncoderListener, videoWidth, videoHeight, BIT_RATE);
                audioMediaEncoder = new AudioMediaEncoder(mediaMuxerWrapper, mediaEncoderListener, false);
                mediaMuxerWrapper.addEncoder(videoMediaEncoder, audioMediaEncoder);
                mediaMuxerWrapper.prepare();
                mediaMuxerWrapper.startRecording();
                nativeSetSaveFilter(mObjPtr, 1);
            }
        }catch (Exception e){

        }
    }

    public void stopSaveFilter(){
        if (mediaMuxerWrapper != null && mediaMuxerWrapper.isStarted()) {
            isVideoRecording =   false;
            mediaMuxerWrapper.stopRecording();
        }
        nativeSetSaveFilter(mObjPtr, 0);
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
                videoMediaEncoder.setEglContext(EGL14.eglGetCurrentContext(), PanoramaCameraRender.this);
            }
        }

        @Override
        public void onStopped(MediaEncoder encoder) {
            if (encoder instanceof VideoMediaEncoder) {
                isVideoRecording = false;
            }
        }
    };

//    @Override
//    synchronized public void onFrameAvailable(SurfaceTexture surface) {
//        if(bDestroy || mObjPtr == 0L){
//            return;
//        }
//        updateSurface = true;
//    }
    public SurfaceTexture getSurfaceTexture1() {
        return mSurfaceTexture1;
    }
    public SurfaceTexture getSurfaceTexture2() {
        return mSurfaceTexture2;
    }
    private static native int  nativeGetTextureId1(long objPtr);
    private static native int  nativeGetTextureId2(long objPtr);
    private static native void nativeInitGL(long objPtr, float widgetWidth, float widgetHeight);
    private static native void nativeOnSurfaceChanged(long objPtr, int pNewSurfaceWidth, int pNewSurfaceHeight);
    private static native void nativeDrawFrame(long objPtr);
    private static native void nativeDrawFilterFrame(long objPtr);
    private static native void nativeOnResume(long objPtr);
    private static native void nativeOnPause(long objPtr);
    private static native long nativeOnCreate(Context context, String mediaDir, PanoramaCameraRender panoramaVideoRender);
    private static native void nativeLoadVideo(long objPtr, Surface surface, AssetManager assetMgr, String filename, int width, int height);
    private static native void nativeLoadLogoImage(long objPtr, AssetManager assetManager, String filePath);
    private static native void nativeSetPlaying(long objPtr, boolean isPlaying);
    private static native void nativeSetOnProgressCallback(long objPtr, final Activity callback);
    private static native void nativeOnDestroy(long objPtr);
    private static native void nativeScreenShot(long objPtr, String screenDir);
    private static native void nativeUpdateProgress(long objPtr, float progress);
    private static native void nativeRestart(long objPtr);
    private static native void nativeUpdateScale(long objPtr, float scale);
    private static native void nativeUpdateRotate(long objPtr, float xMove, float yMove);
    private static native void nativeUpdateRotateFling(long objPtr, float velocityX, float velocityY);
    private static native void nativeUpdateSensorRotate(long objPtr, float[] sRotateMat);
    private static native void nativeUpdateOptionMode(long objPtr, int optionMode);
    private static native void nativeUpdateViewMode(long objPtr, int viewMode);
    private static native void nativeUpdateRenderMode(long objPtr, int vrMode);
    private static native void nativeUpdateLutFilter(long objPtr, AssetManager am, String filePath, int filterType);
    private static native void nativeUpdateLogoAngle(long objPtr, float angle);
    private static native void nativeSetSaveFilter(long objPtr, int saveFilter);//1:save,0:not

    @Override
    public void render() {
        nativeDrawFilterFrame(mObjPtr);
    }

    private Context mContext;
    public void startCamera(Context context){
        mContext = context;
        initCamera1();
        initCamera2();
    }
    private void initCamera1(){
        HandlerThread handlerThread = new HandlerThread("Camera2");
        handlerThread.start();
        childHandler1 = new Handler(handlerThread.getLooper());
        mainHandler1 = new Handler(mContext.getMainLooper());
        cameraID1 = "" + CameraCharacteristics.LENS_FACING_FRONT;//后摄像头
        imageReader1 = ImageReader.newInstance(1080, 1920, ImageFormat.JPEG,1);
        imageReader1.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() { //可以在这里处理拍照得到的临时照片 例如，写入本地
            @Override
            public void onImageAvailable(ImageReader reader) {
                cameraDevice1.close();
                // 拿到拍照照片数据
                Image image = reader.acquireNextImage();
                ByteBuffer buffer = image.getPlanes()[0].getBuffer();
                byte[] bytes = new byte[buffer.remaining()];
                buffer.get(bytes);//由缓冲区存入字节数组
                final Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
                if (bitmap != null) {
                    // iv_show.setImageBitmap(bitmap);
                }
            }
        }, mainHandler1);
        //获取摄像头管理
        mCameraManager = (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
        //打开摄像头
        try {
            mCameraManager.openCamera(cameraID1, stateCallback1, mainHandler1);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /**
     * 摄像头创建监听
     */
    private CameraDevice.StateCallback stateCallback1 = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {//打开摄像头
            cameraDevice1 = camera;
            //开启预览
            takePreview1();
        }

        @Override
        public void onDisconnected(CameraDevice camera) {//关闭摄像头
            if (null != cameraDevice1) {
                cameraDevice1.close();
                cameraDevice1 = null;
            }
        }

        @Override
        public void onError(CameraDevice camera, int error) {//发生错误
            // Toast.makeText(Camera2Activity.this, "摄像头开启失败", Toast.LENGTH_SHORT).show();
        }
    };

    /**
     * 开始预览
     */
    private void takePreview1() {
        try {
            // 创建预览需要的CaptureRequest.Builder
            final CaptureRequest.Builder previewRequestBuilder = cameraDevice1.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            // 将SurfaceView的surface作为CaptureRequest.Builder的目标
            previewRequestBuilder.addTarget(mSurface1);
            // 创建CameraCaptureSession，该对象负责管理处理预览请求和拍照请求
            cameraDevice1.createCaptureSession(Arrays.asList(mSurface1,  imageReader1.getSurface()), new CameraCaptureSession.StateCallback() // ③
            {
                @Override
                public void onConfigured(CameraCaptureSession cameraCaptureSession) {
                    if (null == cameraDevice1) return;
                    // 当摄像头已经准备好时，开始显示预览
                    cameraCaptureSession1 = cameraCaptureSession;
                    try {
                        // 自动对焦
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                        // 打开闪光灯
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);
                        // 显示预览
                        CaptureRequest previewRequest = previewRequestBuilder.build();
                        cameraCaptureSession1.setRepeatingRequest(previewRequest, null, childHandler1);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }

                @Override
                public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
                    // Toast.makeText(Camera2Activity.this, "配置失败", Toast.LENGTH_SHORT).show();
                }
            }, childHandler1);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }
    private CameraManager mCameraManager;//摄像头管理器
    private Handler childHandler1, mainHandler1;
    private String cameraID1;//摄像头Id 0 为后  1 为前
    private ImageReader imageReader1;
    private CameraCaptureSession cameraCaptureSession1;
    private CameraDevice cameraDevice1;
    private Handler childHandler2, mainHandler2;
    private String cameraID2;//摄像头Id 0 为后  1 为前
    private ImageReader imageReader2;
    private CameraCaptureSession cameraCaptureSession2;
    private CameraDevice cameraDevice2;

    class surfaceholderCallbackFont implements SurfaceHolder.Callback {
        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            initCamera2();
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            if (null != cameraDevice2) {
                cameraDevice2.close();
                cameraDevice2 = null;
            }

        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {

        }
    }
    private void initCamera2(){
        HandlerThread handlerThread = new HandlerThread("Camera2");
        handlerThread.start();
        childHandler2 = new Handler(handlerThread.getLooper());
        mainHandler2 = new Handler(mContext.getMainLooper());
        cameraID2 = "" + CameraCharacteristics.LENS_FACING_BACK;//后摄像头
        imageReader2 = ImageReader.newInstance(1080, 1920, ImageFormat.JPEG,1);
        imageReader2.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() { //可以在这里处理拍照得到的临时照片 例如，写入本地
            @Override
            public void onImageAvailable(ImageReader reader) {
                cameraDevice2.close();
                Image image = reader.acquireNextImage();
                ByteBuffer buffer = image.getPlanes()[0].getBuffer();
                byte[] bytes = new byte[buffer.remaining()];
                buffer.get(bytes);//由缓冲区存入字节数组
                final Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
                if (bitmap != null) {
                    // iv_show.setImageBitmap(bitmap);
                }
            }
        }, mainHandler2);
        //获取摄像头管理
        mCameraManager = (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
        //打开摄像头
        try {
            mCameraManager.openCamera(cameraID2, stateCallback2, mainHandler2);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }
    /**
     * 摄像头创建监听
     */
    private CameraDevice.StateCallback stateCallback2 = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {//打开摄像头
            cameraDevice2 = camera;
            //开启预览
            takePreview2();
        }

        @Override
        public void onDisconnected(CameraDevice camera) {//关闭摄像头
            if (null != cameraDevice2) {
                cameraDevice2.close();
                cameraDevice2 = null;
            }
        }

        @Override
        public void onError(CameraDevice camera, int error) {//发生错误
            // Toast.makeText(Camera2Activity.this, "摄像头开启失败", Toast.LENGTH_SHORT).show();
        }
    };

    /**
     * 开始预览
     */
    private void takePreview2() {
        try {
            // 创建预览需要的CaptureRequest.Builder
            final CaptureRequest.Builder previewRequestBuilder = cameraDevice2.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            // 将SurfaceView的surface作为CaptureRequest.Builder的目标
            previewRequestBuilder.addTarget(mSurface2);
            // 创建CameraCaptureSession，该对象负责管理处理预览请求和拍照请求
            cameraDevice2.createCaptureSession(Arrays.asList(mSurface2, imageReader2.getSurface()), new CameraCaptureSession.StateCallback() // ③
            {
                @Override
                public void onConfigured(CameraCaptureSession cameraCaptureSession) {
                    if (null == cameraDevice2) return;
                    // 当摄像头已经准备好时，开始显示预览
                    cameraCaptureSession2 = cameraCaptureSession;
                    try {
                        // 自动对焦
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                        // 打开闪光灯
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);
                        // 显示预览
                        CaptureRequest previewRequest = previewRequestBuilder.build();
                        cameraCaptureSession2.setRepeatingRequest(previewRequest, null, childHandler2);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }

                @Override
                public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
                    // Toast.makeText(Camera2Activity.this, "配置失败", Toast.LENGTH_SHORT).show();
                }
            }, childHandler2);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }
}
