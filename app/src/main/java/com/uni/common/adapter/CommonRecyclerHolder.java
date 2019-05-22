package com.uni.common.adapter;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.support.v7.widget.RecyclerView;
import android.util.SparseArray;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.load.engine.bitmap_recycle.BitmapPool;
import com.bumptech.glide.load.resource.bitmap.BitmapTransformation;
import com.uni.pano.R;

/**
 * @描述：     @万能RecyclerView ViewHolder
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class CommonRecyclerHolder extends RecyclerView.ViewHolder {

    private SparseArray<View> mSparseViews;

    private Context mContext;

    private CommonRecyclerHolder(Context context, View itemView) {
        super(itemView);
        //指定一个初始为8
        mContext     = context;
        mSparseViews = new SparseArray<>(8);


    }

    /**
     * 取得一个RecyclerHolder对象
     * @param context 上下文
     * @param itemView 子项
     * @return 返回一个RecyclerHolder对象
     */
    public static CommonRecyclerHolder getRecyclerHolder(Context context, View itemView){
        return new CommonRecyclerHolder(context,itemView);
    }

    /**
     * 获取所有控件
     * @return
     */
    public final SparseArray<View> getSparseViews(){
        return this.mSparseViews;
    }

    /**
     * 获取控制id
     * @param viewId
     * @param <T>
     * @return
     */
    @SuppressWarnings("unchecked")
    public <T extends View> T getView(int viewId){
        View view = mSparseViews.get(viewId);
        if (view == null ){
            view = itemView.findViewById(viewId);
            mSparseViews.put(viewId,view);
        }
        return (T) view;
    }

    /**
     * 设置setText
     * @param viewId
     * @param text
     * @return
     */
    public final CommonRecyclerHolder setText(int viewId, String text){
        TextView tv = getView(viewId);
        tv.setText(text);
        return this;
    }

    /**
     * 设置控制是否可见
     * @param viewId
     * @param visible
     * @return
     */
    public final CommonRecyclerHolder setVisibility(int viewId,boolean visible){
        final View view = getView(viewId);
        view.setVisibility(visible?View.VISIBLE:View.GONE);
        return this;
    }

    /**
     * 设置图片背景
     * @param viewId
     * @param drawableId
     * @return
     */
    public final CommonRecyclerHolder setImageResource(int viewId, int drawableId){
        ImageView iv = getView(viewId);
        iv.setImageResource(drawableId);
        return this;
    }

    /**
     * 设置图片背景，以bitmap
     * @param viewId
     * @param bitmap
     * @return
     */
    public final CommonRecyclerHolder setImageBitmap(int viewId, Bitmap bitmap){
        ImageView iv = getView(viewId);
        iv.setImageBitmap(bitmap);
        return this;
    }

    /**
     * 设置图片url，网络加载
     * @param viewId
     * @param url
     * @return
     */
    public final CommonRecyclerHolder setImageByUrl(int viewId, String url,int defaultId){
        ImageView iv = getView(viewId);
        Glide.with(mContext).load(url).crossFade(0).transform(getGlideRound()).placeholder(defaultId).into(iv);
        return this;
    }


    /**
     * 加载视频截图
     * @param viewId
     * @param url
     * @return
     */
    public final CommonRecyclerHolder setVideoThumbUrl(int viewId, String url,int defaultId){
        ImageView iv = getView(viewId);
        try {
            Glide.with(mContext).load(url).crossFade(0).transform(getGlideRound())
                    .placeholder(defaultId).into(iv);
        }catch (Exception e) {
            Glide.with(mContext).load(defaultId).crossFade(0).transform(getGlideRound())
                    .placeholder(defaultId).into(iv);
        }
        return this;
    }


    private final GlideRoundTransform getGlideRound(){
        return new GlideRoundTransform(
                mContext, (int) mContext.getResources().getDimension(R.dimen.dp_6));
    }

    private class GlideRoundTransform extends BitmapTransformation {

        private float radius = 0f;

        public GlideRoundTransform(Context context) {
            this(context, (int) context.getResources().getDimension(R.dimen.dp_5));
        }

        public GlideRoundTransform(Context context, int radius) {
            super(context);
            this.radius = radius;
        }

        @Override protected Bitmap transform(BitmapPool pool, Bitmap toTransform, int outWidth, int outHeight) {
            return roundCrop(pool, toTransform);
        }

        private Bitmap roundCrop(BitmapPool pool, Bitmap source) {
            if (source == null) return null;

            Bitmap result = pool.get(source.getWidth(), source.getHeight(), Bitmap.Config.ARGB_8888);
            if (result == null) {
                result = Bitmap.createBitmap(source.getWidth(), source.getHeight(), Bitmap.Config.ARGB_8888);
            }

            Canvas canvas = new Canvas(result);
            Paint paint = new Paint();
            paint.setShader(new BitmapShader(source, BitmapShader.TileMode.CLAMP, BitmapShader.TileMode.CLAMP));
            paint.setAntiAlias(true);
            RectF rectF = new RectF(0f, 0f, source.getWidth(), source.getHeight());
            canvas.drawRoundRect(rectF, radius, radius, paint);
            return result;
        }

        @Override public String getId() {
            return getClass().getName() + Math.round(radius);
        }
    }
}
