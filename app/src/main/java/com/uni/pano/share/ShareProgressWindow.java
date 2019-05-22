package com.uni.pano.share;

import android.animation.ObjectAnimator;
import android.app.Activity;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.animation.LinearInterpolator;

import com.uni.common.concurrent.SingleThreadExecutor;
import com.uni.common.dialog.CDialog;
import com.uni.common.env.Env;
import com.uni.common.util.PackageUtil;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.activities.ShareActivity;
import com.uni.pano.api.ApiClient;
import com.uni.pano.api.ApiService;
import com.uni.pano.api.FileRequestBody;
import com.uni.pano.api.RetrofitCallback;
import com.uni.pano.entity.Result;
import com.uni.pano.entity.ShareInfo;
import com.uni.pano.widget.ProgressImageView;

import java.io.File;

import butterknife.BindView;
import butterknife.OnClick;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.RequestBody;
import retrofit2.Call;
import retrofit2.Response;

/**
 * @描述：     @分享进度弹出框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */

public class ShareProgressWindow extends CDialog {
    private static final String TAG           = "ShareProgressWindow";

    private static final int DURATION         = 10000;

    private static final int REPEAT_COUNT     = -1;

    private static final float START_ANGLE    = 0f;

    private static final float END_ANGLE      = 360f;

    @BindView(R.id.bg_progress)
    View mBgProgress;

    @BindView(R.id.iv_progress)
    ProgressImageView mIvProgress;

    private ApiService mApiService;

    private String mContent;
    private String mFilePath;
    private long mLastModified;

    private ShareActivity mShareActivity;

    private ObjectAnimator mBgAnimate;

    private Call<Result<ShareInfo>> mCallRequest = null;

    public void setShareContent(String content) {
        this.mContent = content;
    }


    public void setFilePath(String mFilePath) {
        this.mFilePath = mFilePath;
    }

    public long getLastModified() {
        return mLastModified;
    }

    public void setLastModified(long lastModified) {
        this.mLastModified = lastModified;
    }

    public ShareProgressWindow(Activity activity, DialogListener listener) {
        super(activity,listener,true,R.style.CDialog_Theme);
        mShareActivity = (ShareActivity) activity;
    }


    @Override
    public int getLayoutId() {
        return R.layout.view_share_progress;
    }


    @OnClick({R.id.txt_cancel})
    public void onClick(View view){
        switch (view.getId()){
            case R.id.txt_cancel:
                mShareActivity.showCancelWindow();
                break;
        }
    }

    @Override
    protected void initUI(View view) {
        mApiService   = ApiClient.retrofit().create(ApiService.class);
    }

    @Override
    protected void afterShow() {
        startBgAnimate();
        requestShare();
    }

    /**
     * 启动背景动画
     */
    private final void startBgAnimate(){
        mBgAnimate = ObjectAnimator.ofFloat(mBgProgress, "rotation",START_ANGLE, END_ANGLE);
        mBgAnimate.setInterpolator(new LinearInterpolator());
        mBgAnimate.setDuration(DURATION);
        mBgAnimate.setRepeatCount(REPEAT_COUNT);
        mBgAnimate.start();
    }

    /**
     * 停止背景动画
     */
    private final void stopBgAnimate(){
        if(null != mBgAnimate && mBgAnimate.isRunning()){
            mBgAnimate.cancel();
            mBgAnimate = null;
        }
    }

    /**
     * 上传文件
     */
    private final void requestShare(){
        RetrofitCallback<Result<ShareInfo>> callback = new RetrofitCallback<Result<ShareInfo>>() {

            final Handler mHandler = new Handler(){
                @Override
                public void handleMessage(Message msg) {
                    super.handleMessage(msg);
                    updateProgress(msg.what);
                }
            };

            private final void updateProgress(final float progress){
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mIvProgress.setProgress(progress);
                    }
                });
            }

            private final void stopAnimate(){
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        stopBgAnimate();
                    }
                });
            }

            @Override
            public void onSuccess(Call<Result<ShareInfo>> call,
                                  Response<Result<ShareInfo>> response) {
                Log.v(TAG,"分享成功---->" + response.body().toString());
                final Result<ShareInfo>  shareResult = response.body();
                if(null != shareResult &&
                        null != shareResult.data
                        && shareResult.rtn == 1){
                    mShareActivity.share(shareResult.data.shareUrl,
                            shareResult.data.thumbUrl);
                    stopAnimate();
                    dismiss();
                }else{
                    stopAnimate();
                    dismiss();
                    //执行分享失败
                    mShareActivity.shareFailure();
                }
            }

            @Override
            public void onFailure(Call<Result<ShareInfo>> call, Throwable t) {
                Log.v(TAG,"分享失败---->" + t.getMessage());
                //进度更新结束
                stopAnimate();
                dismiss();
                mShareActivity.shareFailure();
            }

            @Override
            public void onLoading(long total, long progress) {
                super.onLoading(total, progress);
                final float sampleProgress = progress * 100 /total;
                if(sampleProgress == 100){
                    mHandler.sendEmptyMessageDelayed((int) sampleProgress,4000);
                    return;
                }
                updateProgress(sampleProgress);
            }
        };

        final File file              = new File(mFilePath);
        final RequestBody body1      = RequestBody.create(MediaType.parse("multipart/form-data"), file);
        //通过该行代码将RequestBody转换成特定的FileRequestBody
        FileRequestBody body         = new FileRequestBody(body1, callback);
        MultipartBody.Part part      = MultipartBody.Part.createFormData("file", file.getName(), body);
        mCallRequest                 = mApiService.addShare(part,
                toRequestBody(ResourceUtil.getString(R.string.app_name)),
                toRequestBody(mContent),
                toRequestBody(PackageUtil.getVersionName(Env.getContext())),
                toRequestBody(String.valueOf(getLastModified())));
        mCallRequest.enqueue(callback);
    }

    /**
     * 取消请求
     */
    public final void cancelRequest(){
        if(null != mCallRequest && !mCallRequest.isCanceled()){
            mCallRequest.cancel();
        }
    }

    private final RequestBody toRequestBody(String str){
        return RequestBody.create(MediaType.parse("multipart/form-data"),str);
    }


}
