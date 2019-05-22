package com.uni.pano.widget;


import android.app.Dialog;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.uni.pano.R;

/**
 * 提示对话框, 此对话框只做一个信息提示, 只有一个确定按钮
 *
 * @author 陈希然
 */
public class HintDialog extends RelativeLayout implements OnClickListener {

    private Dialog mDialog;
    private Context mContext;
    private OnDismissListener mOnDismissListener;
    private Button mBtnConfirm;
    private TextView mTvTitle, mTvMessage;

    public HintDialog(Context context) {
        super(context);
        init(context);
    }

    public HintDialog(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public HintDialog(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mTvTitle = (TextView) findViewById(R.id.tv_dialog_title);
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
                mDialog.dismiss();
                if (mOnDismissListener != null) {
                    mOnDismissListener.onDismiss();
                    mOnDismissListener = null;
                }
                break;

        }
    }

    private void init(Context context) {
        // TODO Auto-generated method stub
        View.inflate(context, R.layout.dialog_hint, this);
        mContext = context;
        onFinishInflate();
    }

    public boolean isShowing() {
        return mDialog.isShowing();
    }

    public HintDialog dismiss() {
        mDialog.dismiss();
        return this;
    }

    public HintDialog show() {
        mDialog.show();
        return this;
    }

    public HintDialog setMessage(String message) {
        mTvMessage.setText(message);
        return this;
    }

    public HintDialog setOnDismissListener(OnDismissListener onDismissListener) {
        mOnDismissListener = onDismissListener;
        return this;
    }

    public HintDialog setTitle(String title) {
        mTvTitle.setText(title);
        return this;
    }
    public HintDialog setConfirm(String confirm) {
        mBtnConfirm.setText(confirm);
        return this;
    }

    public interface OnDismissListener {
        void onDismiss();
    }

}
