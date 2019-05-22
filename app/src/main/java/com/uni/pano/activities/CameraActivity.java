package com.uni.pano.activities;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.SwitchCompat;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.lwm.shapeimageview.RoundedImageView;
import com.uni.common.dialog.CDialog;
import com.uni.common.util.DateTimeUtil;
import com.uni.common.util.FileUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.common.util.TimeUtil;
import com.uni.pano.MainApplication;
import com.uni.pano.R;
import com.uni.pano.base.BaseActivity;
import com.uni.pano.bean.ArrayMediaInfo;
import com.uni.pano.bean.FilterInfo;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.blur.UtilBitmap;
import com.uni.pano.blur.UtilScreenCapture;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.CreateFileEvent;
import com.uni.pano.event.DeleteFileEvent;
import com.uni.pano.event.ScreenShotEvent;
import com.uni.pano.event.VideoGLViewCreateEvent;
import com.uni.pano.logutils.LogcatHelper;
import com.uni.pano.share.FBHelper;
import com.uni.pano.share.ShareWindow;
import com.uni.pano.utils.CommonUtil;
import com.uni.pano.widget.CToast;
import com.uni.vr.PanoramaCameraView;
import com.uni.vr.PanoramaImageView;
import com.uni.vr.PanoramaVideoView;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import butterknife.BindView;
import butterknife.OnClick;

/**
 * Created by DELL on 2017/3/13.
 */

public class CameraActivity extends BaseActivity {
    @BindView(R.id.pcv_camera)
    PanoramaCameraView pcv_camera;

    public static void startActivity(Activity activity){
        Intent intent = new Intent(activity, CameraActivity.class);
        activity.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.av_camera);
        LogcatHelper.getInstance(this).start();
        init();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        init();
    }
    private void init(){
     //   pcv_camera.startCamera(this);
    }
    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onEventMainThread(VideoGLViewCreateEvent videoGLViewCreateEvent) {

        pcv_camera.startCamera(this);
    }
}
