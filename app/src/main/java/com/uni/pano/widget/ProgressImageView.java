package com.uni.pano.widget;

import android.animation.FloatEvaluator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.ImageView;

import com.uni.pano.R;

/**
 * @描述：     @自定义分享ImageView
 * @作者：     @蒋诗朋
 * @创建时间： @2017-06-12
 */
public class ProgressImageView extends ImageView{

    private final static float MAX_PROGRESS = 100f; // 不可以修改的最大值

    private int     mTextColor;
    private float   mTextSize;
    private int     mWidth;
    private int     mHeight;
    private Paint   mTextPaint;

    private float   mProgress = 0f;

    private ValueAnimator mValueAnimator;

    public ProgressImageView(Context context) {
        this(context, null);
    }

    public ProgressImageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ProgressImageView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // 初始化属性
        initAttrs(context, attrs, defStyleAttr);
    }

    private final void initAttrs(Context context,AttributeSet attrs, int defStyleAttr){
          TypedArray a    = null;
        try{
            a             = context.obtainStyledAttributes(attrs,R.styleable.ProgressImageView);
            mTextColor    = a.getColor(R.styleable.ProgressImageView_text_color,
                    getResources().getColor(android.R.color.holo_blue_dark));
            mTextSize     = a.getDimension(R.styleable.ProgressImageView_text_size, 22f);
        }finally {
            if(null != a){
                a.recycle();
            }
        }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        // 获取总的宽高
        mWidth  = getMeasuredWidth();
        mHeight = getMeasuredHeight();
        // 设置文字画笔
        initTextPaint();
    }

    /**
     * 设置文字画笔
     */
    private void initTextPaint() {
        mTextPaint = new Paint();
        mTextPaint.setAntiAlias(true);
        mTextPaint.setColor(mTextColor);
        mTextPaint.setTextSize(mTextSize);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        // 第二步：绘制文字
        String text = ((int) (mProgress / MAX_PROGRESS * 100)) + "%";
        Rect bounds = new Rect();
        mTextPaint.getTextBounds(text, 0, text.length(), bounds);

        canvas.drawText(text, mWidth / 2 - bounds.width() / 2,
                mHeight / 2 + bounds.height() / 2, mTextPaint);

    }

    /**
     * 设置刻度值
     * @param progress
     */
    public final void setProgress(float progress){
        this.mProgress = progress;
        // 使用 postInvalidate 比 postInvalidat() 好，线程安全
        postInvalidate();

    }


    /**
     * 开启动画
     * @param targetProgress
     */
    public final void startAnimate(float targetProgress){
        stopAnimate();
        mValueAnimator = ValueAnimator.ofObject(new FloatEvaluator(), 0, targetProgress);
        // 设置差值器
        mValueAnimator.setInterpolator(new AccelerateDecelerateInterpolator());
        mValueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float value = (float) animation.getAnimatedValue();
                setProgress(value);
            }
        });
        mValueAnimator.setDuration((long) (targetProgress * 33));
        mValueAnimator.start();
    }

    /**
     * 停止动画
     */
    public final void stopAnimate() {
        if (mValueAnimator != null && mValueAnimator.isRunning()) {
            mValueAnimator.cancel();
        }
    }
}
