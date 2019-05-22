package com.uni.common.util;

import java.io.FileInputStream;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * MD5操作类
 */
public class Md5Util {

    /**
     * 获取MD5，统一返回大写字母格式
     *
     * @param key
     * @return
     */
    public static String md5(String key) {
        try {
            char[] hex = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
            MessageDigest md;
            md = MessageDigest.getInstance("MD5");
            byte[] buf = key.getBytes();
            md.update(buf, 0, buf.length);
            byte[] bytes = md.digest();
            StringBuilder sb = new StringBuilder(32);
            for (byte b : bytes) {
                sb.append(hex[((b >> 4) & 0xF)]).append(hex[((b >> 0) & 0xF)]);
            }
            key = sb.toString();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        key = key.toUpperCase(); // PS: 统一用大写
        return key;
    }

    public static String fileMd5(String filename) {
        InputStream fis;
        byte[] buffer = new byte[1024];
        int numRead = 0;
        MessageDigest md5;
        try {
            fis = new FileInputStream(filename);
            md5 = MessageDigest.getInstance("MD5");
            while ((numRead = fis.read(buffer)) > 0) {
                md5.update(buffer, 0, numRead);
            }
            fis.close();
            return toHexString(md5.digest());
        } catch (Exception e) {
            System.out.println("error");
            return null;
        }
    }

    private static String toHexString(byte[] b) {
        char[] HEX_DIGITS = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                'A', 'B', 'C', 'D', 'E', 'F'};
        StringBuilder sb = new StringBuilder(b.length * 2);
        for (int i = 0; i < b.length; i++) {
            sb.append(HEX_DIGITS[(b[i] & 0xf0) >>> 4]);
            sb.append(HEX_DIGITS[b[i] & 0x0f]);
        }
        return sb.toString();
    }
}
