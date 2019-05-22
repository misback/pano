package com.uni.pano.fragment;

import android.app.FragmentManager;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.TextView;

import com.uni.common.util.LanguageUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.R;
import com.uni.pano.activities.SplashActivity;
import com.uni.pano.base.BaseDialogFragment;

import java.util.Locale;

import butterknife.BindView;
import butterknife.OnClick;

public class LanguageDialogFragment extends BaseDialogFragment {
    public final static String TAG = LanguageDialogFragment.class.getSimpleName();

    @BindView(R.id.tv_english_check)
    TextView mTvEnglish;

    @BindView(R.id.tv_chinese_check)
    TextView mTvChinese;

    public static void show(FragmentManager fragmentManager){
        LanguageDialogFragment languageDialogFragment = new LanguageDialogFragment();
        languageDialogFragment.show(fragmentManager, LanguageDialogFragment.TAG);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void initView() {
        setTitle(R.string.language);
        String saveLanguage = PreferenceModel.getString(LanguageUtil.class.getSimpleName(), "");
        if(saveLanguage.equals(Locale.ENGLISH.getLanguage())){
            setDrawableRightChecked(mTvEnglish);
        }else{
            setDrawableRightChecked(mTvChinese);
        }
    }

    private final void setDrawableRightChecked(TextView textView){
        final Drawable drawable = getResources().getDrawable(R.drawable.chx_item_s);
        textView.setCompoundDrawablesWithIntrinsicBounds(null,null,drawable,null);
    }

    private final void setDrawableRightNormal(TextView textView){
        final Drawable drawable = getResources().getDrawable(R.drawable.chx_item_n);
        textView.setCompoundDrawablesWithIntrinsicBounds(null,null,drawable,null);
    }

    @OnClick({R.id.rl_chinese, R.id.rl_english, R.id.tv_back})
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.tv_back:
                dismiss();
                break;
            case R.id.rl_english:{
                setDrawableRightChecked(mTvEnglish);
                setDrawableRightNormal(mTvChinese);
                Configuration config = getResources().getConfiguration();// 获得设置对象
                Resources resources = getResources();// 获得res资源对象
                DisplayMetrics dm = resources.getDisplayMetrics();// 获得屏幕参数：主要是分辨率，像素等。
                config.setLocale(Locale.ENGLISH); //语言
                resources.updateConfiguration(config, dm);
                String language = getResources().getConfiguration().locale.getLanguage();
                String saveLanguage = PreferenceModel.getString(LanguageUtil.class.getSimpleName(), "");
                PreferenceModel.putString(LanguageUtil.class.getSimpleName(), Locale.ENGLISH.getLanguage());
                if (!language.equals(saveLanguage) ){
                    Intent intent = new Intent(this.getActivity(), SplashActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    this.getActivity().finish();
                    this.getActivity().startActivity(intent);

                }
                dismiss();
            }
                break;
            case R.id.rl_chinese:{
                setDrawableRightChecked(mTvChinese);
                setDrawableRightNormal(mTvEnglish);
                Configuration config = getResources().getConfiguration();// 获得设置对象
                Resources resources = getResources();// 获得res资源对象
                DisplayMetrics dm = resources.getDisplayMetrics();// 获得屏幕参数：主要是分辨率，像素等。
                config.setLocale(Locale.CHINESE); //语言
                resources.updateConfiguration(config, dm);
                String language = getResources().getConfiguration().locale.getLanguage();
                String saveLanguage = PreferenceModel.getString(LanguageUtil.class.getSimpleName(), "");
                PreferenceModel.putString(LanguageUtil.class.getSimpleName(), Locale.CHINESE.getLanguage());
                if (!language.equals(saveLanguage) ){
                    Intent intent = new Intent(this.getActivity(), SplashActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    this.getActivity().finish();
                    this.getActivity().startActivity(intent);
                }
                dismiss();
            }
                break;
        }
    }
    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        getDialog().requestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreateView(inflater, container, savedInstanceState);
        viewInject(inflater, container, R.layout.dfm_language);
        return mView;
    }
}
