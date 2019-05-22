package com.uni.common.log;

import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;

/**
 * 设置CrashHandler的相关目录
 */
public class CCrashHandlerConfig {

    /**
     * 是否处理奔溃
     */
    public static final boolean CRASH_LOG = true; // PRINT_LOG;
    /**
     * 崩溃日志保存路径
     */
    private static final String CRASH_LOG_DIR = PathConfig.getCacheDir() + "/crash";

    /**
     * 获取保存日志的位置
     *
     * @return
     */
    public static String getCrashLogDir() {
        String dir = CRASH_LOG_DIR;
        FileUtil.ensureDir(dir);
        return dir;
    }
}
