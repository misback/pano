package com.uni.pano.widget;

import android.os.Environment;
import android.view.Gravity;
import android.widget.Toast;

import com.uni.common.env.Env;
import com.uni.common.util.DipPixelUtil;
import com.uni.pano.R;

/**
 * 通用的Toast
 * PS:原则上我们程序中使用Toast都使用这个定制类
 */
public class CToast {
    public static final int LENGTH_SHORT = Toast.LENGTH_SHORT;
    public static final int LENGTH_LONG  = Toast.LENGTH_LONG;

    private static Toast sToast;

    /**
     * 显示吐司
     * @param resId
     */
    public static final void showToast(int resId){
        if(null == sToast) {
            sToast = Toast.makeText(Env.getContext(),resId, LENGTH_SHORT);
            // PS: 后面的偏移量针对前面的Gravity来设置的
        }else{
            sToast.setText(resId);
        }

        sToast.show();
    }

    /**
     * 显示吐司
     * @param msg
     */
    public static final void showToast(String msg){
        if(null == sToast) {
            sToast = Toast.makeText(Env.getContext(),msg, LENGTH_SHORT);
        }else{
            sToast.setText(msg);
        }
        sToast.show();
    }


}
