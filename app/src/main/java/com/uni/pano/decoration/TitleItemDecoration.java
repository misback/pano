package com.uni.pano.decoration;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.TypedValue;
import android.view.View;

import com.uni.common.util.DateTimeUtil;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.bean.MediaInfo;

import java.util.List;

/**
 * @描述：     @重写拓展悬浮title
 * @作者：     @蒋诗朋
 * @创建时间： @2017-05-10
 */
public class TitleItemDecoration extends RecyclerView.ItemDecoration {

    private static final String TAG = "TitleItemDecoration";

    private List<MediaInfo> mDatas;
    private Paint mPaint;
    private Rect  mBounds;//用于存放测量文字Rect

    private int mTitleHeight;//title的高
    private static int COLOR_TITLE_BG   = Color.parseColor("#FFFFFF");
    private static int COLOR_TITLE_FONT = Color.parseColor("#7D7D7D");
    private static int mTitleFontSize;//title字体大小


    public TitleItemDecoration(Context context, List<MediaInfo> datas) {
        super();
        mDatas         = datas;
        mPaint         = new Paint();
        mBounds        = new Rect();
        mTitleHeight   = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP,
                context.getResources().getDimension(R.dimen.dp_10),
                context.getResources().getDisplayMetrics());
        mTitleFontSize = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,
                context.getResources().getDimension(R.dimen.sp_5),
                context.getResources().getDisplayMetrics());
        mPaint.setTextSize(mTitleFontSize);
        mPaint.setAntiAlias(true);
    }


    @Override
    public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
        super.onDraw(c, parent, state);
        final int left  = parent.getPaddingLeft();
        final int right = parent.getWidth() - parent.getPaddingRight();
        final int childCount = parent.getChildCount();
        for (int i = 0; i < childCount; i++) {
            final View child = parent.getChildAt(i);
            final RecyclerView.LayoutParams params = (RecyclerView.LayoutParams) child
                    .getLayoutParams();
            int position = params.getViewLayoutPosition();
            if (position > -1) {
                if (position == 0) {
                    drawTitleArea(c, left, right, child, params, position);
                } else {//其他的通过判断
                    if(mDatas.size() > position && (position - 1) >=0){
                        if (null != mDatas.get(position).tag &&
                                !mDatas.get(position).tag.equals(mDatas.get(position - 1).tag)) {
                            //不为空 且跟前一个tag不一样了，说明是新的分类，也要title
                            drawTitleArea(c, left, right, child, params, position);
                        } else {
                            //none
                        }
                    }
                }
            }
        }
    }

    /**
     * 绘制Title区域背景和文字的方法
     *
     * @param c
     * @param left
     * @param right
     * @param child
     * @param params
     * @param position
     */
    private void drawTitleArea(Canvas c, int left, int right, View child, RecyclerView.LayoutParams params, int position) {//最先调用，绘制在最下层
        if(mDatas.size() > 0){
            mPaint.setColor(COLOR_TITLE_BG);
            c.drawRect(left, child.getTop() - params.topMargin - mTitleHeight,
                    right, child.getTop() - params.topMargin, mPaint);
            mPaint.setColor(COLOR_TITLE_FONT);
            String tag       = mDatas.get(position).tag;
            if(DateTimeUtil.getTodayDate().equals(tag)){
                tag          = ResourceUtil.getString(R.string.today);
            }
            mPaint.getTextBounds(tag, 0,
                    tag.length(), mBounds);
            c.drawText(tag, child.getPaddingLeft(),
                    child.getTop() - params.topMargin -
                            (mTitleHeight / 2 - mBounds.height() / 2), mPaint);
        }

    }

    @Override
    public void onDrawOver(Canvas c, RecyclerView parent, RecyclerView.State state) {//最后调用 绘制在最上层
        int pos = ((LinearLayoutManager)(parent.getLayoutManager())).findFirstVisibleItemPosition();
        if(mDatas.size() > 0){
            String tag = mDatas.get(pos).tag;
            if(DateTimeUtil.getTodayDate().equals(tag)){
                tag    = ResourceUtil.getString(R.string.today);
            }

            View child = parent.findViewHolderForLayoutPosition(pos).itemView;
            mPaint.setColor(COLOR_TITLE_BG);
            c.drawRect(parent.getPaddingLeft(), parent.getPaddingTop(),
                    parent.getRight() - parent.getPaddingRight(),
                    parent.getPaddingTop() + mTitleHeight, mPaint);
            mPaint.setColor(COLOR_TITLE_FONT);
            mPaint.getTextBounds(tag, 0, tag.length(), mBounds);
            c.drawText(tag, child.getPaddingLeft(),
                    parent.getPaddingTop() + mTitleHeight - (mTitleHeight / 2 - mBounds.height() / 2),
                    mPaint);
        }
    }

    @Override
    public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
        super.getItemOffsets(outRect, view, parent, state);
        int position = ((RecyclerView.LayoutParams) view.getLayoutParams()).getViewLayoutPosition();
        if (position > -1) {
            if (position == 0) {//等于0肯定要有title的
                outRect.set(0, mTitleHeight, 0, 0);
            } else {//其他的通过判断
                if(mDatas.size() > position && (position - 1) >=0){
                    if (null != mDatas.get(position).tag &&
                            !mDatas.get(position).tag.equals(mDatas.get(position - 1).tag)) {
                        outRect.set(0, mTitleHeight, 0, 0);//不为空 且跟前一个tag不一样了，说明是新的分类，也要title
                    } else {
                        outRect.set(0, 0, 0, 0);
                    }
                }
            }
        }
    }

}
