package com.uni.pano.activities;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.uni.common.dialog.CDialog;
import com.uni.common.file.DownloadProgressHandler;
import com.uni.common.util.ApkUtil;
import com.uni.common.util.PackageUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.api.ApiClient;
import com.uni.pano.api.ApiService;
import com.uni.pano.api.DownloadManage;
import com.uni.pano.api.RetrofitCallback;
import com.uni.pano.entity.Result;
import com.uni.pano.entity.VersionInfo;
import com.uni.pano.fragment.SettingFragment;
import com.uni.pano.widget.CToast;

import butterknife.BindView;
import butterknife.OnClick;
import retrofit2.Call;
import retrofit2.Response;

/**
 * @描述：     @检查更新对话框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class UpdateWindow extends CDialog {

    /**
     * 来源主界面
     */
    public final static int FROM_PANDA_ACTIVITY    = 1;

    /**
     * 来自设置检查更新界面
     */
    public final static int FROM_SETTING_FRAGMENT  = 2;

    @BindView(R.id.tv_new_version)
    TextView mTvNewVersion;

    @BindView(R.id.txt_cancel)
    TextView mTvCancel;

    @BindView(R.id.btn_confirm)
    Button mBtnConfirm;

    @BindView(R.id.pb_download)
    ProgressBar mProgressBar;

    @BindView(R.id.cb_not_prompt)
    CheckBox mCbNotPrompt;

    private String mNewVersion;

    private String mFileSize;

    private BlurListener mBlurListener;

    private VersionInfo mNewVerionInfo;

    private int mSource        = FROM_PANDA_ACTIVITY;

    private boolean mRecommend = false;

    public UpdateWindow(Activity activity,int source,
                        DialogListener dialogListener,
                        BlurListener listener) {
        super(activity,dialogListener,true,R.style.CDialog_Theme);
        this.mBlurListener    = listener;
        this.mSource          = source;
    }

    public boolean isRecommend() {
        return mRecommend;
    }

    public void setRecommend(boolean recommend) {
        this.mRecommend = recommend;
    }

    public void setNewVersion(String mNewVersion) {
        this.mNewVersion = mNewVersion;
    }


    public void setFileSize(String mFileSize) {
        this.mFileSize = mFileSize;
    }

    @Override
    public int getLayoutId() {
        return R.layout.view_update;
    }

    @OnClick({R.id.btn_confirm,R.id.txt_cancel,R.id.cb_not_prompt})
    public void onClick(View view){
        switch (view.getId()){
            case R.id.btn_confirm:
                mBtnConfirm.setEnabled(false);
                confirm(false);
                downloadApk();
                break;
            case R.id.txt_cancel:
                cancel();
                this.mBlurListener.hideBlur();
                break;
            case R.id.cb_not_prompt:
                if(PreferenceModel.isVersionNotPrompt()){
                    mCbNotPrompt.setChecked(false);
                    PreferenceModel.setVersionNotPrompt(false);
                }else{
                    mCbNotPrompt.setChecked(true);
                    PreferenceModel.setVersionNotPrompt(true);
                }
                break;
        }
    }

    public final void updateProgress(int progress,int max){
        mProgressBar.setVisibility(View.VISIBLE);
        mTvNewVersion.setVisibility(View.GONE);
        mProgressBar.setMax(max);
        mProgressBar.setProgress(progress);
    }

    @Override
    protected void afterShow() {
        switch (mSource){
            case FROM_PANDA_ACTIVITY:
                if(isRecommend()){
                    mCbNotPrompt.setVisibility(View.VISIBLE);
                    mTvCancel.setVisibility(View.VISIBLE);
                }else{
                    mCbNotPrompt.setVisibility(View.GONE);
                    mTvCancel.setVisibility(View.GONE);
                }
                break;
            case FROM_SETTING_FRAGMENT:
                mCbNotPrompt.setVisibility(View.INVISIBLE);
                break;
        }
        mTvNewVersion.setText(String.format(
                ResourceUtil.getString(R.string.find_new_version),mNewVersion));
        mBtnConfirm.setText(String.format(
                ResourceUtil.getString(R.string.download_now),mFileSize));
    }


    /**
     * 下载apk
     */
    public final void downloadApk(){
        if(null != mNewVerionInfo) {
            if (!TextUtils.isEmpty(mNewVerionInfo.downloadSuffixUrl)) {
                DownloadManage.downloadAPK(mNewVerionInfo.downloadPrefixUrl,
                        mNewVerionInfo.downloadSuffixUrl,
                        mNewVerionInfo.fileName,
                        new DownloadProgressHandler() {
                            @Override
                            protected void onProgress(long bytesRead, long total, boolean done) {
                                updateProgress((int) (bytesRead / 1024),
                                        (int) (total / 1024));
                            }

                            @Override
                            public void onComplete(String filePath) {
                                CToast.showToast(R.string.download_complete);
                                mBtnConfirm.setEnabled(true);
                                mBlurListener.hideBlur();
                                dismiss();
                                ApkUtil.installApk(getActivity(), filePath);
                            }

                            @Override
                            public void onFailure() {
                                mBtnConfirm.setEnabled(true);
                                mBtnConfirm.setText(String.format(
                                        ResourceUtil.getString(R.string.download_again),mFileSize));
                                CToast.showToast(R.string.download_failure);
                            }
                        });
            }
        }
    }

    /**
     * 检查新版本
     */
    public final void checkVersion(){
        if(!PreferenceModel.isVersionNotPrompt()
                || mSource == FROM_SETTING_FRAGMENT){
            final PackageInfo packageInfo        = PackageUtil.getPackageInfo(getActivity());
            final String packageName             = packageInfo.packageName;
            final int versionCode                = packageInfo.versionCode;
            final String versionName             = packageInfo.versionName;
            final ApiService apiService          = ApiClient.retrofit().create(ApiService.class);
            final Call<Result<VersionInfo>> call = apiService.checkVersion(packageName,1,versionCode,versionName);
            call.enqueue(new RetrofitCallback<Result<VersionInfo>>() {
                @Override
                public void onSuccess(Call<Result<VersionInfo>> call, Response<Result<VersionInfo>> response) {

                    boolean showNewVersion       = false;
                    final Result<VersionInfo> result = response.body();
                    if(null != result){
                        if(result.rtn == 1){
                            final VersionInfo v  = result.data;
                            mNewVerionInfo       = v;
                            if(v.publish && v.upgrade){
                                setRecommend(v.recommend);
                                if(null != mBlurListener){
                                    mBlurListener.showBlur();
                                }
                                setFileSize(v.fileSize);
                                setNewVersion(v.newVersionName);
                                showDialog();
                                showNewVersion = true;
                            }
                        }
                    }

                    if(!showNewVersion){
                        if(mSource == FROM_SETTING_FRAGMENT){
                            CToast.showToast(R.string.no_new_version);
                        }

                    }
                }

                @Override
                public void onFailure(Call<Result<VersionInfo>> call, Throwable t) {
                    if(mSource == FROM_SETTING_FRAGMENT){
                        CToast.showToast(R.string.check_update_failure);
                    }

                }
            });
        }
    }

    private final void showDialog(){
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                show();
            }
        });
    }

}