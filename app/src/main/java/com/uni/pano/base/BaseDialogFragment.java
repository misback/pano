package com.uni.pano.base;

import android.app.DialogFragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import com.uni.pano.R;

import butterknife.BindView;
import butterknife.ButterKnife;

public abstract class BaseDialogFragment extends DialogFragment {

    protected View mView;

    private boolean isFullScreeen = true;

    @BindView(R.id.tv_back)
    TextView mTvBack;

    protected View viewInject(LayoutInflater inflater, ViewGroup container, int resId) {
        mView = inflater.inflate(resId, container, false);
        ButterKnife.bind(this, mView);
        if (null != mView) {
            initView();
        }
        return mView;
    }

    public final void setTitle(int resId){
        mTvBack.setText(resId);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        getDialog().requestWindowFeature(Window.FEATURE_NO_TITLE);
        setFullScreen(true);

        //getDialog().getWindow().getAttributes().windowAnimations = R.style.customDialogAnim;
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (isFullScreeen) {
            setStyle(DialogFragment.STYLE_NORMAL, android.R.style.Theme_Light_NoTitleBar_Fullscreen);
        }
    }

    protected void showDialogFragment(BaseDialogFragment dialogFragment) {
        dialogFragment.show(getFragmentManager(), dialogFragment.getClass().getName());
    }

    protected void showDialogFragment(BaseDialogFragment dialogFragment, boolean isFullScreen) {
        dialogFragment.setFullScreen(isFullScreen);
        showDialogFragment(dialogFragment);
    }

    protected void showDialogFragment(Class clas, Bundle bundle) {
        FragmentManager fmm = getFragmentManager();
        BaseDialogFragment bfm = getFragment(fmm, clas);

        if (null != bfm && !bfm.isVisible()) {
            bfm.setArguments(bundle);
            bfm.show(fmm, bfm.getClass().getName());
        }
    }

    protected void showDialogFragment(Class clas, boolean isFullScreen) {
        FragmentManager fmm = getFragmentManager();
        BaseDialogFragment bfm = getFragment(fmm, clas);

        if (null != bfm && !bfm.isVisible() && !bfm.isAdded()) {
            bfm.setFullScreen(isFullScreen);
            bfm.show(fmm, bfm.getClass().getName());
        }
    }

    protected void showDialogFragment(Class clas, Bundle bundle, boolean isFullScreen) {
        FragmentManager fmm = getFragmentManager();
        BaseDialogFragment bfm = getFragment(fmm, clas);

        if (null != bfm && !bfm.isVisible()) {
            bfm.setArguments(bundle);
            bfm.setFullScreen(isFullScreen);
            bfm.show(fmm, bfm.getClass().getName());
        }
    }

    protected void showDialogFragment(Class clas) {
        FragmentManager fmm = getFragmentManager();
        BaseDialogFragment bfm = getFragment(fmm, clas);

        if (null != bfm && !bfm.isVisible()) {
            bfm.show(fmm, bfm.getClass().getName());
        }
    }

    /**
     * 得到一个Fragment实例
     *
     * @param clas
     * @return
     */
    public BaseDialogFragment getFragment(FragmentManager fm, Class clas) {
        if (null == fm || clas == null) {
            return null;
        }

        BaseDialogFragment fragment = (BaseDialogFragment) fm.findFragmentByTag(clas.getName());
        if (fragment == null) {

            try {
                fragment = (BaseDialogFragment) Class.forName(clas.getName())
                        .newInstance();// 通过反射获取到当前类实力
            } catch (java.lang.InstantiationException e) {
                e.printStackTrace();
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
            }
        }

        return fragment;
    }

    /**
     * 返回上一个fragment
     *
     * @param fm
     */
    public void backPopStack(FragmentManager fm) {
        if (fm == null)
            getFragmentManager().popBackStack();
        else
            fm.popBackStack();
    }

    public void dismissCurFm() {
        FragmentManager fmm = getFragmentManager();
        FragmentTransaction transaction = fmm.beginTransaction();
        transaction.remove(this);
        transaction.commitAllowingStateLoss();
    }

    public void setFullScreen(boolean isFull) {
        this.isFullScreeen = isFull;
    }

    protected void setContentView(LayoutInflater inflater, ViewGroup container, int id) {
        if (mView == null) {
            mView = inflater.inflate(id, container, false);
            initView();
        } else {
            ViewGroup parent = (ViewGroup) mView.getParent();
            if (parent != null)
                parent.removeView(mView);
        }
    }

    protected abstract void initView();
}
