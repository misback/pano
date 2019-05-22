package com.uni.vr;



public interface TextureRenderListener {
	
	public void onRenderStarted();

	public void onRenderSlow(int framerate);

	public void onRenderStoped();
	
}