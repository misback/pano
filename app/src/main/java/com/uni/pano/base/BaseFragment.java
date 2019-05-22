package com.uni.pano.base;

import android.app.Fragment;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import butterknife.ButterKnife;

/**
 * BaseFragment
 * <p>
 * Created by ZachLi on 2016/7/15.
 */
public abstract class BaseFragment extends Fragment {

    /**
     * Fragment Tag
     */
    protected final String TAG = this.getClass().getSimpleName();

    private boolean isVisiabled;
    private View mContentView;

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        // 用户可见与不可见的处理
        // 通常用于数据的懒加载
        if (!isVisiabled && isVisibleToUser) {
            isVisiabled = true;
            onUserVisible();
        } else {
            onUserInvisible();
        }

    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        // 避免重复加载布局
        if (mContentView == null) {
            initUI(savedInstanceState);
        } else {
            ViewGroup parent = (ViewGroup) mContentView.getParent();
            if (parent != null) {
                parent.removeView(mContentView);
            }
        }
        return mContentView;
    }

    /**
     * 使用一个明确的view设置Fragment内容
     *
     * @param view
     */
    protected void setContentView(View view) {
        this.mContentView = view;
        //　绑定解析注入框架
        ButterKnife.bind(this, mContentView);
    }

    /**
     * 使用布局资源设置Fragment内容
     *
     * @param layoutResId
     */
    protected void setContentView(@LayoutRes int layoutResId) {

        this.mContentView = LayoutInflater.from(this.getActivity()).inflate(layoutResId, null);
        //　绑定解析注入框架
        ButterKnife.bind(this, mContentView);
    }

    /**
     * 查找具有给定id的子view
     *
     * @param id 用来查找view的id
     * @return 具有给定id的view, 如果不包含指定id则返回 null
     */
    public View findViewById(@IdRes int id) {

        return mContentView == null ? null : mContentView.findViewById(id);
    }


    /**
     * 初始化UI
     *
     * @param savedInstanceState
     */
    protected abstract void initUI(@Nullable Bundle savedInstanceState);


    /**
     * 当Fragment对用户可见的时候回调
     */
    public void onUserVisible() {

    }

    /**
     * 当Fragemnt对用户不可见的时候回调
     */
    public void onUserInvisible() {

    }
}
