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
public class CTextView extends TextView {

    private Paint mPaint;

    private float mFrameStrokeWidth;
    private int mFrameColor;

    public CTextView(Context context) {
        this(context, null);
    }

    public CTextView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CTextView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initAttribute(context, attrs);
        initFramePaint();
    }

    private void initAttribute(Context context, AttributeSet attrs) {
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.CTextView);
        mFrameStrokeWidth = a.getDimension(R.styleable.CTextView_frameStrokeWidth, 4);
        mFrameColor = a.getColor(R.styleable.CTextView_frameColor, Color.GREEN);
        a.recycle();
    }


    private void initFramePaint() {
        mPaint = new Paint();
        mPaint.setColor(mFrameColor);
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(mFrameStrokeWidth);
    }


    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (isSelected()) {
            Drawable drawable = getCompoundDrawables()[1];
            Rect bounds = drawable.getBounds();
//            bounds.left+=mFrameStrokeWidth;
//            bounds.top+=mFrameStrokeWidth;
//            bounds.right-=mFrameStrokeWidth;
//            bounds.bottom-=mFrameStrokeWidth;
            canvas.drawRect(bounds, mPaint);
        }
    }


    @Override
    public void setSelected(boolean selected) {
        super.setSelected(selected);
        invalidate();
    }
}
