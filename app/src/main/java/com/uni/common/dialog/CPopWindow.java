package com.uni.common.dialog;

import android.app.Activity;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.PopupWindow;

/**
 * @描述：     @公共底部弹出框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public  abstract class CPopWindow {

    private PopupWindow mPopupWindow;
    private Activity   mActivity;
    private View       mParent;
    private int        style;

    public CPopWindow(){

    }

    public CPopWindow(Activity activity,View parent,int style){
        this.mActivity       = activity;
        this.mParent         = parent;
        init(style);
    }

    public abstract int getLayoutId();

    private final void init(int style){
        View popupWindowView = mActivity.getLayoutInflater().inflate(getLayoutId(), null);
        mPopupWindow = new PopupWindow(popupWindowView,
        ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, true);
        mPopupWindow.setAnimationStyle(style);
        ColorDrawable dw = new ColorDrawable(0xffffffff);
        mPopupWindow.setBackgroundDrawable(dw);
        backgroundAlpha(0.5f);
        mPopupWindow.setOnDismissListener(new PopupDismissListener());
        popupWindowView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                dismiss();
                return false;
            }
        });
    }

    /**
     * 显示弹出框
     */
    public final void show(){
        mPopupWindow.showAtLocation(this.mParent,
                Gravity.BOTTOM|Gravity.CENTER_HORIZONTAL, 0, 0);
    }

    /**
     * 关闭弹出框
     */
    public final void dismiss(){
        mPopupWindow.dismiss();
    }

    public final void backgroundAlpha(float bgAlpha) {
        WindowManager.LayoutParams lp = mActivity.getWindow().getAttributes();
        lp.alpha = bgAlpha; //0.0-1.0
        mActivity.getWindow().setAttributes(lp);
    }


    public  class PopupDismissListener implements PopupWindow.OnDismissListener{

        @Override
        public void onDismiss() {
            backgroundAlpha(1f);
        }

    }

}
