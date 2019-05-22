package com.uni.pano.api;

import com.uni.common.env.Env;
import com.uni.common.util.NetUtil;
import com.uni.common.util.PackageUtil;
import com.uni.pano.BuildConfig;

/**
 * @描述：     @http访问配置类
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */

public class URLConstants {

    /**
     * 客户端操作系统版本
     */
    private static final String CLIENT_OS           = "android";

    /**
     * 测试环境
     */
    public static final String TEST_BASE_URL        ="http://192.168.1.104:8778/api/";

    /**
     * 生产环境
     */
    public static final String PROD_BASE_URL        = "http://api.uniunivr.com/";

    /**
     * 添加分享
     */
    public static final String API_ADD_SHARE        = "share/addShare";

    /**
     * 松果版本更新
     */
    public static final String API_APP_CHECKVERSION = "app/checkVersion";

    /**
     * 检查app是否正常运行
     */
    public static final String API_APP_ISRUNNING    = "app/isRunning";

    /**
     * 获取客户端操作系统
     * @return
     */
    public static final String getClientOs(){
        return CLIENT_OS;
    }

    /**
     * 获取客户端版本
     * @return
     */
    public static final String getClientVersion(){
        return PackageUtil.getVersionName(Env.getContext());
    }

    /**
     * 获取客户端迭代号
     * @return
     */
    public static final String getClientVersionCode(){
        return PackageUtil.getVersionCode(Env.getContext()) + "";
    }

    /**
     * 获取mac地址
     * @return
     */
    public static final String getMac(){
        return NetUtil.getLocalMacAddressFromIp(Env.getContext());
    }

    /**
     * 获取baseUrl
     * @return
     */
    public static final String getBaseUrl(){
        if(BuildConfig.DEBUG){
            return PROD_BASE_URL;
        }else{
            return PROD_BASE_URL;
        }
    }
}
