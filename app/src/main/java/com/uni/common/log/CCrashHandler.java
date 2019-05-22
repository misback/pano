package com.uni.common.log;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.os.Debug;
import android.os.Looper;
import android.os.Process;

import com.uni.pano.widget.CToast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.lang.Thread.UncaughtExceptionHandler;
import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;


/**
 * UncaughtException处理类,当程序发生Uncaught异常的时候,有该类 来接管程序,并记录 发送错误报告.
 */
public class CCrashHandler implements UncaughtExceptionHandler {


    /**
     * Debug Log tag
     */
    public static final String TAG = CCrashHandler.class.getSimpleName();
    private static final String VERSION_NAME = "versionName";
    private static final String VERSION_CODE = "versionCode";
    private static final String STACK_TRACE = "STACK_TRACE";
    /**
     * 错误报告文件的扩展名
     */
    private static final String CRASH_REPORTER_EXTENSION = ".log";
    /**
     * CrashHandler实例
     */
    private static CCrashHandler sInstance;
    /**
     * 系统默认的UncaughtException处理类
     */
    private UncaughtExceptionHandler mDefaultHandler;
    /**
     * 程序的Context对象
     */
    private Context mContext;
    /**
     * 使用Properties来保存设备的信息和错误堆栈信息
     */
    private Properties mDeviceCrashInfo = new Properties();
    /**
     * 错误报告文件的路径
     */
    private String mCrashFilePath;

    /**
     * 保证只有一个CrashHandler实例
     */
    private CCrashHandler() {
    }

    /**
     * 获取CrashHandler实例 ,单例模式
     */
    public static CCrashHandler getInstance() {
        if (sInstance == null) {
            sInstance = new CCrashHandler();
        }
        return sInstance;
    }

    /**
     * 辅助函数：获取当前时间
     *
     * @return
     */
    public static String getMillTimeEx() {
        SimpleDateFormat format = new SimpleDateFormat("HH:mm:ss.SSS");
        return format.format(new Date());
    }

    /**
     * 辅助函数：获取当前时间
     *
     * @return
     */
    public static String getDateTime() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH-mm-ss");
        return format.format(new Date());
    }

    /**
     * 初始化,注册Context对象, 获取系统默认的UncaughtException处理器, 设置该CrashHandler为程序的默认处理器
     *
     * @param ctx
     */
    public void init(Context ctx) {
        mContext = ctx;
        mDefaultHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(this);
    }

    /**
     * 当UncaughtException发生时会转入该函数来处理
     */
    @Override
    public void uncaughtException(Thread thread, Throwable ex) {

        // 手动处理异常
        if ((ex != null) && (CCrashHandlerConfig.CRASH_LOG == true)) {

            // PS：手动处理才会弹出toast和奔溃日志
            handleException(ex);

            // Sleep一会后结束程序
            try {
                Thread.sleep(3000);
            } catch (InterruptedException e) {
                CLog.log(TAG, "Error : " + e.toString());
            }
            Process.killProcess(Process.myPid());
            System.exit(10);

        }
        // 交给系统默认处理
        else {
            mDefaultHandler.uncaughtException(thread, ex);
        }

    }

    /**
     * 自定义错误处理,收集错误信息 发送错误报告等操作均在此完成. 开发者可以根据自己的情况来自定义异常处理逻辑
     */
    private void handleException(Throwable ex) {

        final String msg = ex.getLocalizedMessage();
        // 使用Toast来显示异常信息
        new Thread() {
            @Override
            public void run() {
                Looper.prepare();
                CToast.showToast("程序出错啦:" + msg);
                CLog.log(TAG, "exception :" + msg);
                Looper.loop();
            }
        }.start();

        // 收集设备信息
        collectCrashDeviceInfo(mContext);

        // 保存错误报告文件
        mCrashFilePath = saveCrashInfoToFile(ex);


        //如果是OOM异常，手机内存快照
        //不生成OOM hprof文件，以免过多占用用户sd卡空间
        //collectionDumpHprofDataIfOOM(ex);
    }

    /**
     * 保存错误信息到文件中
     *
     * @param ex
     * @return
     */
    private String saveCrashInfoToFile(Throwable ex) {
        Writer info = new StringWriter();
        PrintWriter printWriter = new PrintWriter(info);
        ex.printStackTrace(printWriter);
        Throwable cause = ex.getCause();
        while (cause != null) {
            cause.printStackTrace(printWriter);
            cause = cause.getCause();
        }
        String result = "\nstack:\n" + info.toString();
        CLog.logError(TAG, result);

        printWriter.close();
        //mDeviceCrashInfo.put(STACK_TRACE, result);
        try {

            String path = CCrashHandlerConfig.getCrashLogDir();
            File pathFile = new File(path);
            if (!pathFile.exists() || !pathFile.isDirectory()) {
                pathFile.mkdirs();
            }
            String fileName = path + "/" + "crash-" + getDateTime() + CRASH_REPORTER_EXTENSION;
            FileOutputStream trace = new FileOutputStream(new File(fileName));
            mDeviceCrashInfo.store(trace, "");
            trace.write(result.getBytes());
            trace.flush();
            trace.close();
            return fileName;
        } catch (Exception e) {
            CLog.log(TAG, "an error occured while writing report file..." + e.toString());
        }
        return null;
    }

    /**
     * 收集程序崩溃的设备信息
     *
     * @param ctx
     */
    public void collectCrashDeviceInfo(Context ctx) {
        try {
            PackageManager pm = ctx.getPackageManager();
            PackageInfo pi = pm.getPackageInfo(ctx.getPackageName(),
                    PackageManager.GET_ACTIVITIES);
            if (pi != null) {
                mDeviceCrashInfo.put(VERSION_NAME,
                        pi.versionName == null ? "not set" : pi.versionName);
                mDeviceCrashInfo.put(VERSION_CODE, pi.versionCode + "");
            }
        } catch (NameNotFoundException e) {
            CLog.log(TAG, "Error while collect package info" + e.toString());
        }
        // 使用反射来收集设备信息.在Build类中包含各种设备信息,
        // 例如: 系统版本号,设备生产商 等帮助调试程序的有用信息
        // 具体信息请参考后面的截图
        Field[] fields = Build.class.getDeclaredFields();
        for (Field field : fields) {
            try {
                field.setAccessible(true);
                mDeviceCrashInfo.put(field.getName(), field.get(null) + "");
                CLog.log(TAG, field.getName() + " : " + field.get(null));
            } catch (Exception e) {
                CLog.log(TAG, "Error while collect crash info" + e);
            }
        }
    }

    /**
     * 如果是OOM错误，则保存崩溃时的内存快照，供分析使用
     *
     * @param ex
     */
    public void collectionDumpHprofDataIfOOM(Throwable ex) {
        // 如果是OOM错误，则保存崩溃时的内存快照，供分析使用
        if (isOOM(ex)) {
            try {
                String path = CCrashHandlerConfig.getCrashLogDir();
                String fileName = path + "crash-" + getDateTime() + ".hprof";
                Debug.dumpHprofData(fileName);
            } catch (IOException e) {
                CLog.log(TAG, "couldn’t dump hprof,  an error occurs while opening or writing files.");
            } catch (UnsupportedOperationException e) {
                CLog.log(TAG, "couldn’t dump hprof,  the VM was built without HPROF support.");
            }
        }
    }

    /**
     * 检测这个抛出对象是否为OOM Error
     *
     * @param throwable
     * @return
     */
    private boolean isOOM(Throwable throwable) {

        if (null != throwable && OutOfMemoryError.class.getName().equals(throwable.getClass().getName())) {
            return true;
        } else {
            Throwable cause = throwable.getCause();
            if (cause != null) {
                return isOOM(cause);
            }
            return false;
        }
    }

}