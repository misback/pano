package com.uni.pano;
import android.app.Activity;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.util.Log;

import com.facebook.FacebookSdk;
import com.tencent.bugly.crashreport.CrashReport;
import com.uni.common.env.Env;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.config.EnumElement;
import com.uni.pano.config.KeyConstants;
import com.uni.pano.event.CameraConnectEvent;
import com.uni.pano.utils.CameraUtil;
import com.uni.pano.utils.CommonUtil;

import org.greenrobot.eventbus.EventBus;

public class MainApplication extends Application {
    private static final String TAG = Application.class.getSimpleName();
    public static final int SD_AVAILABLE_SIZE = 500;//M
    public static final int VENDOR_ID = 10750;
    //public static final int PRODUCT_ID = 19795;
    public static final int PRODUCT_ID = 45069;
    private UsbManager mUsbManager;
    private String firmInfoDb = "firmInfoDb";
    private String firmInfoSn = "firmInfo_sn";
    private String firmInfoVersion = "firmInfo_version";
    private SharedPreferences mSharedPreferences;
    public static boolean bSensorRotate = false;
    public static boolean bSupportMediaCodecInterface = false;

    //检查相机是否连接
    private boolean connectionCamera    = false;

    private static MainApplication instance;
    static {
        System.loadLibrary("unipano");
    }
    public static EnumElement.RENDER_MODE render_mode = EnumElement.RENDER_MODE.SINGLE;
    public boolean mCameraConnectStatus =   false;
    public void updateRotationByConnectStatus(Activity activity){
        CommonUtil.openRotateScreenInSetting(this);
        if (mCameraConnectStatus){
            activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
        }else{
            activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        }

    }
    public static synchronized MainApplication getInstance() {
        return instance;
    }


    public final void setConnectionCamera(boolean connection){
        this.connectionCamera = connection;
    }

    public final boolean isConnectionCamera(){
        return this.connectionCamera;
    }

    @Override
    public void onCreate() {
        instance = this;
        //if(BuildConfig.DEBUG)
            //LogcatHelper.getInstance(this).start();
        super.onCreate();
        mCameraConnectStatus = CameraUtil.isAttachCamera(this);
        if(!BuildConfig.DEBUG){
            CrashReport.initCrashReport(getApplicationContext(), KeyConstants.BUGLY_KEY, true);
        }
        bSensorRotate = CommonUtil.isHaveSensorRotate(this);
        Env.setContext(this);
        Env.setAppStartTime();
        iniPreference();
        IntentFilter usbFilter = new IntentFilter();
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        registerReceiver(mUsbReceiver, usbFilter);
        String[] photoModel=getResources().getStringArray(R.array.unsupport);
        for (String photo:photoModel){
            if (photo.equalsIgnoreCase(Build.MODEL)||Build.MODEL.contains(photo)){
                bSupportMediaCodecInterface = true;
                break;
            }
        }
    }
    private BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            switch (action) {
                case UsbManager.ACTION_USB_DEVICE_ATTACHED: {
                    if (CameraUtil.isAttachCamera(context)) {
                        mCameraConnectStatus = true;
                        CommonUtil.Vibrate(context, 100);
                        EventBus.getDefault().post(new CameraConnectEvent(CameraConnectEvent.ACTION_USB_DEVICE_ATTACHED));
                    }
                }
                    break;
                case UsbManager.ACTION_USB_DEVICE_DETACHED: {
                    mCameraConnectStatus = false;
                    CommonUtil.Vibrate(context, 100);
                    EventBus.getDefault().post(new CameraConnectEvent(CameraConnectEvent.ACTION_USB_DEVICE_DETACHED));
                }
                    break;
            }
        }
    };
    private void iniPreference() {
        PreferenceModel.init(this);
    }
    public String getFirmVersionStr() {
        if (mSharedPreferences == null)
            mSharedPreferences = Env.getContext().getSharedPreferences(firmInfoDb, Context.MODE_PRIVATE);
        return mSharedPreferences.getString(firmInfoVersion, "");
    }
    public String getFirmSerialNumberStr() {
        if (mSharedPreferences == null)
            mSharedPreferences = Env.getContext().getSharedPreferences(firmInfoDb, Context.MODE_PRIVATE);
        return mSharedPreferences.getString(firmInfoSn, "");
    }
}