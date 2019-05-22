package com.uni.pano.activities;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import com.kevin.crop.UCrop;
import com.kevin.crop.util.BitmapLoadUtils;
import com.kevin.crop.view.GestureCropImageView;
import com.kevin.crop.view.OverlayView;
import com.kevin.crop.view.TransformImageView;
import com.kevin.crop.view.UCropView;
import com.uni.common.config.PathConfig;
import com.uni.common.util.BitmapUtil;
import com.uni.pano.R;
import com.uni.pano.base.BaseActivity;
import com.uni.pano.bean.MediaInfo;

import java.io.File;
import java.io.OutputStream;

import butterknife.BindView;
import butterknife.OnClick;

/**
 * Created by DELL on 2017/3/13.
 */

public class UCropActivity extends BaseActivity {
    @BindView(R.id.ucv_crop)
    UCropView ucv_crop;
    @BindView(R.id.tv_back)
    TextView tv_back;
    @BindView(R.id.tv_use)
    TextView tv_use;
    @BindView(R.id.sb_zoom)
    SeekBar sb_zoom;
    @BindView(R.id.tv_zoom)
    TextView tv_zoom;
    @BindView(R.id.sb_rotate)
    SeekBar sb_rotate;
    @BindView(R.id.tv_rotate)
    TextView tv_rotate;

    Uri inputUri;
    Uri outputUri;
    private GestureCropImageView gestureCropImageView;

    public static void startActivity(Activity activity, MediaInfo mediaInfo){
        Intent intent = new Intent(activity, UCropActivity.class);
        intent.putExtra(MediaInfo.TAG, mediaInfo);
        activity.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.av_crop);
        OverlayView overlayView = ucv_crop.getOverlayView();
        overlayView.setShowCropGrid(false);
        overlayView.setOvalDimmedLayer(true);
        overlayView.setShowCropFrame(false);
        gestureCropImageView = ucv_crop.getCropImageView();
        Intent intent = getIntent();
        Uri inputUri = intent.getParcelableExtra(UCrop.EXTRA_INPUT_URI);
        Uri outputUri = intent.getParcelableExtra(UCrop.EXTRA_OUTPUT_URI);
        if (inputUri != null && outputUri != null) {
            try {
                gestureCropImageView.setImageUri(inputUri);
            } catch (Exception e) {
                finish();
            }
        } else {
           finish();
        }
        gestureCropImageView.setTargetAspectRatio(1.0f);
        gestureCropImageView.setRotateEnabled(false);
        gestureCropImageView.setScaleType(ImageView.ScaleType.FIT_CENTER);
        gestureCropImageView.setMaxScaleMultiplier(5.6f);
        gestureCropImageView.setTransformImageListener(transformImageListener);
        sb_zoom.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float scale = (progress+50.f)/100.f;
                gestureCropImageView.setScaleX(scale);
                gestureCropImageView.setScaleY(scale);
                tv_zoom.setText(progress + 50 + "%");
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        sb_rotate.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                gestureCropImageView.setRotation(progress-180);
                tv_rotate.setText(progress-180+"°");
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

    }
    /**
     * @param bitmap src图片
     * @return
     */
    public static Bitmap getCircleBitmap(Bitmap bitmap) {
        Bitmap output = Bitmap.createBitmap( bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas( output);

        final int color = 0xff424242;
        final Paint paint = new Paint();
        final Rect rect = new Rect( 0, 0, bitmap.getWidth(), bitmap.getHeight());

        paint.setAntiAlias( true);
        paint.setFilterBitmap( true);
        paint.setDither( true);
        canvas.drawARGB( 0, 0, 0, 0);
        paint.setColor( color);
        //在画布上绘制一个圆
        canvas.drawCircle( bitmap.getWidth() / 2, bitmap.getHeight() / 2, bitmap.getWidth() / 2, paint);
        paint.setXfermode( new PorterDuffXfermode( PorterDuff.Mode.SRC_IN));
        canvas.drawBitmap( bitmap, rect, rect, paint);
        return output;
    }
    private void cropAndSaveImage() {
        final Bitmap croppedBitmap = gestureCropImageView.cropImage();
        if (croppedBitmap != null) {
            String fileName = PathConfig.getSelfLogoDir()+ File.separator+System.currentTimeMillis()+".jpeg";
            File file = new File(fileName);
            if (file.exists()) {
                file.delete();
                file = new File(fileName);
            }
            BitmapUtil.saveBitmap(file, getCircleBitmap(croppedBitmap));
            croppedBitmap.recycle();
            setResult(RESULT_OK, new Intent().putExtra(UCrop.EXTRA_OUTPUT_URI, fileName));
            finish();
        }
    }

    private TransformImageView.TransformImageListener transformImageListener = new TransformImageView.TransformImageListener() {
        @Override
        public void onRotate(float currentAngle) {
            //setAngleText(currentAngle);
        }

        @Override
        public void onScale(float currentScale) {
           // setScaleText(currentScale);
        }

        @Override
        public void onLoadComplete() {
            Animation fadeInAnimation = AnimationUtils.loadAnimation(getApplicationContext(), R.anim.ucrop_fade_in);
            fadeInAnimation.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {
                    ucv_crop.setVisibility(View.VISIBLE);
                }

                @Override
                public void onAnimationEnd(Animation animation) {

                }

                @Override
                public void onAnimationRepeat(Animation animation) {
                }
            });
            ucv_crop.startAnimation(fadeInAnimation);
        }
        @Override
        public void onLoadFailure(@NonNull Exception e) {
            finish();
        }
    };

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
    }
    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @OnClick({R.id.tv_back,R.id.tv_use})
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.tv_back:
                onBackPressed();
                break;
            case R.id.tv_use:
                cropAndSaveImage();
                break;
        }
    }
}
