package com.uni.pano.widget;

import android.content.Context;
import android.support.v4.view.ViewCompat;
import android.support.v4.widget.ViewDragHelper;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;

/**
 * 自定义水平滑动选择控件
 * 只对其子view有效
 * Created by ZachLi on 2016/5/31.
 */
public class SwitchView extends LinearLayout {

    public static final int SWITCH_LEFT = 1;
    public static final int SWITCH_RIGHT = 2;
    private static final String TAG = "SwitchView";
    //当前状态
    private int curStatus = SWITCH_LEFT;
    private View mChildView;
    private ViewDragHelper mViewDragHelper;
    private OnChangeListener mSwitchListener;

    public SwitchView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mViewDragHelper = ViewDragHelper.create(this, 1.0f, new ScrollDragHelper());
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        return mViewDragHelper.shouldInterceptTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        mViewDragHelper.processTouchEvent(event);
        return true;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        int childCount = getChildCount();
        mChildView = getChildAt(childCount - 1);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        int switchViewWidth = 0;

        int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            switchViewWidth += getChildAt(i).getMeasuredWidth();
        }
        int childHeight = getChildAt(0).getMeasuredHeight();

        switchViewWidth += switchViewWidth / 2; //让父布局宽度比子控件宽度是子view总宽度的1.5倍

        setMeasuredDimension(switchViewWidth, childHeight);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);

        if (curStatus == SWITCH_LEFT) {
            switchLeft();
        } else {
            switchRight();
        }
    }

    @Override
    public void computeScroll() {
        if (mViewDragHelper.continueSettling(true)) {
            ViewCompat.postInvalidateOnAnimation(this);
        }
    }

    /**
     * 当选择改变时通知给订阅者
     */
    private void callOnSwitchChange(int status) {

        if (mSwitchListener != null) {
            mSwitchListener.onSwitchChange(status);
        }
    }

    /**
     * 切换到左边
     */
    public final void switchLeft() {
        this.curStatus = SWITCH_LEFT;
        mViewDragHelper.smoothSlideViewTo(mChildView, 0, 0);
        ViewCompat.postInvalidateOnAnimation(this);
    }

    /**
     * 切换到右边
     */
    public final void switchRight() {

        this.curStatus = SWITCH_RIGHT;
        int rightBound = getMeasuredWidth() - mChildView.getMeasuredWidth();
        mViewDragHelper.smoothSlideViewTo(mChildView, rightBound, 0);
        ViewCompat.postInvalidateOnAnimation(this);

    }

    /**
     * 注册选择状态改变监听器
     *
     * @param listener
     */
    public final void setOnSwitchChangeListener(OnChangeListener listener) {
        this.mSwitchListener = listener;
    }

    public interface OnChangeListener {
        public void onSwitchChange(int status);
    }

    private final class ScrollDragHelper extends ViewDragHelper.Callback {

        @Override
        public boolean tryCaptureView(View child, int pointerId) {
            return true;
        }

        @Override
        public int clampViewPositionHorizontal(View child, int left, int dx) {
            final int leftBound = getPaddingLeft();
            final int rightBound = getWidth() - mChildView.getWidth() - leftBound;
            final int newLeft = Math.min(Math.max(left, leftBound), rightBound);
            return newLeft;
        }


        @Override
        public int getViewHorizontalDragRange(View child) {
            return getMeasuredWidth() - child.getMeasuredWidth();
        }

        //松开之后的处理
        @Override
        public void onViewReleased(View releasedChild, float xvel, float yvel) {
            //子view到右边界距离
            float childViewRight = mChildView.getX() + mChildView.getMeasuredWidth();
            float rightDistance = getMeasuredWidth() - childViewRight;
            //子view到左边界距离
            float leftDistance = mChildView.getLeft();

            //哪头到父view的距离最小则表示选择哪头
            if (leftDistance < rightDistance) {
                switchLeft();
                callOnSwitchChange(SWITCH_LEFT);
            } else {
                switchRight();
                callOnSwitchChange(SWITCH_RIGHT);
            }

        }
    }

}
