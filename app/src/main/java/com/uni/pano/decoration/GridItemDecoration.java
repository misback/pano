package com.uni.pano.decoration;

import android.content.Context;
import android.graphics.Rect;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import com.uni.pano.R;

/**
 * @描述：     @上边距
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class GridItemDecoration extends RecyclerView.ItemDecoration {

    private int mTopMargin;

    public GridItemDecoration(Context context) {
        mTopMargin = context.getResources().getDimensionPixelOffset(R.dimen.dp_20);
    }

    @Override
    public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
        outRect.set(0, mTopMargin, 0, 0);
    }
}