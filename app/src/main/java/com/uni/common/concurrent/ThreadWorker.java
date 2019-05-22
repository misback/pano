/**
 * TODO
 * Cloudva SDK--ThreadWorker.java
 * Copyright YunVa
 *
 * @2012-12-12
 */
package com.uni.common.concurrent;


import android.os.Looper;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

/**
 * 线程工作者
 *
 * @author 陈希然
 */
public class ThreadWorker implements Runnable {

    //    private byte[] mLock = new byte[0];
    private Looper mLooper;
    private Lock mLock;
    private Condition mLockCondition;


    public ThreadWorker() {
//        synchronized (mLock) {
//            try {
//                while (mLooper == null) {
//                    mLock.wait();//等待looper的创建
//                }
//            } catch (Exception e) {
//                e.printStackTrace();
//            }
//        }

        mLock = new ReentrantLock();
        mLockCondition = mLock.newCondition();
//    	ThreadExecutor.getInstance().execute(this);

        Thread thread = new Thread(this);
        thread.setPriority(Thread.NORM_PRIORITY);
        thread.start();

        mLock.lock();
        try {
            while (mLooper == null) {
                mLockCondition.await();//等待looper的创建
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            mLock.unlock();
        }
    }


    @Override
    public void run() {

//        synchronized (mLock) {
//            if (mLooper == null) {
//                Looper.prepare();
//                mLooper = Looper.myLooper();
//                mLock.notifyAll();
//            }
//        }
//        Looper.loop();

        mLock.lock();
        try {
            if (mLooper == null) {
                Looper.prepare();
                mLooper = Looper.myLooper();
                mLockCondition.signal();
            }
        } finally {
            mLock.unlock();
        }

        Looper.loop();
    }

    public void quit() {
        if (mLooper != null) {
            mLooper.quit();
        }
//        mLock = null;
    }

    public Looper getLooper() {
        return mLooper;
    }

}
