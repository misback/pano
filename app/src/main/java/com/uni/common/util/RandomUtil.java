package com.uni.common.util;

/**
 * 获得随机数的工具类
 */
public class RandomUtil {

    /**
     * 获得指定长度的随机数
     *
     * @param len 随机数的长度，限制小于long的范围
     * @return
     */
    public static String getRandStr(int len) {
        String ts = String.format("%s", System.currentTimeMillis());
        String rand = ts.substring(ts.length() - len);
        return rand;

    }


}
