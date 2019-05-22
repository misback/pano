package com.uni.pano.fragment;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.widget.SwitchCompat;
import android.text.TextUtils;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.facebook.FacebookException;
import com.facebook.login.LoginResult;
import com.uni.common.dialog.CDialog;
import com.uni.common.glide.GlidCacheManage;
import com.uni.common.util.LanguageUtil;
import com.uni.common.util.PackageUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.activities.BlurListener;
import com.uni.pano.activities.ClearCacheWindow;
import com.uni.pano.activities.PandaActivity;
import com.uni.pano.activities.UpdateWindow;
import com.uni.pano.activities.WindowType;
import com.uni.pano.base.BaseFragment;
import com.uni.pano.share.FBHelper;
import com.uni.pano.widget.CToast;

import java.util.Locale;

import butterknife.BindView;
import butterknife.OnClick;


/**
 * @描述：     @设置界面
 * @作者：     @蒋诗朋
 * @创建时间： @2017-05-10
 */
public class SettingFragment extends BaseFragment {
    public final static String TAG = SettingFragment.class.getSimpleName();
    @BindView(R.id.tv_language)
    TextView tv_language;

    @BindView(R.id.tv_app_version)
    TextView mTvAppVersion;

    @BindView(R.id.tv_firmware_version)
    TextView mTvFirmwareVersion;

    @BindView(R.id.tv_fb_user_name)
    TextView mTvFBUserName;

    @BindView(R.id.tv_cache)
    TextView mTvCache;

    @BindView(R.id.rl_clear_cache)
    RelativeLayout mLLClearCache;

    @BindView(R.id.rl_check_update)
    RelativeLayout mRLCheckUpdate;

    @BindView(R.id.sc_shutter_sound)
    SwitchCompat mScShutterSound;

    @BindView(R.id.sc_record_time_limit)
    SwitchCompat mScRecordTimeLimit;

    private FBHelper mFBHelper;

    private ClearCacheWindow mCacheWindow;

    private UpdateWindow  mUpdateWindow;

    private WindowType mType = WindowType.CLEAR;

    @Override
    protected void initUI(@Nullable Bundle savedInstanceState) {
        setContentView(R.layout.fm_setting);
        init();
        initWindow();
    }

    private final void init(){
        String language     = getResources().getConfiguration().locale.getLanguage();
        String saveLanguage = PreferenceModel.getString(
                LanguageUtil.class.getSimpleName(), language);
        tv_language.setText(LanguageUtil.getDisplayLanguage(new Locale(saveLanguage)));
        String size         = GlidCacheManage.getInstance().getCacheSize();
        if(TextUtils.isEmpty(size)){
            size            = ResourceUtil.getString(R.string.zero_byte);
        }
        mTvCache.setText(size);
        mScRecordTimeLimit.setChecked(PreferenceModel.isRecordTimeLimitOpen());
        mScShutterSound.setChecked(PreferenceModel.isShutterSoundOpen());
        mScShutterSound.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                setShutterSound();
            }
        });
        mScRecordTimeLimit.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                setRecordTimeLimit();
            }
        });
    }

    private final void initWindow(){
        mCacheWindow        = new ClearCacheWindow(getActivity(),mDialogListener);
        mUpdateWindow       = new UpdateWindow(getActivity(),
                UpdateWindow.FROM_SETTING_FRAGMENT,
                mDialogListener,new BlurListener() {
            @Override
            public void showBlur() {
                SettingFragment.this.showBlur();
            }

            @Override
            public void hideBlur() {
                SettingFragment.this.hideBlur();
            }
        });
    }

    private final CDialog.DialogListener mDialogListener = new CDialog.DialogListener() {
        @Override
        public void onCancelListener() {
            hideBlur();
        }

        @Override
        public void onConfrimListener() {
            hideBlur();
            switch (mType){
                case CLEAR:
                    GlidCacheManage.getInstance().cleanCacheDisk();
                    GlidCacheManage.getInstance().clearCacheMemory();
                    getActivity().runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            CToast.showToast(R.string.clear_success);
                        }
                    });
                    break;
                case UPDATE:
                    break;
            }
        }
        @Override
        public boolean onKeyListener() {
            hideBlur();
            return true;
        }
    };


    private final void showBlur(){
        final PandaActivity activity = (PandaActivity) getActivity();
        activity.showBlur();
    }

    private final void hideBlur(){
        final PandaActivity activity = (PandaActivity) getActivity();
        activity.hideBlur();
    }

    public final void setFBHelper(FBHelper helper){
        this.mFBHelper = helper;
        initData();
    }

    //初始化数据
    private final void initData(){
        mTvAppVersion.setText(PackageUtil.getVersionName(this.getActivity()));
        mTvFirmwareVersion.setText(PackageUtil.getVersionName(this.getActivity()));
        if(mFBHelper.isLoggedIn()){
            final String userName = mFBHelper.getUserName();
            if(!TextUtils.isEmpty(userName)){
                mTvFBUserName.setText(mFBHelper.getUserName());
            }
        }
    }


    @OnClick({R.id.rl_language,
            R.id.rl_logo,
            R.id.rl_facebook,
            R.id.rl_clear_cache,
            R.id.rl_check_update,
            R.id.rl_record_time_limit,
            R.id.rl_shutter_sound})
    public void onAlbumAndFilterClick(View view) {
        switch (view.getId()) {
            case R.id.rl_language:
                LanguageDialogFragment.show(getFragmentManager());
                break;
            case R.id.rl_logo:
                LogoDialogFragment.show(getFragmentManager());
                break;
            case R.id.rl_clear_cache:
                if(null != mCacheWindow){
                    mType = WindowType.CLEAR;
                    showBlur();
                    mCacheWindow.show();
                }
                break;
            case R.id.rl_check_update:
                mType = WindowType.UPDATE;
                mUpdateWindow.checkVersion();
                break;
            case R.id.rl_facebook:
                if(!mFBHelper.isLoggedIn()){
                    mFBHelper.loginFacebook(new FBHelper.FBLoginCallback() {
                        @Override
                        public void onSuccess(LoginResult loginResult) {
                            getActivity().runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    mTvFBUserName.setText(mFBHelper.getUserName());
                                }
                            });
                        }
                        @Override
                        public void onError(FacebookException error) {

                        }
                    });
                }
                break;
            case R.id.rl_record_time_limit:
                setRecordTimeLimit();
                break;
            case R.id.rl_shutter_sound:
                setShutterSound();
                break;
        }
    }

    private final void setRecordTimeLimit(){
        if(PreferenceModel.isRecordTimeLimitOpen()){
            mScRecordTimeLimit.setChecked(false);
            PreferenceModel.setRecordTimeLimitOpen(false);
        }else{
            mScRecordTimeLimit.setChecked(true);
            PreferenceModel.setRecordTimeLimitOpen(true);
        }
    }

    private final void setShutterSound(){
        if(PreferenceModel.isShutterSoundOpen()){
            mScShutterSound.setChecked(false);
            PreferenceModel.setShutterSoundOpen(false);
        }else{
            mScShutterSound.setChecked(true);
            PreferenceModel.setShutterSoundOpen(true);
        }
    }

}
