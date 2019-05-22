package com.uni.common.util;

import android.content.Context;
import android.content.SharedPreferences;

import com.adobe.internal.xmp.impl.ISO8601Converter;

import java.util.Set;

/**
 * @author 陈希然
 */
public class PreferenceModel {

    private final static String PREFERENCES_FILE_NAME = "shared_preference";

    /**录制15分钟自动停止**/
    private final static String KEY_RECORD_TIME_LIMIT = "key_record_time_limit";

    /**快门声音**/
    private final static String KEY_SHUTTER_SOUND     = "key_shutter_sound";

    /**不在提示**/
    private final static String KEY_VERSION_NOT_PROMPT= "key_version_not_prompt";

    private static PreferenceModel mInstance;

    private Context mContext;
    private SharedPreferences mPreference;
    private SharedPreferences.Editor mEditor;

    private PreferenceModel(Context context) {
        mContext = context;
        mPreference = mContext.getSharedPreferences(PREFERENCES_FILE_NAME, Context.MODE_PRIVATE);
        mEditor = mPreference.edit();
    }

    public static void init(Context context) {
        if (mInstance == null) {
            mInstance = new PreferenceModel(context);
        }
    }


    private static boolean isValidate() {
        if (mInstance == null || null == mInstance.mPreference
                || null == mInstance.mEditor) {
            return false;
        }

        return true;
    }

    /**
     * 清空Preference
     */
    public static void clear() {
        mInstance.mEditor.clear();
        mInstance.mEditor.commit();
    }

    /**
     * Get preference value with key string
     */
    public static String getString(String key, String defValue) {
        if (!isValidate() || key == null)
            return defValue;
        return mInstance.mPreference.getString(key, defValue);

    }

    /**
     * Set preference value with key/value pair
     */
    public static void putString(String key, String value) {
        if (!isValidate() || key == null)
            return;

        mInstance.mEditor.putString(key, value).commit();

    }

    /**
     * Get preference value with key string
     */
    public static Set<String> getStringSet(String key, Set<String> defValue) {
        if (!isValidate() || key == null)
            return defValue;
        return mInstance.mPreference.getStringSet(key, defValue);

    }

    /**
     * Set preference value with key/value pair
     */
    public static void putStringSet(String key, Set<String> value) {
        if (!isValidate() || key == null)
            return;
        mInstance.mEditor.putStringSet(key, value).commit();

    }


    /**
     * Get preference value with key string
     */
    public static int getInt(String key, int defValue) {
        if (!isValidate() || key == null)
            return defValue;
        return mInstance.mPreference.getInt(key, defValue);
    }


    /**
     * Set preference value with key/value pair
     */
    public static void putInt(String key, int value) {
        if (!isValidate() || key == null)
            return;
        mInstance.mEditor.putInt(key, value).commit();

    }

    /**
     * Get preference value with key string
     */
    public static long getLong(String key, long defValue) {
        if (!isValidate() || key == null)
            return defValue;
        return mInstance.mPreference.getLong(key, defValue);

    }

    /**
     * Set preference value with key/value pair
     */
    public static void putLong(String key, long value) {
        if (!isValidate() || key == null)
            return;
        mInstance.mEditor.putLong(key, value).commit();

    }

    /**
     * Get preference value with key string
     */
    public static float getFloat(String key, float defValue) {
        if (!isValidate() || key == null)
            return defValue;
        return mInstance.mPreference.getFloat(key, defValue);

    }

    /**
     * Set preference value with key/value pair
     */
    public static void putFloat(String key, float value) {
        if (!isValidate() || key == null)
            return;
        mInstance.mEditor.putFloat(key, value).commit();

    }


    /**
     * Get preference value with key string
     */
    public static boolean getBoolean(String key, boolean defValue) {
        if (!isValidate() || key == null)
            return defValue;
        return mInstance.mPreference.getBoolean(key, defValue);

    }

    /**
     * Set preference value with key/value pair
     */
    public static void putBoolean(String key, boolean value) {
        if (!isValidate() || key == null)
            return;
        mInstance.mEditor.putBoolean(key, value).commit();
    }

    public static void remove(String key) {
        if (!isValidate() || key == null)
            return;
        mInstance.mEditor.remove(key).commit();
    }

    /**
     * 录像时间限制是否打开
     * @param isOpen
     */
    public  static final void setRecordTimeLimitOpen(boolean isOpen){
        putBoolean(KEY_RECORD_TIME_LIMIT, isOpen);
    }


    /**
     * 录像时间限制是否打开
     */
    public static boolean isRecordTimeLimitOpen() {
       return getBoolean(KEY_RECORD_TIME_LIMIT,true);
    }


    /**
     * 快门声音是否打开
     * @param isOpen
     */
    public  static final void setShutterSoundOpen(boolean isOpen){
        putBoolean(KEY_SHUTTER_SOUND, isOpen);
    }


    /**
     * 快门声音是否打开
     */
    public static boolean isShutterSoundOpen() {
        return getBoolean(KEY_SHUTTER_SOUND,true);
    }

    /**
     * 是否提示升级
     * @param prompt
     */
    public  static final void setVersionNotPrompt(boolean prompt){
        putBoolean(KEY_VERSION_NOT_PROMPT, prompt);
    }


    /**
     * 是否提示升级
     */
    public static boolean isVersionNotPrompt() {
        return getBoolean(KEY_VERSION_NOT_PROMPT,false);
    }

}
