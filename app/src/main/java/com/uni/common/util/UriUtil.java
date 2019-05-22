package com.uni.common.util;

import android.app.Activity;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;

/**
 * UriUtil工具类
 */
public class UriUtil {

    /**
     * 根据Uri获取绝对路径
     *
     * @param activity
     * @param uri
     * @return
     */
    public static String getAbsoluteImagePath(Activity activity, Uri uri) {
        String[] proj = {
                MediaStore.Images.Media.DATA
        };
        Cursor cursor = activity.managedQuery(uri, proj, null, null, null);
        if (cursor == null) {
            return "";
        } else {
            int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
            cursor.moveToFirst();
            return cursor.getString(column_index);
        }
    }
}
