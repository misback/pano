package com.uni.pano.widget;

import android.app.Dialog;
import android.content.Context;
import android.support.annotation.StringRes;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.uni.pano.R;

public class ConfirmDialog extends RelativeLayout implements OnClickListener {

    private Context mContext;
    private OnConfirmDialogMenuClickListener mOnConfirmDialogMenuClickListener;
    private Button mBtnCancel;
    private Button mBtnConfirm;
    private Dialog mDialog;
    private TextView mTvMessage;

    public ConfirmDialog(Context context) {
        super(context);
        init(context);
    }

    public ConfirmDialog(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public ConfirmDialog(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mBtnCancel = (Button) findViewById(R.id.btn_cancel);
        mBtnCancel.setOnClickListener(this);
        mBtnConfirm = (Button) findViewById(R.id.btn_confirm);
        mBtnConfirm.setOnClickListener(this);
        mTvMessage = (TextView) findViewById(R.id.tv_message);
        mDialog = new Dialog(mContext, R.style.DialogTheme);
        mDialog.setContentView(this);
        mDialog.setCanceledOnTouchOutside(false);
        mDialog.setCancelable(false);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_confirm:
                if (mOnConfirmDialogMenuClickListener != null) {
                    mOnConfirmDialogMenuClickListener.onConfirmDialogMenuClick(OnConfirmDialogMenuClickListener.MENU_CONFIRM);
                }
                break;
            case R.id.btn_cancel:
                if (mOnConfirmDialogMenuClickListener != null) {
                    mOnConfirmDialogMenuClickListener.onConfirmDialogMenuClick(OnConfirmDialogMenuClickListener.MENU_CANCEL);
                }
                break;

        }
        dismiss();
    }

    private void init(Context context) {
        // TODO Auto-generated method stub
        View.inflate(context, R.layout.confirm_dialog, this);
        mContext = context;
        onFinishInflate();
    }

    public boolean isShowing() {
        return mDialog.isShowing();
    }

    public ConfirmDialog dismiss() {
        mDialog.dismiss();
        return this;
    }

    public ConfirmDialog show() {
        mDialog.show();
        return this;
    }

    /**
     * 设置取消按钮的文字
     *
     * @param text
     * @return
     */
    public ConfirmDialog setCancelText(String text) {
        mBtnCancel.setText(text);
        return this;
    }

    /**
     * 设置确认按钮的文字
     *
     * @param text
     * @return
     */
    public ConfirmDialog setConfirmText(String text) {
        mBtnConfirm.setText(text);
        return this;
    }

    /**
     * 设置提示信息
     *
     * @param message
     * @return
     */
    public ConfirmDialog setMessage(String message) {
        mTvMessage.setText(message);
        return this;
    }

    /**
     * 设置提示信息
     *
     * @param message
     * @return
     */
    public ConfirmDialog setMessage(@StringRes int message) {
        mTvMessage.setText(message);
        return this;
    }

    public ConfirmDialog setOnConfirmDialogMenuClickListener(OnConfirmDialogMenuClickListener onConfirmDialogMenuClickListener) {
        mOnConfirmDialogMenuClickListener = onConfirmDialogMenuClickListener;
        return this;
    }

    public interface OnConfirmDialogMenuClickListener {
        /**
         * 取消按钮菜单id
         */
        public static final int MENU_CANCEL = 0;
        /**
         * 确认按钮菜单id
         */
        public static final int MENU_CONFIRM = 1;

        /**
         * 对话框按钮菜单事件回调
         *
         * @param menuId 按钮菜单id。取值为{@link #MENU_CANCEL}、{@link #MENU_CONFIRM}
         */
        void onConfirmDialogMenuClick(int menuId);
    }

}
