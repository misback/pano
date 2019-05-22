package com.uni.common.util;

import android.text.TextUtils;

/**
 * 用来处理一些字符相关的工作
 */
public class CharUtil {

    /**
     * 计算字符个数，两个英文字符算一个中文字符
     *
     * @param text
     * @return
     */
    public static int calculateCharNum(String text) {
        int count = 0;

        if (text == null) {
            return count;
        }

        char[] array = text.toCharArray();
        int chineseCount = 0;
        int englishCount = 0;
        for (int i = 0; i < array.length; i++) {
            if ((char) (byte) array[i] != array[i]) {
                chineseCount++;
            } else {
                englishCount++;
            }
        }

        if (englishCount % 2 == 0) {
            count = chineseCount + englishCount / 2;
        } else {
            count = chineseCount + englishCount / 2 + 1;
        }

        return count;
    }

    /**
     * 获取字符串的hang数
     *
     * @param text
     * @return
     */
    public static int calulateCharRow(String text) {
        if (text == null) {
            return 0;
        }
        int rowNum = 0;
        int len = text.length();
        for (int i = 0; i < len; i++) {
            if (text.charAt(i) == '\n') {
                rowNum++;
            }
        }
        return rowNum + 1;
    }

    /**
     * 判断输入是否是a、B之类的字母
     *
     * @param c
     * @return
     */
    public static boolean isLetter(char c) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 获取首字母
     *
     * @param name
     * @return
     */
    public static String getHeadChar(String name) {
        if (TextUtils.isEmpty(name)) {
            return "#unknow";
        }

        String letter = CharacterParser.getInstance().getSelling(name);
        if (TextUtils.isEmpty(letter)) {
            return "#unknow";
        }

        return letter.toUpperCase().substring(0, 1);
    }
}
