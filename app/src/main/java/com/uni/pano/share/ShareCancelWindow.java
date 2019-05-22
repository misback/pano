package com.uni.pano.share;

import android.app.Activity;
import android.view.View;

import com.uni.common.dialog.CDialog;
import com.uni.pano.R;

import butterknife.OnClick;

/**
 * @描述：     @分享取消弹出框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */

public class ShareCancelWindow extends CDialog {

    public ShareCancelWindow(Activity activity, DialogListener listener) {
        super(activity,listener,true,R.style.CDialog_Theme);
    }

    @Override
    public int getLayoutId() {
        return R.layout.view_share_cancel;
    }


    @OnClick({R.id.txt_cancel,R.id.txt_confirm})
    public void onClick(View view){
        switch (view.getId()){
            case R.id.txt_cancel:
                cancel();
                break;
            case R.id.txt_confirm:
                confirm(false);
                break;
        }
    }

}
