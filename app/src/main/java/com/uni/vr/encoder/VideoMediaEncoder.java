package com.uni.vr.encoder;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.uni.pano.config.EnumElement;
import com.uni.vr.CameraVideoRender;

import java.io.IOException;
import java.nio.ByteBuffer;


/**
 * Created by DELL on 2017/4/14.
 */

public class VideoMediaEncoder extends MediaEncoder {
    public static final String TAG = VideoMediaEncoder.class.getSimpleName();
    private static final int FRAME_RATE = 30;               // 30fps
    public static final int IFRAME_INTERVAL = 10;           // 5 seconds between I-frames
    private float BPP = 0.25f;
    private int videoWidth;
    private int videoHeight;
    private static final String MIME_TYPE = "video/avc";
    RenderHandler mRenderHandler;
    public VideoMediaEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener mediaEncoderListener, int videoWidth, int videoHeight, int BIT_RATE){
        super(mediaMuxerWrapper, mediaEncoderListener);
        this.videoWidth = videoWidth;
        this.videoHeight = videoHeight;
        mRenderHandler = RenderHandler.createHandler(TAG);
    }
    Surface mSurface;

    @Override
    void prepare() throws IOException {
        mTrackIndex = -1;
        mMuxerStarted = mIsEOS = false;
        try {
            final MediaCodecInfo videoCodecInfo = selectVideoCodec(MIME_TYPE);
            if (videoCodecInfo == null) {
                Log.e(TAG, "Unable to find an appropriate codec for " + MIME_TYPE);
                return;
            }
            MediaFormat format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, videoWidth, videoHeight);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            format.setInteger(MediaFormat.KEY_BIT_RATE, 4000000);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
            bufferInfo = new MediaCodec.BufferInfo();
            mediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
            mediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mSurface = mediaCodec.createInputSurface();
            mediaCodec.start();
            if (mediaEncoderListener != null) {
                try {
                    mediaEncoderListener.onPrepared(this);
                } catch (final Exception e) {
                    Log.e(TAG, "prepare:", e);
                }
            }
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    public void setEglContext(final EGLContext shared_context, RenderHandler.DrawInterface drawInterface) {
        mRenderHandler.setEglContext(shared_context, mSurface, drawInterface);
    }
    @Override
    public void release() {
        if (mSurface != null) {
            mSurface.release();
            mSurface = null;
        }
        if (mRenderHandler != null) {
            mRenderHandler.release();
            mRenderHandler = null;
        }
        super.release();
    }
    @Override
    public boolean frameAvailableSoon(){
        boolean result;
        if (result = super.frameAvailableSoon())
            mRenderHandler.draw();
        return result;
    }
    /**
     * select the first codec that match a specific MIME type
     * @param mimeType
     * @return null if no codec matched
     */
    protected static final MediaCodecInfo selectVideoCodec(final String mimeType) {
        // get the list of available codecs
        final int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            final MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);

            if (!codecInfo.isEncoder()) {	// skipp decoder
                continue;
            }
            // select first codec that match a specific MIME type and color format
            final String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                    final int format = selectColorFormat(codecInfo, mimeType);
                    if (format > 0) {
                        return codecInfo;
                    }
                }
            }
        }
        return null;
    }
    /**
     * select color format available on specific codec and we can use.
     * @return 0 if no colorFormat is matched
     */
    protected static final int selectColorFormat(final MediaCodecInfo codecInfo, final String mimeType) {
        int result = 0;
        final MediaCodecInfo.CodecCapabilities caps;
        try {
            Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
            caps = codecInfo.getCapabilitiesForType(mimeType);
        } finally {
            Thread.currentThread().setPriority(Thread.NORM_PRIORITY);
        }
        int colorFormat;
        for (int i = 0; i < caps.colorFormats.length; i++) {
            colorFormat = caps.colorFormats[i];
            if (isRecognizedViewoFormat(colorFormat)) {
                if (result == 0)
                    result = colorFormat;
                break;
            }
        }
        if (result == 0)
            Log.e(TAG, "couldn't find a good color format for " + codecInfo.getName() + " / " + mimeType);
        return result;
    }
    /**
     * color formats that we can use in this class
     */
    protected static int[] recognizedFormats;
    static {
        recognizedFormats = new int[] {
//        	MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar,
//        	MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar,
//        	MediaCodecInfo.CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar,
                MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface,
        };
    }
    private static final boolean isRecognizedViewoFormat(final int colorFormat) {
        final int n = recognizedFormats != null ? recognizedFormats.length : 0;
        for (int i = 0; i < n; i++) {
            if (recognizedFormats[i] == colorFormat) {
                return true;
            }
        }
        return false;
    }
    @Override
    protected void signalEndOfInputStream() {
        mediaCodec.signalEndOfInputStream();	// API >= 18
        mIsEOS = true;
    }
    private int calcBitRate() {
        final int bitrate = (int)(BPP * FRAME_RATE * videoWidth * videoHeight);
        return bitrate;
    }
}
