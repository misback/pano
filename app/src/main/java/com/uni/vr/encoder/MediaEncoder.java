package com.uni.vr.encoder;

import android.media.MediaCodec;
import android.media.MediaFormat;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Created by DELL on 2017/4/14.
 */

public abstract class MediaEncoder implements Runnable {
    private static final String TAG = MediaEncoder.class.getSimpleName();
    private static final int TIMEOUT_USEC = 10000;
    protected MediaCodec mediaCodec;
    protected MediaCodec.BufferInfo bufferInfo;
    protected final WeakReference<MediaMuxerWrapper> mediaMuxerWrapperWeakReference;
    protected int mTrackIndex;
    /**
     * Flag that indicate this encoder is capturing now.
     */
    protected volatile boolean mIsCapturing;
    /**
     * Flag that indicate the frame data will be available soon.
     */
    private int mRequestDrain;
    /**
     * Flag to request stop capturing
     */
    protected volatile boolean mRequestStop;
    protected boolean mIsEOS;
    protected boolean mMuxerStarted;
    protected final Object mSync = new Object();

    ExecutorService singleThreadExecutor= Executors.newSingleThreadExecutor();

    public interface MediaEncoderListener {
        public void onPrepared(MediaEncoder encoder);
        public void onStopped(MediaEncoder encoder);
    }
    MediaEncoderListener mediaEncoderListener;
    abstract void prepare() throws IOException;
    @Override
    public void run() {
        synchronized (mSync) {
            mRequestStop = false;
            mRequestDrain = 0;
            mSync.notify();
        }
        final boolean isRunning = true;
        boolean localRequestStop;
        boolean localRequestDrain;
        while (isRunning) {
            synchronized (mSync) {
                localRequestStop = mRequestStop;
                localRequestDrain = (mRequestDrain > 0);
                if (localRequestDrain)
                    mRequestDrain--;
            }
            if (localRequestStop) {
                drain();
                // request stop recording
                signalEndOfInputStream();
                // process output data again for EOS signale
                drain();
                // release all related objects
                release();
                break;
            }
            if (localRequestDrain) {
                drain();
            } else {
                synchronized (mSync) {
                    try {
                        mSync.wait();
                    } catch (final InterruptedException e) {
                        break;
                    }
                }
            }
        } // end of while
        synchronized (mSync) {
            mRequestStop = true;
            mIsCapturing = false;
        }
    }


    public void startRecording(){
        synchronized (mSync) {
            mIsCapturing = true;
            mRequestStop = false;
            mSync.notifyAll();
        }
    }
    public void stopRecording(){
        synchronized (mSync) {
            if (!mIsCapturing || mRequestStop) {
                return;
            }
            mRequestStop = true;	// for rejecting newer frame
            mSync.notifyAll();
        }
    }

    protected void release() {
        if (mediaEncoderListener!=null){
            mediaEncoderListener.onStopped(this);
            mediaEncoderListener = null;
        }
        singleThreadExecutor.shutdown();
        mIsCapturing = false;
        if (mediaCodec != null) {
            mediaCodec.stop();
            mediaCodec.release();
            mediaCodec = null;
        }
        if (mMuxerStarted) {
            MediaMuxerWrapper mediaMuxerWrapper = mediaMuxerWrapperWeakReference != null ? mediaMuxerWrapperWeakReference.get() : null;
            if (mediaMuxerWrapper != null) {
                mediaMuxerWrapper.stop();
            }
        }
        bufferInfo = null;

    }

    public MediaEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener mediaEncoderListener) {
        mediaMuxerWrapperWeakReference = new WeakReference<MediaMuxerWrapper>(mediaMuxerWrapper);
        singleThreadExecutor.execute(this);
        this.mediaEncoderListener = mediaEncoderListener;
    }
    public boolean frameAvailableSoon(){
        synchronized (mSync) {
            if (!mIsCapturing || mRequestStop) {
                return false;
            }
            mRequestDrain++;
            mSync.notifyAll();
        }
        return true;
    }
    protected void signalEndOfInputStream() {
        encode(null, 0, getPTSUs());
    }
    protected void encode(final ByteBuffer buffer, final int length, final long presentationTimeUs) {
        if (!mIsCapturing) return;
        if (length <= 0) return;
        int index = 0;
        int size = 0;
        while (index < length) {
            int inputBufferIndex = mediaCodec.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                final ByteBuffer inputBuffer = mediaCodec.getInputBuffer(inputBufferIndex);
                inputBuffer.clear();
                size = inputBuffer.remaining();
                size = ((index + size) < length) ? size : (length - index);
                if ((size > 0) && (buffer != null)) {
                    inputBuffer.put(buffer);
                }
                index += size;
                if (length <= 0) {
                    mediaCodec.queueInputBuffer(inputBufferIndex, 0, 0, presentationTimeUs, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                    index = length + 1;
                } else {
                    mediaCodec.queueInputBuffer(inputBufferIndex, 0, size, presentationTimeUs, 0);
                }
            }
        }
    }

    protected void drain() {
        if (mediaCodec == null || mediaMuxerWrapperWeakReference == null ) {
            return;
        }
        final MediaMuxerWrapper mediaMuxerWrapper = mediaMuxerWrapperWeakReference.get();
        if (mediaMuxerWrapper == null) {
            return;
        }
        int count = 0;
LOOP:   while (mIsCapturing) {
            int encoderStatus = mediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_USEC);
            if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                if (!mIsEOS) {
                    if (++count > 5)
                        break LOOP;		// out of while
                }
            } else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat newFormat = mediaCodec.getOutputFormat();
                mTrackIndex = mediaMuxerWrapper.addTrack(newFormat);
                mMuxerStarted = true;
                if (!mediaMuxerWrapper.start()) {
                    // we should wait until muxer is ready
                    synchronized (mediaMuxerWrapper) {
                        while (!mediaMuxerWrapper.isStarted())
                            try {
                                mediaMuxerWrapper.wait(100);
                            } catch (final InterruptedException e) {
                                break LOOP;
                            }
                    }
                }
            } else if (encoderStatus < 0) {
                break;
            } else {
                ByteBuffer encoderOutputBuffer = mediaCodec.getOutputBuffer(encoderStatus);
                if(encoderOutputBuffer == null){
                    break;
                }
                if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                    bufferInfo.size = 0;
                }
                if (bufferInfo.size != 0 && mMuxerStarted) {
                    count = 0;
                    encoderOutputBuffer.position(bufferInfo.offset);
                    encoderOutputBuffer.limit(bufferInfo.offset + bufferInfo.size);
                    mediaMuxerWrapper.writeSampleData(mTrackIndex, encoderOutputBuffer, bufferInfo);
                    prevOutputPTSUs = bufferInfo.presentationTimeUs;
                }
                mediaCodec.releaseOutputBuffer(encoderStatus, false);
                if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    break;
                }
            }
        }
    }

    protected long prevOutputPTSUs = 0;
    protected long getPTSUs() {
        long result = System.nanoTime() / 1000L;
        if (result < prevOutputPTSUs)
            result = (prevOutputPTSUs - result) + result;
        return result;
    }
}
