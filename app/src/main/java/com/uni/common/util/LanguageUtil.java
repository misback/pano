package com.uni.common.util;

import android.content.ContextWrapper;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.DisplayMetrics;

import com.uni.pano.event.LanguageChangeEvent;

import org.greenrobot.eventbus.EventBus;

import java.util.Locale;

/**
 * Created by Chenxiran on 2016/8/18.
 */
public class LanguageUtil {
    public static void changeLanguage(ContextWrapper context, Locale locale) {
        Configuration config = context.getResources().getConfiguration();// 获得设置对象
        Resources resources = context.getResources();// 获得res资源对象
        DisplayMetrics dm = resources.getDisplayMetrics();// 获得屏幕参数：主要是分辨率，像素等。
        config.setLocale(locale); //语言
        resources.updateConfiguration(config, dm);
        PreferenceModel.putString(LanguageUtil.class.getSimpleName(), locale.getLanguage());
    }

    public static String getDisplayLanguage(Locale locale){
        String language = locale.getLanguage();
        if (language.equals(Locale.CHINESE.getLanguage())){
            return locale.getDisplayLanguage();
        }else{
            return locale.ENGLISH.getDisplayLanguage();
        }
    }

}
