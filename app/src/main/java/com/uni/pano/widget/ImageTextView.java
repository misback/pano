package com.uni.pano.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.TextView;

import com.uni.pano.R;

/**
 * Created by ZachLi on 2016/9/13.
 */
public class ImageTextView extends TextView {
    int disableResId;
    int ableResId;
    int disableColor;
    int ableColor;
    Drawable ableDrawable;
    Drawable disableDrawable;
    public ImageTextView(Context context) {
        this(context, null);
    }

    public ImageTextView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ImageTextView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initAttribute(context, attrs);
    }

    private void initAttribute(Context context, AttributeSet attrs) {
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ImageTextView);
        disableResId = a.getResourceId(R.styleable.ImageTextView_disableResId, 0);
        ableResId = a.getResourceId(R.styleable.ImageTextView_ableResId, 0);
        disableColor = a.getColor(R.styleable.ImageTextView_disableColor, 0);
        ableColor = a.getColor(R.styleable.ImageTextView_ableColor, 0);
        ableDrawable = getResources().getDrawable(ableResId);
        disableDrawable = getResources().getDrawable(disableResId);
        a.recycle();
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        if (enabled){
            setTextColor(ableColor);
            setCompoundDrawables(ableDrawable, null, null, null);
        }else {
            setTextColor(disableColor);
            setCompoundDrawables(disableDrawable, null, null, null);
        }
    }
}
