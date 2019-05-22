package com.uni.vr.encoder;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.util.Log;

import com.uni.pano.config.EnumElement;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by DELL on 2017/4/15.
 */

public class AudioMediaEncoder extends MediaEncoder {
    public static final String TAG = AudioMediaEncoder.class.getSimpleName();
    public static final int SAMPLE_RATE = 44100;	// 44.1[KHz] is only setting guaranteed to be available on all devices.
    public static final int BIT_RATE = 64000;
    public static final int SAMPLES_PER_FRAME = 1024;	// AAC, bytes/frame/channel
    public static final int FRAMES_PER_BUFFER = 30; 	// AAC, frame/buffer/sec
    private AudioThread mAudioThread = null;
    private boolean mUsbAudioRecord = true;
    public AudioMediaEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener mediaEncoderListener, boolean useAudioRecord){
        super(mediaMuxerWrapper, mediaEncoderListener);
        mUsbAudioRecord = useAudioRecord;
    }

    @Override
    void prepare() throws IOException {
        mTrackIndex = -1;
        mMuxerStarted = mIsEOS = false;
        bufferInfo = new MediaCodec.BufferInfo();
        final MediaFormat audioFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, SAMPLE_RATE, 1);
        audioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
        audioFormat.setInteger(MediaFormat.KEY_CHANNEL_MASK, AudioFormat.CHANNEL_IN_MONO);
        audioFormat.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);
        audioFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, 1);
        try {
            mediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            mediaCodec.configure(audioFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mediaCodec.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
        if (mediaEncoderListener != null) {
            try {
                mediaEncoderListener.onPrepared(this);
            } catch (final Exception e) {
                Log.e(TAG, "prepare:", e);
            }
        }
    }

    @Override
    public void startRecording(){
        super.startRecording();
        if (mAudioThread == null && mUsbAudioRecord) {
            mAudioThread = new AudioThread();
            mAudioThread.start();
        }
    }
    public void stopRecording(){
        super.stopRecording();
    }
    @Override
    protected void release() {
        mAudioThread = null;
        super.release();
    }
    private final int[] AUDIO_SOURCES = new int[] {
        MediaRecorder.AudioSource.MIC,
        MediaRecorder.AudioSource.DEFAULT,
        MediaRecorder.AudioSource.CAMCORDER,
        MediaRecorder.AudioSource.VOICE_COMMUNICATION,
        MediaRecorder.AudioSource.VOICE_RECOGNITION,
    };
    private class AudioThread extends Thread {
        @Override
        public void run() {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
            final int min_buffer_size = AudioRecord.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
            int buffer_size = SAMPLES_PER_FRAME * FRAMES_PER_BUFFER;
            if (buffer_size < min_buffer_size) {
                buffer_size = ((min_buffer_size / SAMPLES_PER_FRAME) + 1) * SAMPLES_PER_FRAME * 2;
            }
            AudioRecord audioRecord = null;
            for (final int source : AUDIO_SOURCES) {
                try {
                    audioRecord = new AudioRecord(source, SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, buffer_size);
                    if (audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                        audioRecord = null;
                    }
                } catch (final Exception e) {
                    audioRecord = null;
                }
                if (audioRecord != null) {
                    break;
                }
            }
            if (audioRecord != null) {
                try {
                    if (mIsCapturing) {
                        final ByteBuffer buf = ByteBuffer.allocateDirect(SAMPLES_PER_FRAME);
                        int readBytes;
                        audioRecord.startRecording();
                        try {
                            for (; mIsCapturing&&!mRequestStop && !mIsEOS ;) {
                                buf.clear();
                                readBytes = audioRecord.read(buf, SAMPLES_PER_FRAME);
                                if (readBytes > 0) {
                                    buf.position(readBytes);
                                    buf.flip();
                                    encode(buf, readBytes, getPTSUs());
                                    frameAvailableSoon();
                                }
                            }
                            frameAvailableSoon();
                        } finally {
                            audioRecord.stop();
                        }
                    }
                } finally {
                    audioRecord.release();
                }
            }
        }
    }
    //for filter
    public void filterEncode(ByteBuffer buf, int readBytes){
        encode(buf, readBytes, getPTSUs());
        frameAvailableSoon();
    }
    /**
     * select the first codec that match a specific MIME type
     * @param mimeType
     * @return
     */
    private static final MediaCodecInfo selectAudioCodec(final String mimeType) {

        MediaCodecInfo result = null;
        // get the list of available codecs
        final int numCodecs = MediaCodecList.getCodecCount();
 LOOP:	for (int i = 0; i < numCodecs; i++) {
            final MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            if (!codecInfo.isEncoder()) {	// skipp decoder
                continue;
            }
            final String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                    if (result == null) {
                        result = codecInfo;
                        break LOOP;
                    }
                }
            }
        }
        return result;
    }
}
