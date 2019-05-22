package com.uni.common.adapter;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

/**
 * @描述：     @万能RecyclerView适配器
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public abstract class AbstractRecyclerAdapter<T> extends RecyclerView.Adapter<CommonRecyclerHolder> {

    private Context mContext;//上下文
    private List<T> mDatalist;//数据源
    private LayoutInflater inflater;//布局器
    private int itemLayoutId;//布局id
    private boolean isScrolling;//是否在滚动
    private OnItemClickListener mItemlistener;//点击事件监听器
    private OnItemLongClickListener mLongClickListener;//长按监听器
    private RecyclerView mRecyclerView;

    //在RecyclerView提供数据的时候调用
    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        super.onAttachedToRecyclerView(recyclerView);
        this.mRecyclerView = recyclerView;
        this.mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
            @Override
            public void onScrollStateChanged(RecyclerView mRecyclerView, int newState) {
                super.onScrollStateChanged(mRecyclerView, newState);
                isScrolling = !(newState == RecyclerView.SCROLL_STATE_IDLE);
                if (!isScrolling) {
                    notifyDataSetChanged();
                }
            }
        });
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerView recyclerView) {
        super.onDetachedFromRecyclerView(recyclerView);
        this.mRecyclerView = null;
    }

    /**
     * 定义一个点击事件接口回调
     */
    public interface OnItemClickListener<T> {
        void onItemClick(CommonRecyclerHolder holder,RecyclerView parent, View view, T item,int position);
    }

    public interface OnItemLongClickListener {
        boolean onItemLongClick(RecyclerView parent, View view, int position);
    }

    /**
     * 添加一项
     * @param item
     * @param position
     */
    public final void add(T item, int position) {
        mDatalist.add(position, item);
        notifyItemInserted(position);
    }

    /**
     * 插入所有
     * @param list
     */
    public final void addAll(List<T> list){
        mDatalist.addAll(list);
        notifyDataSetChanged();
    }

    public final List<T> getDatalist(){
        return mDatalist;
    }
    /**
     * 删除一项
     *
     * @param position 删除位置
     */
    public void delete(int position) {
        mDatalist.remove(position);
        notifyItemRemoved(position);
    }


    public AbstractRecyclerAdapter(Context context, List<T> list, int itemLayoutId) {
        this.mContext  = context;
        this.mDatalist = list;
        this.itemLayoutId = itemLayoutId;
        inflater = LayoutInflater.from(context);


    }

    @Override
    public CommonRecyclerHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = inflater.inflate(itemLayoutId, parent, false);
        return CommonRecyclerHolder.getRecyclerHolder(mContext, view);
    }

    @Override
    public void onBindViewHolder(final CommonRecyclerHolder holder, final int position) {

        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mItemlistener != null && view != null && mRecyclerView != null) {
                    int index = mRecyclerView.getChildAdapterPosition(view);
                    mItemlistener.onItemClick(holder,mRecyclerView, view,
                            mDatalist.get(position),index);
                }
            }
        });


        holder.itemView.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                if (mLongClickListener != null && view != null && mRecyclerView != null) {
                    int position = mRecyclerView.getChildAdapterPosition(view);
                    mLongClickListener.onItemLongClick(mRecyclerView, view, position);
                    return true;
                }
                return false;
            }
        });

        bindViewData(holder, mDatalist.get(position), position, isScrolling);

    }

    @Override
    public int getItemCount() {
        return mDatalist == null ? 0 : mDatalist.size();
    }

    public void setOnItemClickListener(OnItemClickListener listener) {
        this.mItemlistener = listener;
    }

    public void setOnItemLongClickListener(OnItemLongClickListener longClickListener) {
        this.mLongClickListener = longClickListener;
    }

    /**
     * 填充RecyclerView适配器的方法，子类需要重写
     *
     * @param holder      ViewHolder
     * @param item        子项
     * @param position    位置
     * @param isScrolling 是否在滑动
     */
    public abstract void bindViewData(CommonRecyclerHolder holder, T item, int position, boolean isScrolling);
}
