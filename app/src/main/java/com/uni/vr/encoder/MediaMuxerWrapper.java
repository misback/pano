package com.uni.vr.encoder;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
public class MediaMuxerWrapper {
	private static final String TAG = MediaMuxerWrapper.class.getSimpleName();
	private String outPutPath;
	private MediaMuxer mediaMuxer;	// API >= 18
	private volatile int mEncoderCount = 0;
	private volatile int mStartCount = 0;
	private volatile long mVideoEncodeCount = 0;
	private volatile boolean mIsStarted = false;
	private VideoMediaEncoder videoMediaEncoder;
	private AudioMediaEncoder audioMediaEncoder;
	private long startEncodeTime = 0;
	private long preEncodeTime = 0;
	public interface MediaMuxerWrapperListener {
		public void onStarted();
		public void onStopped();
		public void onUpdateTime(long seconds);
	}
	MediaMuxerWrapperListener mediaMuxerWrapperListener;
	public MediaMuxerWrapper(String outPutPath, MediaMuxerWrapperListener mediaMuxerWrapperListener) {
		try {
			this.outPutPath	= outPutPath;
			this.mediaMuxerWrapperListener = mediaMuxerWrapperListener;
			mediaMuxer = new MediaMuxer(outPutPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
			mIsStarted = false;
		}catch (Exception e){
			e.printStackTrace();
		}
	}
	public void prepare() throws IOException {
		if (videoMediaEncoder != null)
			videoMediaEncoder.prepare();
		if (audioMediaEncoder != null)
			audioMediaEncoder.prepare();
	}
	public void startRecording() {
		if (videoMediaEncoder != null)
			videoMediaEncoder.startRecording();
		if (audioMediaEncoder != null)
			audioMediaEncoder.startRecording();
	}

	public void stopRecording() {
		if (videoMediaEncoder != null)
			videoMediaEncoder.stopRecording();
		videoMediaEncoder = null;
		if (audioMediaEncoder != null)
			audioMediaEncoder.stopRecording();
		audioMediaEncoder = null;
	}

	public synchronized boolean isStarted() {
		return mIsStarted;
	}
	public void addEncoder(final VideoMediaEncoder videoMediaEncoder, AudioMediaEncoder audioMediaEncoder) {
		this.videoMediaEncoder = videoMediaEncoder;
		this.audioMediaEncoder = audioMediaEncoder;
		mEncoderCount = 2;
	}

	synchronized boolean start() {
		mStartCount ++;
		if (mEncoderCount>0 && (mEncoderCount == mStartCount) && mediaMuxer!=null && !mIsStarted && mediaMuxerWrapperListener!=null) {
			mediaMuxerWrapperListener.onStarted();
			mediaMuxer.start();
			mIsStarted = true;
			startEncodeTime = 0;
			preEncodeTime = 0;
			notifyAll();
		}
		return mIsStarted;
	}

	synchronized void stop() {
		mStartCount --;
		if (mEncoderCount>0 && mStartCount<=0 && mediaMuxer!=null && mIsStarted && mediaMuxerWrapperListener!=null) {
			mediaMuxer.stop();
			mediaMuxer.release();
			mediaMuxer= null;
			mIsStarted = false;
			mediaMuxerWrapperListener.onStopped();
		}
	}

	synchronized int addTrack(final MediaFormat format) {
		final int trackIx = mediaMuxer.addTrack(format);
		return trackIx;
	}

	synchronized void writeSampleData(final int trackIndex, final ByteBuffer byteBuf, final MediaCodec.BufferInfo bufferInfo) {
		if(mediaMuxer!=null && mIsStarted) {
			mediaMuxer.writeSampleData(trackIndex, byteBuf, bufferInfo);
			if(startEncodeTime == 0){
				startEncodeTime = bufferInfo.presentationTimeUs;
				preEncodeTime = 0;
			}else{
				long microSeconds = (bufferInfo.presentationTimeUs - startEncodeTime)/1000L;
				if ((microSeconds-preEncodeTime)>999){
					preEncodeTime = microSeconds;
					mediaMuxerWrapperListener.onUpdateTime(microSeconds);
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
	public void frameAvailableSoon(){
		if(videoMediaEncoder!=null){
			videoMediaEncoder.frameAvailableSoon();
		}
	}

}
