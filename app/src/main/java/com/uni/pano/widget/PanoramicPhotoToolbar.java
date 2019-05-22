package com.uni.pano.widget;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.LinearLayout;

import com.uni.common.util.AnimationUtil;

public class PanoramicPhotoToolbar extends LinearLayout {

    public PanoramicPhotoToolbar(Context context) {
        super(context);
    }

    public PanoramicPhotoToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PanoramicPhotoToolbar(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public PanoramicPhotoToolbar(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public void hide() {
        AnimationUtil.hideByAlpha(this, 0);
    }

    public void show() {
        AnimationUtil.showByAlpha(this, 0);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return true;
    }
}
