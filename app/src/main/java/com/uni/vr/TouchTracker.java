package com.uni.vr;
import android.content.Context;
import android.graphics.PointF;
import android.util.Log;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.View.OnTouchListener;
public class TouchTracker implements OnTouchListener {
    private static final float DEFAULT_DEGREES_PER_DPI_FACTOR = 0.033333335F;
    private PointF pxToDegreesFactor = new PointF();
    private final float dipToPx;
    private final GestureDetector mGestureDetector;
    private final ScaleGestureDetector mScaleGestureDetector;
    private final OnGestureListener mOnGestureListener;
    private OnGestureTracker mOnGestureTracker;
    private boolean mIsScale   =   false;
    private boolean mIsScroll   =   false;
    public TouchTracker(Context context, OnGestureListener gestureListener) {
        this.dipToPx = TypedValue.applyDimension(1, 1.0f, context.getResources().getDisplayMetrics());
        this.mScaleGestureDetector = new ScaleGestureDetector(context, new OnScaleTracker());
        mOnGestureTracker = new OnGestureTracker();
        this.mGestureDetector = new GestureDetector(context, mOnGestureTracker);
        this.mOnGestureListener = gestureListener;
        this.setTouchSpeed(DEFAULT_DEGREES_PER_DPI_FACTOR, DEFAULT_DEGREES_PER_DPI_FACTOR);
    }
    public void setTouchSpeed(float xSpeed, float ySpeed) {
        this.pxToDegreesFactor.set(this.dipToPx * xSpeed, this.dipToPx * ySpeed);
    }
    @Override
    public boolean onTouch(View view, MotionEvent event) {
        if (event.getPointerCount() >= 2) {
            mIsScale = true;
            mIsScroll   =   false;
            return mScaleGestureDetector.onTouchEvent(event);
        } else {
            mIsScale = false;
            return mGestureDetector.onTouchEvent(event);
        }
    }

    /**
     * 手势跟踪器：缩放
     */
    private class OnScaleTracker implements ScaleGestureDetector.OnScaleGestureListener {

        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            float cp = detector.getCurrentSpan() - detector.getPreviousSpan();
            mIsScale = true;
            mIsScroll = false;
            return mOnGestureListener.onScale(cp);
        }

        @Override
        public boolean onScaleBegin(ScaleGestureDetector detector) {
            mIsScale = true;
            mIsScroll = false;
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {
            mIsScale = false;
            mIsScroll = false;
        }
    }

    /**
     * 手势跟踪器：单击和拖动
     */
    private class OnGestureTracker extends GestureDetector.SimpleOnGestureListener {
        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            if(mIsScroll) {
                return mOnGestureListener.onDrag(distanceX * pxToDegreesFactor.x, distanceY * pxToDegreesFactor.y);
            }
            return mIsScroll;
        }
        public boolean onSingleTapUp(MotionEvent e) {
            return false;
        }

        @Override
        public void onShowPress(MotionEvent e) {

        }

        public void onLongPress(MotionEvent e) {
        }

        public boolean onSingleTapConfirmed(MotionEvent e) {
            return mOnGestureListener.onClick();
        }

        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            if(mIsScroll) {
                return mOnGestureListener.onFling(e1, e2, velocityX * pxToDegreesFactor.x, velocityY * pxToDegreesFactor.y);
            }
            return true;
        }

        public boolean onDown(MotionEvent e) {
            mIsScroll = true;
            return false;
        }

        public boolean onDoubleTap(MotionEvent e) {
            return false;
        }

        public boolean onDoubleTapEvent(MotionEvent e) {
            return false;
        }

        public boolean onContextClick(MotionEvent e) {
            return false;
        }
    }

    /**
     * 手势监听器
     */
    public interface OnGestureListener {

        /**
         * 点击回调
         */
        boolean onClick();

        /**
         * 缩放回调
         *
         * @param angle 角度
         */
        boolean onScale(float angle);

        /**
         * 拖动回调
         *
         * @param yaw   x轴拖动距离
         * @param pitch y轴拖动距离
         */
        boolean onDrag(float yaw, float pitch);
        boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY);
    }
}
