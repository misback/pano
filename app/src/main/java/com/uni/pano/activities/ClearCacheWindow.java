package com.uni.pano.activities;

import android.app.Activity;
import android.view.View;
import android.widget.TextView;

import com.uni.common.dialog.CDialog;
import com.uni.pano.R;

import butterknife.BindView;
import butterknife.OnClick;

/**
 * @描述：     @清除缓存对话框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class ClearCacheWindow extends CDialog {

    public ClearCacheWindow(Activity activity, DialogListener listener) {
        super(activity, listener,true,R.style.CDialog_Theme);
    }

    @Override
    public int getLayoutId() {
        return R.layout.view_clear_cache;
    }

    @OnClick({R.id.txt_confirm,R.id.txt_cancel})
    public void onClick(View view){
        switch (view.getId()){
            case R.id.txt_confirm:
                confirm(true);
                break;
            case R.id.txt_cancel:
                cancel();
                break;
        }
    }

}