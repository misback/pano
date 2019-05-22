package com.uni.pano.utils;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.util.Base64;
import android.util.Log;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * Created by terrence on 17-5-8.
 */

public class KeyHashUtil {

    private static final String TAG = "KeyHashUtil";

    public static String getKeyHash(Activity context) {
        PackageInfo packageInfo;
        String key = null;
        try {
            //getting application package name, as defined in manifest
            String packageName = context.getApplicationContext().getPackageName();
            //Retriving package info
            packageInfo = context.getPackageManager().getPackageInfo(packageName,
                    PackageManager.GET_SIGNATURES);
            Log.e(TAG, "\n\n Package Name = " + context.getApplicationContext().getPackageName());
            for (Signature signature : packageInfo.signatures) {
                MessageDigest md = MessageDigest.getInstance("SHA");
                md.update(signature.toByteArray());
                key = new String(Base64.encode(md.digest(), 0));
                Log.e(TAG, "Key Hash: " + key);
            }
        } catch (PackageManager.NameNotFoundException e1) {
            Log.e(TAG, "Name not found " +  e1.toString());
        }
        catch (NoSuchAlgorithmException e) {
            Log.e(TAG, "No such an algorithm " + e.toString());
        } catch (Exception e) {
            Log.e(TAG, "Exception " + e.toString());
        }
        return key;
    }
}
