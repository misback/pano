package com.uni.vr.encoder;

import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.Matrix;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.uni.vr.CameraVideoRender;

/**
 * Created by DELL on 2017/6/6.
 */

public class RenderHandler implements Runnable{
    private static final String TAG = RenderHandler.class.getSimpleName();
    private static final int EGL_RECORDABLE_ANDROID = 0x3142;
    private final Object mSync = new Object();
    private boolean mRequestSetEglContext;
    private boolean mRequestRelease;
    private EGLDisplay mEncoderEglDisplay;
    private EGLContext mEncoderEglContext;
    private EGLSurface mEncoderSurface;
    private int mRequestDraw = 0;
    public interface DrawInterface {
        void render();
    }
    private DrawInterface drawInterface;
    EGLContext eglContext;
    Object mSurface;
    public void setEglContext(EGLContext eglContext, final Object surface, DrawInterface drawInterface){
        synchronized (mSync) {
            if (mRequestRelease) return;
            this.eglContext = eglContext;
            mRequestSetEglContext = true;
            mSurface = surface;
            this.drawInterface = drawInterface;
            mSync.notifyAll();
            try {
                mSync.wait();
            } catch (final InterruptedException e) {
            }
        }
    }
    public final void release() {
        synchronized (mSync) {
            if (mRequestRelease) return;
            mRequestRelease = true;
            mSync.notifyAll();
            try {
                mSync.wait();
            } catch (final InterruptedException e) {
            }
        }
    }
    public static final RenderHandler createHandler(final String name) {
        final RenderHandler handler = new RenderHandler();
        synchronized (handler.mSync) {
            new Thread(handler, !TextUtils.isEmpty(name) ? name : TAG).start();
            try {
                handler.mSync.wait();
            } catch (final InterruptedException e) {
            }
        }
        return handler;
    }
    public void draw(){
        synchronized (mSync) {
            mRequestDraw ++;
            mSync.notifyAll();
        }
    }
    public EGLSurface createWindowSurface(Object surface) {
        mEncoderEglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (mEncoderEglDisplay == EGL14.EGL_NO_DISPLAY) {
            throw new RuntimeException("unable to get EGL14 display");
        }
        int[] version = new int[2];
        if (!EGL14.eglInitialize(mEncoderEglDisplay, version, 0, version, 1)) {
            mEncoderEglDisplay = null;
            throw new RuntimeException("unable to initialize EGL14");
        }

        int[] attribList = {
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                EGL_RECORDABLE_ANDROID, 1,
                EGL14.EGL_NONE
        };
        android.opengl.EGLConfig[] configs = new android.opengl.EGLConfig[1];
        int[] numConfigs = new int[1];
        if (!EGL14.eglChooseConfig(mEncoderEglDisplay, attribList, 0, configs, 0, configs.length,
                numConfigs, 0)) {
            throw new RuntimeException("unable to find RGB888+recordable ES2 EGL config");
        }

        int[] attrib_list = {
                EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL14.EGL_NONE
        };
        mEncoderEglContext = EGL14.eglCreateContext(mEncoderEglDisplay, configs[0], eglContext, attrib_list, 0);

        if (!EGL14.eglMakeCurrent(mEncoderEglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT)) {
            Log.w("TAG", "makeDefault" + EGL14.eglGetError());
        }
        int[] surfaceAttribs = {
                EGL14.EGL_NONE
        };
        return EGL14.eglCreateWindowSurface(mEncoderEglDisplay, configs[0], surface, surfaceAttribs, 0);
    }

    public  void makeEncoderSurfaceCurrent() {
        EGL14.eglMakeCurrent(mEncoderEglDisplay,mEncoderSurface, mEncoderSurface, mEncoderEglContext);
    }

    public void swapEncoderSurfaceBuffer() {
        if (EGL14.eglSwapBuffers(mEncoderEglDisplay, mEncoderSurface)) {
        }
    }
    private void makeDefault() {
        if (!EGL14.eglMakeCurrent(mEncoderEglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT)) {
            Log.w("TAG", "makeDefault" + EGL14.eglGetError());
        }
    }
    @Override
    public void run() {
        synchronized (mSync) {
            mRequestSetEglContext = mRequestRelease = false;
            mRequestDraw = 0;
            mSync.notifyAll();
        }
        boolean localRequestDraw;
        for(;;){
            synchronized (mSync) {
                if (mRequestRelease) break;
                if (mRequestSetEglContext) {
                    mRequestSetEglContext = false;
                    mEncoderSurface = createWindowSurface(mSurface);
                    mSync.notifyAll();
                }
            }
            localRequestDraw = mRequestDraw > 0;
            if (localRequestDraw) {
                mRequestDraw--;
            }
            if (localRequestDraw) {
                makeEncoderSurfaceCurrent();
                drawInterface.render();
                swapEncoderSurfaceBuffer();
            }else {
                synchronized (mSync) {
                    try {
                        mSync.wait();
                    } catch (final InterruptedException e) {
                        break;
                    }
                }
            }
        }
        synchronized (mSync) {
            mRequestRelease = true;
            makeDefault();
            if(mEncoderSurface != null) {
                EGL14.eglDestroySurface(mEncoderEglDisplay, mEncoderSurface);
                mEncoderSurface =   EGL14.EGL_NO_SURFACE;
            }
            if (mEncoderEglDisplay != EGL14.EGL_NO_DISPLAY) {
                EGL14.eglDestroyContext(mEncoderEglDisplay, mEncoderEglContext);
                EGL14.eglTerminate(mEncoderEglDisplay);
                EGL14.eglReleaseThread();
            }
            mEncoderEglDisplay = EGL14.EGL_NO_DISPLAY;
            mEncoderEglContext = EGL14.EGL_NO_CONTEXT;
            mSync.notifyAll();
        }
    }
}
