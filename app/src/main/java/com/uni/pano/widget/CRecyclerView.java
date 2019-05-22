package com.uni.pano.widget;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.View;

/**
 * @描述：     @相册
 * @作者：     @蒋诗朋
 * @创建时间： @2017-05-10
 */
public class CRecyclerView extends RecyclerView{

    private View mEmptyView;

    private View mParent;

    final private AdapterDataObserver mObserver = new AdapterDataObserver() {
        @Override
        public void onChanged() {
            checkIfEmpty();
        }

        @Override
        public void onItemRangeInserted(int positionStart, int itemCount) {
            checkIfEmpty();
        }

        @Override
        public void onItemRangeRemoved(int positionStart, int itemCount) {
            checkIfEmpty();
        }
    };

    public CRecyclerView(Context context) {
        super(context);
    }

    public CRecyclerView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public CRecyclerView(Context context, AttributeSet attrs,
                             int defStyle) {
        super(context, attrs, defStyle);
    }

    private void checkIfEmpty() {
        if (mEmptyView != null && getAdapter() != null && null != mParent) {
            final boolean emptyViewVisible =
                    getAdapter().getItemCount() == 0;
            mEmptyView.setVisibility(emptyViewVisible ? VISIBLE : GONE);
            mParent.setVisibility(emptyViewVisible ? GONE : VISIBLE);
        }
    }

    @Override
    public void setAdapter(Adapter adapter) {
        final Adapter oldAdapter = getAdapter();
        if (oldAdapter != null) {
            oldAdapter.unregisterAdapterDataObserver(mObserver);
        }
        super.setAdapter(adapter);
        if (adapter != null) {
            adapter.registerAdapterDataObserver(mObserver);
        }

        checkIfEmpty();
    }

    /**
     * 关联视图
     * @param emptyView
     */
    public void setEmptyView(View emptyView,View parent) {
        this.mEmptyView = emptyView;
        this.mParent    = parent;
        checkIfEmpty();
    }
}
