package com.uni.pano.activities;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.umeng.socialize.PlatformConfig;
import com.umeng.socialize.ShareAction;
import com.umeng.socialize.UMShareAPI;
import com.umeng.socialize.UMShareListener;
import com.umeng.socialize.bean.SHARE_MEDIA;
import com.umeng.socialize.media.UMImage;
import com.umeng.socialize.media.UMWeb;
import com.uni.common.dialog.CDialog;
import com.uni.pano.R;
import com.uni.pano.base.BaseActivity;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.blur.UtilBitmap;
import com.uni.pano.blur.UtilScreenCapture;
import com.uni.pano.config.KeyConstants;
import com.uni.pano.share.ShareCancelWindow;
import com.uni.pano.share.ShareProgressWindow;
import com.uni.pano.widget.CToast;

import butterknife.BindView;
import butterknife.OnClick;

/**
 * @描述：     @分享界面
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class ShareActivity extends BaseActivity {
    public static final String TAG = ShareActivity.class.getSimpleName();

    @BindView(R.id.tv_back)
    TextView mTvback;

    @BindView(R.id.tv_share_content_limit)
    TextView mTvLimit;

    @BindView(R.id.et_share_content)
    EditText mEtShareContent;

    @BindView(R.id.iv_blur)
    ImageView mIvBlur;

    @BindView(R.id.btn_share)
    Button mBtnShare;

    @BindView(R.id.ll_content_container)
    LinearLayout mLLcontentContainer;

    private MediaInfo mMediaInfo;
    private SHARE_MEDIA mShareMedia;

    private ShareProgressWindow mShareProgressWindow;
    private ShareCancelWindow  mShareCancelWindow;

    private WindowType mWindowType = WindowType.PROGRESS;

    /* 分享回调 */
    private UMShareListener umShareListener = new UMShareListener() {
        @Override
        public void onStart(SHARE_MEDIA share_media) {

        }
        @Override
        public void onResult(SHARE_MEDIA share_media) {
            CToast.showToast(R.string.share_tips_success_share);
            finish();
        }
        @Override
        public void onError(SHARE_MEDIA share_media, Throwable throwable) {
            CToast.showToast(R.string.share_tips_error_share);
            finish();
        }
        @Override
        public void onCancel(SHARE_MEDIA share_media) {
            CToast.showToast(R.string.cancel);
            finish();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.av_share);
        loadShareConfig();
        init();
    }

    //初始化
    private final void loadShareConfig(){
        // 分享平台密钥
//        final String WX_APPID = "wx1992016308339b32";
//        final String WX_APPSECRET = "a91a858c645ce1c209e9797d5810b773";
//        final String QQ_APPID = "1105924103";
//        final String QQ_APPSECRET = "tKBdYy1Vvp9zPBib";

//        PlatformConfig.setWeixin(KeyConstants.UU360_WECHAT_APP_ID,
//                KeyConstants.UU360_WECHAT_APP_SECRET);
//        PlatformConfig.setQQZone(KeyConstants.UU360_QQ_OPEN_APP_ID,
//                KeyConstants.UU360_QQ_OPEN_APP_KEY);

        PlatformConfig.setWeixin(KeyConstants.LP360_WECHAT_APP_ID,
                KeyConstants.LP360_WECHAT_APP_SECRET);
        PlatformConfig.setQQZone(KeyConstants.LP360_QQ_OPEN_APP_ID,
                KeyConstants.LP360_QQ_OPEN_APP_KEY);
        UMShareAPI.get(this);
    }


    private final CDialog.DialogListener mDialogListener = new CDialog.DialogListener() {
        @Override
        public void onCancelListener() {
            switch (mWindowType){
                //取消分享对话框
                case CANCEL:
                    mWindowType = WindowType.PROGRESS;
                    break;
            }
        }

        @Override
        public void onConfrimListener() {
            switch (mWindowType){
                case PROGRESS:
                    break;
                case CANCEL:
                    //取消请求
                    mShareProgressWindow.cancelRequest();
                    mWindowType = WindowType.PROGRESS;
                    mShareCancelWindow.dismiss();
                    mShareProgressWindow.dismiss();
                    hideBlur();
                    break;
            }
        }

        @Override
        public boolean onKeyListener() {
//            hideBlur();
            switch (mWindowType){
                case PROGRESS:
                    showCancelWindow();
                    break;
                case CANCEL:

                    break;
            }
            return true;
        }
    };

    /**
     *显示取消对话框
     */
    public final void showCancelWindow(){
        mWindowType = WindowType.CANCEL;
        hideBlur();
        showBlur();
        mShareCancelWindow.show();
    }

    /**
     * 显示分享进度提示框
     */
    private final void showShareProgress(){
        showBlur();
        mShareProgressWindow.setFilePath(mMediaInfo.filePath);
        mShareProgressWindow.setLastModified(mMediaInfo.lastModified);
        mShareProgressWindow.setShareContent(
                mEtShareContent.getText().toString().trim());
        mShareProgressWindow.show();
    }

    /**
     * 隐藏毛玻璃效果
     */
    public final void hideBlur(){
        mIvBlur.setImageBitmap(null);
        mIvBlur.setVisibility(View.INVISIBLE);
        mLLcontentContainer.setVisibility(View.VISIBLE);
    }

    /**
     * 显示毛玻璃效果
     */
    public final void showBlur(){
        Bitmap bitmap = UtilScreenCapture.getDrawing(this);
        if (bitmap != null) {
            mIvBlur.setVisibility(View.VISIBLE);
            mLLcontentContainer.setVisibility(View.INVISIBLE);
            // 将截屏Bitma放入ImageView
            mIvBlur.setImageBitmap(bitmap);
            // 将ImageView进行高斯模糊【25是最高模糊等级】【0x77000000是蒙上一层颜色，此参数可不填】
            UtilBitmap.blurImageView(this,mIvBlur,10, 0x77000000);
        } else {
            // 获取的Bitmap为null时，用半透明代替
            mIvBlur.setBackgroundColor(0x77000000);
        }
    }

    //初始化
    private final void init(){
        mShareProgressWindow = new ShareProgressWindow(this,mDialogListener);
        mShareCancelWindow   = new ShareCancelWindow(this,mDialogListener);
        final Intent intent  = getIntent();
        // 取出Intent传递过来的
        mShareMedia          = (SHARE_MEDIA) intent.getSerializableExtra(
                SHARE_MEDIA.class.getSimpleName());
        mMediaInfo           = (MediaInfo)intent.
                getSerializableExtra(MediaInfo.class.getSimpleName());
        switch (mShareMedia){
            case WEIXIN:
                mTvback.setText(R.string.share_to_wechat);
                break;
            case WEIXIN_CIRCLE:
                mTvback.setText(R.string.share_to_friends);
                break;
            case QQ:
                mTvback.setText(R.string.share_to_qq);
                break;
            case QZONE:
                mTvback.setText(R.string.share_to_qqzone);
                break;
        }
        // 监听输入字数
        mEtShareContent.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }
            @Override
            public void afterTextChanged(Editable s) {
                mTvLimit.setText(s.length() + "/30");
            }
        });
    }

    /**
     * 分享失败
     */
    public final void shareFailure(){
        hideBlur();
        CToast.showToast(R.string.share_failure);
        mBtnShare.setText(R.string.re_share);
    }

    /**
     * 分享
     * @param shareUrl
     * @param thumbUrl
     */
    public final void share(String shareUrl,String thumbUrl){
        ShareAction shareAction = new ShareAction(this);
        String shareText        = mEtShareContent.getText().toString();
        if (shareText.isEmpty()){
            shareText           = " ";
        }
        shareAction.withText(shareText);
        // 基于URL的分享
        UMImage image = new UMImage(getApplicationContext(),
                thumbUrl);
        UMWeb umWeb = new UMWeb(shareUrl);
        umWeb.setTitle(getString(R.string.share_title_default));
        umWeb.setThumb(image);
        umWeb.setDescription(getString(R.string.share_default_text));
        shareAction.withMedia(umWeb);
        shareAction.setPlatform(mShareMedia).setCallback(umShareListener).share();
        hideBlur();
    }


    /**
     * 启动activity
     * @param activity
     * @param share_media
     * @param mediaInfo
     */
    public static void startActivity(Activity activity,
                                     SHARE_MEDIA share_media, MediaInfo mediaInfo){
        Intent intent = new Intent(activity, ShareActivity.class);
        intent.putExtra(SHARE_MEDIA.class.getSimpleName(), share_media);
        intent.putExtra(MediaInfo.TAG, mediaInfo);
        activity.startActivity(intent);
    }

    @OnClick({R.id.tv_back,R.id.btn_share})
    public void doClick(View v) {
        switch (v.getId()) {
            case R.id.tv_back:
                onBackPressed();
                break;
            case R.id.btn_share:
                mWindowType = WindowType.PROGRESS;
                showShareProgress();
                break;
        }
    }

    @Override
    public void onBackPressed() {
        finish();
    }
}