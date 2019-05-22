package com.uni.pano.share;


import android.content.Context;
import android.os.Handler;

import com.uni.pano.R;

/**
 * 转菊花的等待框-setProHintStr设置菊花下方的提示字符串
 */
public class CWaitingDialog extends CBaseDialog {

    public static final String TAG = CWaitingDialog.class.getSimpleName();
    private static CWaitingDialog sDialog = null;
    public static DismissListener mDismissListener;
    /**
     * 滞后隐藏对话框的handler
     */
    private static int MSG_HIDE_DIALOG = 0;
    private static Handler sHideHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            if (msg.what == MSG_HIDE_DIALOG) {
                if ((sDialog != null) && (sDialog.isShowing())) {
                    sDialog.dismiss();
                    sDialog = null;
                    if(mDismissListener != null) {
                        mDismissListener.onDismiss();
                        mDismissListener = null;
                    }
                }
            }
        }

        ;
    };

    // ----------------------- 针对简单调用 ------------------------------

    public CWaitingDialog(Context context) {
        super(context, R.style.waiting_dialog);
        setContentView(R.layout.c_dialog_waiting);
    }

    /**
     * 简单的函数：直接显示对话框
     *
     * @param context
     */
    public static void showDialog(Context context) {
        showDialog(context, true);
    }

    private static void showDialog(Context context, boolean cancelAble) {
        // 如果正在显示，则不处理
        if ((sDialog != null) && (sDialog.isShowing())) {
            sHideHandler.removeMessages(MSG_HIDE_DIALOG); // 清空隐藏对话框的滞后消息，避免刚刚显示，则被之前的消息隐藏了
            return;
        }

        // 否则，重新创建显示
        sDialog = new CWaitingDialog(context);
        sDialog.show();

        if (!cancelAble) {
            sDialog.setCancelable(false);
            sDialog.setCanceledOnTouchOutside(false);
        }
    }

    /**
     * 显示不可点击取消的等待对话框
     *
     * @param context
     */
    public static void showDialogUncancel(Context context) {
        showDialog(context, false);
    }

    /**
     * 简单的函数：直接隐藏之前的对话框
     */
    public static void hideDialog() {
        sHideHandler.sendEmptyMessageDelayed(MSG_HIDE_DIALOG, 0); // 滞后隐藏，避免显示后立马隐藏太快，看不到菊花的过程
    }

    public interface DismissListener{
        void onDismiss();
    }
}
