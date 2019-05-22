package com.uni.pano.activities;

import android.content.Context;
import android.media.SoundPool;

import com.uni.pano.R;

/**
 * @描述：     @快门声音管理
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class SoundPoolManage {

    private SoundPool mSoundPool;

    private int mSoundId = -1;

    public SoundPoolManage(Context context){
        mSoundPool = new SoundPool.Builder().build();
        mSoundId   = mSoundPool.load(context, R.raw.camera_click,1);
    }

    /**
     * 播放声音
     */
    public final void playSound(){
        if(mSoundId > 0){
            mSoundPool.play(mSoundId, 1, 1, 0, 0, 1);
        }
    }

    /**
     * 释放资源
     */
    public final void releaseSound(){
        if(null != mSoundPool && mSoundId > 0){
            mSoundPool.release();
            mSoundId = -1;
            mSoundPool = null;
        }
    }

}
