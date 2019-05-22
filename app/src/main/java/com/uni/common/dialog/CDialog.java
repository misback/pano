package com.uni.common.dialog;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import com.uni.pano.R;
import com.uni.pano.base.BaseDialogFragment;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;

/**
 * @描述：     @公共对话框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public abstract class CDialog {
    private Activity mActivity;
    private Dialog   mDialog;
    private Display display;
    private boolean showVisible = false;


    public CDialog(){

    }

    public CDialog(Activity activity,DialogListener listener,
                   boolean adjustScreen,int style) {
        this.mActivity       = activity;
        this.mDialogListener = listener;
        WindowManager windowManager = (WindowManager) this.mActivity
                .getSystemService(Context.WINDOW_SERVICE);
        display = windowManager.getDefaultDisplay();
        init(adjustScreen,style);
    }



    public abstract int getLayoutId();

    protected void initUI(View view){}

    protected void beforeShow(){

    }

    protected void afterShow(){

    }

    public final void init(boolean adjustScreen,int style) {
        View view = LayoutInflater.from(mActivity).inflate(getLayoutId(), null);
        view.setMinimumWidth(display.getWidth());
        mDialog = new Dialog(mActivity, style);
        mDialog.setContentView(view);
        mDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if (keyCode==KeyEvent.KEYCODE_BACK
                        &&event.getRepeatCount()==0){
                    if(mDialogListener.onKeyListener()){
                        dismiss();
                    }
                    return true;
                } else{
                    return false;
                }
            }
        });
        ButterKnife.bind(this, view);
        setCancelable(true).setCanceledOnTouchOutside(true);
        Window dialogWindow = mDialog.getWindow();
        dialogWindow.setGravity(Gravity.LEFT | Gravity.BOTTOM);
        WindowManager.LayoutParams lp = dialogWindow.getAttributes();
        WindowManager m = mActivity.getWindowManager();
        if(adjustScreen){
            final Display d =     m.getDefaultDisplay();  //为获取屏幕宽、高
            lp.width        = d.getWidth();
            lp.height       = d.getHeight();
            dialogWindow.setAttributes(lp);
        }
        initUI(view);
    }

    public final CDialog setCancelable(boolean cancel) {
        mDialog.setCancelable(cancel);
        mDialog.setOnCancelListener(new DialogInterface.OnCancelListener(){
            @Override
            public void onCancel(DialogInterface dialog) {
                if(showVisible){
                    showVisible = false;
                }
            }
        });

        mDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if(showVisible){
                    showVisible = false;
                }
            }
        });
        return this;
    }

    public final CDialog setCanceledOnTouchOutside(boolean cancel) {
        mDialog.setCanceledOnTouchOutside(cancel);
        return this;
    }

    public final void show() {
        if(!showVisible){
            beforeShow();
            mDialog.show();
            afterShow();
            showVisible = true;
        }
    }

    /**
     * 关闭对话框
     */
    public final void dismiss(){
        if(showVisible){
            mDialog.dismiss();
            showVisible = false;
        }

    }

    /**
     * 取消
     */
    public final void cancel(){
        if(showVisible){
            if(null != mDialogListener){
                mDialogListener.onCancelListener();
            }
            dismiss();

        }
    }

    /**
     * 取消
     */
    public final void confirm(boolean dismiss){
        if(showVisible){
            if(null != mDialogListener){
                mDialogListener.onConfrimListener();
            }

            if(dismiss){
                dismiss();
            }

        }
    }

    public final Activity getActivity(){
        return mActivity;
    }

    public final Dialog getDialog(){
        return mDialog;
    }

    private DialogListener mDialogListener;


    public void setDialogListener(DialogListener mDialogListener) {
        this.mDialogListener = mDialogListener;
    }

    /**
     * 对话框监听器
     */
    public static interface DialogListener{
        public void onCancelListener();
        public void onConfrimListener();
        public boolean onKeyListener();
    }


}
