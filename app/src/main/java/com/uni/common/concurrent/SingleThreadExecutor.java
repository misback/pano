package com.uni.common.concurrent;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

/**
 * 程序池管理类
 *
 * @author 陈希然
 */
public class SingleThreadExecutor {

    private static SingleThreadExecutor mInstance;
    private static Lock mLock = new ReentrantLock();
    //	private static byte[] mLock = new byte[0];
    private ExecutorService mExecutor;

    private SingleThreadExecutor() {
        mExecutor = Executors.newSingleThreadExecutor();
    }

    public static SingleThreadExecutor getInstance() {
        if (mInstance == null) {
//        	mLock = new ReentrantLock();
            mLock.lock();
//    		synchronized (mLock) {
            try {
                if (mInstance == null) {
                    mInstance = new SingleThreadExecutor();
                }
            } finally {
                mLock.unlock();
            }
//			}
        }

        return mInstance;
    }

    public void execute(Runnable task) {
        mExecutor.execute(task);
    }

    public void shutdown() {
        mExecutor.shutdown();
        mInstance = null;
    }

    public void shutdownNow() {
        mExecutor.shutdownNow();
        mInstance = null;
    }
}
