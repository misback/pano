package com.uni.common.log;

import com.uni.common.config.DebugConfig;
import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;

/**
 * 配置JDLog的相关路径
 */
public class CLogConfig {

    /**
     * 控制是否打开日志
     * PS：发布产品的时候把PRINT_LOG设置成false
     * PS：也可以在混淆的时候，设置该值
     */
    public static final boolean PRINT_LOG = DebugConfig.DEBUG_ENABLE;


    /**
     * 日志保存路径
     */
    private static final String LOG_DIR = PathConfig.getCacheDir() + "/log";

    /**
     * 获取保存日志的位置
     *
     * @return
     */
    public static String getLogDir() {
        String dir = LOG_DIR;
        FileUtil.ensureDir(dir);
        return dir;
    }

}
