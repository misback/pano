package com.uni.common.image;

import android.app.Activity;
import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.widget.ImageView;

import com.bumptech.glide.DrawableRequestBuilder;
import com.bumptech.glide.Glide;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.uni.common.util.DeviceUtil;
import com.uni.pano.utils.CommonUtil;

public class IndeterminateDrawerView extends ImageView {

    private String mUrl;
    private int mErrorholder = -1, mPlaceholder = -1;
    private static int mScreenWidth;
    private Activity mActivity;

    public IndeterminateDrawerView(Context context) {
        super(context);
        initPost();
    }

    public IndeterminateDrawerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initPost();
    }

    public IndeterminateDrawerView(Context context, AttributeSet attrs, int style) {
        super(context, attrs, style);
        initPost();
    }

    private void initPost() {
        mActivity = (Activity) getContext();
        if (mScreenWidth == 0)
            mScreenWidth = DeviceUtil.getScreenWidth(mActivity);
    }

    public void setThumbnailURI(String url) {
        if (TextUtils.isEmpty(url))
            return;
        setImageURI(CommonUtil.getThumbnailUrl(url));
    }

    public void setThumbnailURISize(String url) {
        if (TextUtils.isEmpty(url))
            return;
        displayImage2(CommonUtil.getThumbnailUrl(url));
    }

    public void setThumbnailURIAlbum(String url) {
        if (TextUtils.isEmpty(url))
            return;
        setImageURIAlbum(CommonUtil.getThumbnailUrl(url));
    }
    public void setImageURIAlbum(String url) {
        if (TextUtils.isEmpty(url))
            return;
        mUrl = url;
        if(mUrl.startsWith("http")){
            displayImageSize(mUrl);
        } else {
            displayImageAlbum(mUrl);
        }
    }
    public void setImageURI(String url) {
        if (TextUtils.isEmpty(url))
            return;
        mUrl = url;
        if(mUrl.startsWith("http")){
            displayImageSize(mUrl);
        } else {
            displayImage(mUrl);
        }
    }

    private void displayImage(String url) {
        if(mActivity == null || mActivity.isFinishing() ||mActivity.isDestroyed()) return;
        DrawableRequestBuilder drb = Glide
                .with(mActivity)
                .load(url)
                .centerCrop()
                .diskCacheStrategy(DiskCacheStrategy.ALL)
                .crossFade(0);
        if (mErrorholder > 0) {
            drb.error(mErrorholder);
        }
        if (mPlaceholder > 0) {
            drb.placeholder(mPlaceholder);
        }
        drb.into(this);
    }

    private void displayImageSize(String url) {
        if(mActivity == null || mActivity.isFinishing() ||mActivity.isDestroyed()) return;
        int w = getWidth();
        int h = getHeight();
        if (w == 0 || w == mScreenWidth) {
            w = (int) (mScreenWidth * 0.8);
            h = (int) (w * 0.8);
        } else if (h == 0) {
            h = (int) (w * 0.8);
        }
        DrawableRequestBuilder drb = Glide
                .with(mActivity)
                .load(new SmartImageSizeModelImp(url))
                .centerCrop()
                .override(w, h)
                .diskCacheStrategy(DiskCacheStrategy.ALL)
                .crossFade(0);
        if (mErrorholder > 0) {
            drb.error(mErrorholder);
        }
        if (mPlaceholder > 0) {
            drb.placeholder(mPlaceholder);
        }
        drb.into(this);
    }


    private void displayImageAlbum(String url) {
        if(mActivity == null || mActivity.isFinishing() ||mActivity.isDestroyed()) return;
        int w = getWidth();
        int h = getHeight();
        if (w == 0 || h == 0) {
            w = mScreenWidth;
            h = (int)(w*0.55);
        }

        DrawableRequestBuilder drb = Glide
                .with(mActivity)
                .load(url)
                .centerCrop()
                .override(w, h)
                .diskCacheStrategy(DiskCacheStrategy.ALL)
                .crossFade(0);
        if (mErrorholder > 0) {
            drb.error(mErrorholder);
        }
        if (mPlaceholder > 0) {
            drb.placeholder(mPlaceholder);
        }
        drb.into(this);
    }

    private void displayImage2(String url) {
        if(mActivity == null || mActivity.isFinishing() ||mActivity.isDestroyed()) return;
        int w = getWidth();
        int h = getHeight();
        if (w == 0 || h == 0) {
            w = mScreenWidth /3;
            h = w;
        }
        w *= 0.7;
        h *= 0.7;

        DrawableRequestBuilder drb = Glide
                .with(mActivity)
                .load(url)
                .centerCrop()
                .override(w, h)
                .diskCacheStrategy(DiskCacheStrategy.ALL)
                .crossFade(0);
        if (mErrorholder > 0) {
            drb.error(mErrorholder);
        }
        if (mPlaceholder > 0) {
            drb.placeholder(mPlaceholder);
        }
        drb.into(this);
    }

    public void setErrorholder(int errorholder) {
        this.mErrorholder = errorholder;
    }

    public void setPlaceholder(int placeholder) {
        this.mPlaceholder = placeholder;
    }
}
