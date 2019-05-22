package com.uni.common.config;

/**
 * 请求域名配置
 */
public class HostConfig {

    /**
     * API请求域名
     */
    private static final String API_HOST = "http://api.idooboo.net";

    /**
     * API请求域名（内网测试环境）
     */
    private static final String API_HOST_TEST = "http://api.idooboo.net"; // 未配置

    /**
     * 获得API请求域名
     *
     * @return
     */
    public static String getApiHost() {

        if (!DebugConfig.TEST_HOST_ENABLE) {
            return API_HOST;
        } else {
            return API_HOST_TEST;
        }
    }

}
