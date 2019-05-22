package com.uni.common.concurrent;

import java.util.concurrent.Callable;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

/**
 * 程序池管理类
 *
 * @author 陈希然
 */
public class ThreadExecutor {

    private static volatile ThreadExecutor sInstance;

    private ExecutorService mExecutor;
    private ScheduledExecutorService mScheduledExecutor;

    private ThreadExecutor() {
        mExecutor = Executors.newCachedThreadPool();
        mScheduledExecutor = Executors.newScheduledThreadPool(5);
    }

    public static ThreadExecutor getInstance() {
        //　单例：延时加载
        if (sInstance == null) {
            synchronized (ThreadExecutor.class) {
                if (sInstance == null) {
                    sInstance = new ThreadExecutor();
                }
            }
        }

        return sInstance;
    }

    public Executor getExecutor() {
        return mExecutor;
    }

    public void execute(Runnable task) {
        mExecutor.execute(task);
    }

    /**
     * @param task
     * @param initialDelay 首次执行的延迟时间(单位秒)
     * @param period       连续执行之间的周期(单位秒)
     * @return
     */
    public ScheduledFuture<?> schedule(Runnable task, long initialDelay, long period) {
        return mScheduledExecutor.scheduleAtFixedRate(task, initialDelay, period, TimeUnit.SECONDS);
    }

    /**
     * @param task
     * @param delay 延迟执行的时间(单位秒)
     * @return
     */
    public ScheduledFuture<?> schedule(Runnable task, long delay) {
        return mScheduledExecutor.schedule(task, delay, TimeUnit.SECONDS);
    }

    /**
     * 提交一个有返回值的任务用于执行
     *
     * @param callable 任务执行的结果
     * @return
     */
    public Future<?> submit(Callable<?> callable) {
        return mExecutor.submit(callable);
    }

    public void shutdown() {
        mExecutor.shutdown();
        mScheduledExecutor.shutdown();
        sInstance = null;
    }

    public void shutdownNow() {
        mExecutor.shutdownNow();
        mScheduledExecutor.shutdownNow();
        sInstance = null;
    }

}
