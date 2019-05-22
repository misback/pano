/*
 * Copyright (C) 2016 Nico(duyouhua1214@163.com), Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.uni.pano.activities;

import android.os.Bundle;
import android.os.Handler;
import android.widget.ImageView;
import android.widget.TextView;

import com.uni.common.util.LanguageUtil;
import com.uni.common.util.PackageUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.R;
import com.uni.pano.base.BaseActivity;

import java.util.Locale;
import com.uni.pano.utils.CommonUtil;
import com.uni.common.config.PathConfig;
import butterknife.BindView;


public class SplashActivity extends BaseActivity {

    private final String TAG = "SplashActivity";

    private Handler mHandler = new Handler();

    @BindView(R.id.tv_app_version)
    TextView mTvAppVersion;

    @BindView(R.id.iv_logo)
    ImageView mIvLogo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.av_splash);
        initData();
        CommonUtil.copy(this, "seeta_fa_v1.1.bin", PathConfig.getFaceDir(), "seeta_fa_v1.1.bin");
        CommonUtil.copy(this, "seeta_fd_frontal_v1.0.bin", PathConfig.getFaceDir(), "seeta_fd_frontal_v1.0.bin");
        gotoMainDelayed();
    }
    private void gotoMainDelayed() {
        mHandler.postDelayed(new Runnable() {
            public void run() {
                CameraActivity.startActivity(SplashActivity.this);
                 finish();
            }
        }, 500);
    }

    private final void initData(){
        mTvAppVersion.setText(String.format(getResources().getString(R.string.splash_version),
                PackageUtil.getVersionName(this)));
//        String saveLanguage = PreferenceModel.getString(LanguageUtil.class.getSimpleName(), "");
//        if(saveLanguage.equals(Locale.ENGLISH.getLanguage())){
//            mIvLogo.setImageResource(R.drawable.ic_logo_en);
//        }else{
//            mIvLogo.setImageResource(R.drawable.ic_logo);
//        }
        mIvLogo.setImageResource(R.drawable.ic_logo_lp360);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}