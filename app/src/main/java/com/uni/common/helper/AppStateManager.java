package com.uni.common.helper;

import com.uni.common.log.CLog;

import java.util.ArrayList;


/**
 * 应用的状态管理对象（单例）
 * <p>
 * 目前：实现了应用切后台时通知监听对象
 */
public class AppStateManager {

    private static final String TAG = AppStateManager.class.getSimpleName();
    private static AppStateManager mInstance;
    private ArrayList<AppStateListener> mAppStateListeners = new ArrayList<AppStateListener>();

    private AppStateManager() {

    }

    /**
     * 获取单例
     *
     * @return
     */
    public static AppStateManager getInstance() {
        if (mInstance == null) {
            synchronized (TAG) {
                if (mInstance == null) {
                    mInstance = new AppStateManager();
                }
            }
        }

        return mInstance;
    }

    /**
     * 添加监听
     *
     * @param listener
     */
    public void addListener(AppStateListener listener) {
        if (listener == null) {
            return;
        }

        synchronized (TAG) {
            if (!mAppStateListeners.contains(listener)) {
                mAppStateListeners.add(listener);
            }
        }
    }

    /**
     * 移除监听
     *
     * @param listener
     */
    public void removeListener(AppStateListener listener) {
        if (listener == null) {
            return;
        }

        synchronized (TAG) {
            mAppStateListeners.remove(listener);
        }
    }

    /**
     * 通知app切入后台
     */
    public void notifyAppToBackstage() {
        CLog.log(TAG, "notifyAppToBackstage");
        synchronized (TAG) {
            for (AppStateListener listener : mAppStateListeners) {
                listener.onAppToBackstage();
            }
        }
    }

    /**
     * 通知app切回前台
     */
    public void notifyAppToForeground() {
        CLog.log(TAG, "notifyAppToForeground");
        synchronized (TAG) {
            for (AppStateListener listener : mAppStateListeners) {
                listener.onAppToForeground();
            }
        }
    }

    /**
     * 应用状态监听
     *
     * @author ls
     */
    public interface AppStateListener {
        /**
         * 程序从前台切到后台的回调
         */
        public void onAppToBackstage();

        /**
         * 程序从后台切回前台的回调
         */
        public void onAppToForeground();

    }

}
