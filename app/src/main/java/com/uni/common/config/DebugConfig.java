package com.uni.common.config;

/**
 * 调试开关配置文件
 */
public class DebugConfig {

    /**
     * 调试开关：是否开启调试模式，release版本需要设置为false
     */
    public static final boolean DEBUG_ENABLE = true;

    /**
     * 调试开关：是否使用内网测试环境，release版本需要设置为false
     * PS：非常注意！！！发布的时候一定要设置为false！
     */
    public static final boolean TEST_HOST_ENABLE = true;

}
