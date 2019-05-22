package com.uni.vr;
import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.EGL14;
import android.opengl.EGLDisplay;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;

import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.MainApplication;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.CreateFileEvent;
import com.uni.pano.event.ScreenShotEvent;

import org.greenrobot.eventbus.EventBus;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
/**
 * Created by DELL on 2017/3/8.
 */

public class PanoramaImageRender implements GLSurfaceView.Renderer {
    private final static String TAG 				= 	PanoramaImageRender.class.getSimpleName();
    private float width;
    private float height;
    private boolean bDestroy    = false;
    private long mObjPtr = 0L;
    public PanoramaImageRender(AssetManager assetManager) {
        mObjPtr = nativeOnCreate(MainApplication.getInstance(), PathConfig.getMediaFileSaveDir(), this);
        String logoFilePath = PreferenceModel.getString("logoFileName", "logo_none.png");
        int zoom = PreferenceModel.getInt("ZOOM", 15);
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
        nativeInitGL(mObjPtr);
    }
    @Override
    public void onDrawFrame(GL10 glUnused) {
        if(bDestroy || mObjPtr == 0L){
            return;
        }
        nativeDrawFrame(mObjPtr);
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
    //1:YES 0:NO
    public void callback(final String fileName, int isScreen){
        if(isScreen == 1) {
            EventBus.getDefault().post(new ScreenShotEvent(fileName));
        }else{
            EventBus.getDefault().post(new CreateFileEvent(fileName));
        }
    }

    public void onResume(){
        nativeOnResume(mObjPtr);
    }
    public void onPause(){
        nativeOnPause(mObjPtr);
    }
    public void onDestroy(){
        bDestroy = true;
        if(mObjPtr != 0L) {
            nativeOnDestroy(mObjPtr);
            mObjPtr = 0L;
        }
    }

    public void loadImage(String filePath, AssetManager assetManager){
        nativeLoadImage(mObjPtr, assetManager, filePath);
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
    public void doSaveFilterImage(){
        nativeSaveFilterImage(mObjPtr, PathConfig.getMediaFileSaveDir());
    }

    public void doUpdateLogoAngle(final float angle){
        nativeUpdateLogoAngle(mObjPtr, angle);
    }

    public void doUpdateRotate(final float rotateX, final float rotateY, final boolean bNotAnimation){
        nativeUpdateRotate(mObjPtr, rotateX, rotateY, bNotAnimation?1:0);
    }
    public void doUpdateScale(float angle){
        nativeUpdateScale(mObjPtr, angle);
    }
    public void doUpdateRotate(float yaw, float pitch, int notAnimation){
        nativeUpdateRotate(mObjPtr, yaw, pitch, notAnimation);
    }
    public void doUpdateRotateFling(float velocityX, float velocityY){
        nativeUpdateRotateFling(mObjPtr, velocityX, velocityY);
    }
    public void doUpdateSensorRotate(float[] sRotateMat){
        nativeUpdateSensorRotate(mObjPtr, sRotateMat);
    }

    private static native void nativeInitGL(long objPtr);
    private static native void nativeOnSurfaceChanged(long objPtr, int pNewSurfaceWidth, int pNewSurfaceHeight);
    private static native void nativeDrawFrame(long objPtr);
    private static native void nativeOnResume(long objPtr);
    private static native void nativeOnPause(long objPtr);
    private static native long nativeOnCreate(Context context, String mediaDir, PanoramaImageRender panoramaImageRender);
    private static native void nativeLoadImage(long objPtr, AssetManager assetManager, String filePath);
    private static native void nativeLoadLogoImage(long objPtr, AssetManager assetManager, String filePath);
    private static native void nativeOnDestroy(long objPtr);
    private static native void nativeSaveFilterImage(long objPtr, String fileSaveDir);
    private static native void nativeScreenShot(long objPtr, String screenDir);
    private static native void nativeUpdateScale(long objPtr, float scale);
    private static native void nativeUpdateRotate(long objPtr, float xMove, float yMove, int notAnimation);
    private static native void nativeUpdateRotateFling(long objPtr, float velocityX, float velocityY);
    private static native void nativeUpdateScaleFling(long objPtr, float velocityScale);
    private static native void nativeUpdateSensorRotate(long objPtr, float[] sRotateMat);
    private static native void nativeUpdateOptionMode(long objPtr, int optionMode);
    private static native void nativeUpdateViewMode(long objPtr, int viewMode);
    private static native void nativeUpdateRenderMode(long objPtr, int renderMode);
    private static native void nativeUpdateLutFilter(long objPtr, AssetManager am, String filePath, int filterType);
    private static native void nativeUpdateLogoAngle(long objPtr, float angle);

}