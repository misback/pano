package com.uni.pano.activities;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.lwm.shapeimageview.RoundedImageView;
import com.uni.common.config.PathConfig;
import com.uni.common.util.FileSizeUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.common.util.UnitConvertUtils;
import com.uni.pano.MainApplication;
import com.uni.pano.R;
import com.uni.pano.base.BaseActivity;
import com.uni.pano.bean.ArrayMediaInfo;
import com.uni.pano.bean.FilterInfo;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.config.EnumElement;
import com.uni.pano.event.CameraConnectEvent;
import com.uni.pano.event.ConnectEvent;
import com.uni.pano.event.CreateFileEvent;
import com.uni.pano.event.DeleteFileEvent;
import com.uni.pano.event.DisConnectEvent;
import com.uni.pano.event.RecordTimeEvent;
import com.uni.pano.event.TakePhotoEvent;
import com.uni.pano.logutils.LogcatHelper;
import com.uni.pano.utils.CameraUtil;
import com.uni.pano.utils.CommonUtil;
import com.uni.pano.widget.CToast;
import com.uni.vr.CameraVideoView;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import java.io.File;
import java.util.ArrayList;

import butterknife.BindView;
import butterknife.OnClick;

public class PanoramaCameraActivity extends BaseActivity {
    public final static String TAG = PanoramaCameraActivity.class.getSimpleName();
    public static final long MAX_RECORD_TIME       = 15 * 60;
    public static final long MIN_AVAIL_MEMORY_SIZE = 500*1024*1024;
    @BindView(R.id.cvv_camera)
    CameraVideoView cvv_camera;
    @BindView(R.id.rl_top)
    RelativeLayout rl_top;
    @BindView(R.id.ib_close)
    ImageButton ib_close;
    @BindView(R.id.tv_title)
    TextView tv_title;
    @BindView(R.id.ib_help)
    ImageButton ib_help;
    @BindView(R.id.rl_not_connect)
    RelativeLayout rl_not_connect;
    @BindView(R.id.tv_not_connect)
    TextView tv_not_connect;
    @BindView(R.id.rv_filter)
    RecyclerView rv_filter;
    @BindView(R.id.cb_flip)
    CheckBox cb_flip;
    @BindView(R.id.ll_bottom)
    LinearLayout ll_bottom;
    @BindView(R.id.cb_take_photo)
    CheckBox cb_take_photo;
    @BindView(R.id.cb_record_video)
    CheckBox cb_record_video;
    @BindView(R.id.iv_photo)
    ImageView iv_photo;
    @BindView(R.id.ib_take_photo)
    ImageButton ib_take_photo;
    @BindView(R.id.cb_record_video_ing)
    CheckBox cb_record_video_ing;
    @BindView(R.id.cb_filter)
    CheckBox cb_filter;
    @BindView(R.id.iv_view_mode)
    ImageView iv_view_mode;


    // 拍照录像音效
    private SoundPoolManage mSoundManage;
    EnumElement.VIEW_MODE view_mode = EnumElement.VIEW_MODE.FISH;
    private MediaInfo mediaInfo = null;
    private boolean recordState = false;

    private void doStartRecord(){
        if (recordState){
            return;
        }
        cb_record_video_ing.setEnabled(false);
        recordState = true;
        cvv_camera.doStartRecord();

    }



    private void doStopRecord(){
        if(!recordState){
            return;
        }
        cvv_camera.doStopRecord();
        cb_record_video_ing.setChecked(false);
    }

    public static void startActivity(Activity activity){
        Intent intent = new Intent(activity, PanoramaCameraActivity.class);
        activity.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LogcatHelper.getInstance(this).start();
        setContentView(R.layout.av_panorama_camera);

        initFilter();
        updateView(false);
        if(PreferenceModel.isShutterSoundOpen()){
            mSoundManage = new SoundPoolManage(this);
        }
        initCamera();
    }

    @Override
    protected void onResume() {
        super.onResume();
        cvv_camera.onResume();
    }

    @Override
    protected void onPause() {
        cvv_camera.onPause();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        //unregisterReceiver(broadcastReceiver);
        cvv_camera.onDestroy();
        if(null != mSoundManage){
            mSoundManage.releaseSound();
        }
    }
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        onCheckPermission();
    }

    private void updateView(boolean isConnected){
        rl_not_connect.setVisibility(isConnected?View.GONE:View.VISIBLE);
        cb_flip.setEnabled(isConnected);
        cb_take_photo.setEnabled(isConnected);
        cb_record_video.setEnabled(isConnected);
        ib_take_photo.setEnabled(isConnected);
        cb_record_video_ing.setEnabled(isConnected);
        cb_filter.setChecked(false);
        rv_filter.setVisibility(View.GONE);
        cb_filter.setEnabled(isConnected);
        iv_view_mode.setEnabled(isConnected);
        cvv_camera.setVisibility(isConnected?View.VISIBLE:View.GONE);
        iv_photo.setVisibility(mediaInfo != null? View.VISIBLE:View.GONE);
        iv_view_mode.setImageResource(R.drawable.selector_fisheye);
    }

    @OnClick({R.id.ib_close, R.id.ib_help, R.id.tv_not_connect, R.id.cb_flip, R.id.cb_take_photo, R.id.cb_record_video, R.id.iv_photo,  R.id.ib_take_photo, R.id.cb_record_video_ing, R.id.cb_filter, R.id.iv_view_mode})
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.ib_close:
                finish();
                PandaActivity.startActivity(this);
                break;
            case R.id.ib_help:
                break;
            case R.id.cb_flip:
                cvv_camera.doUpdateRotate(cb_flip.isChecked()?180.f:-180.f);
                break;
            case R.id.cb_take_photo:
                tv_title.setText(R.string.photograph);
                cb_record_video.setChecked(false);
                cb_record_video.setEnabled(true);
                cb_take_photo.setChecked(true);
                cb_take_photo.setEnabled(false);
                ib_take_photo.setVisibility(View.VISIBLE);
                cb_record_video_ing.setVisibility(View.GONE);
                cb_record_video_ing.setEnabled(true);
                boolean isOpen = PreferenceModel.isShutterSoundOpen();
                Log.v(TAG,"isOpen--->" + isOpen);
                if (cb_record_video_ing.isChecked()){
                    doStopRecord();
                }
                break;
            case R.id.cb_record_video:

                tv_title.setText(R.string.record);
                cb_record_video.setChecked(true);
                cb_record_video.setEnabled(false);
                cb_take_photo.setChecked(false);
                cb_take_photo.setEnabled(true);
                ib_take_photo.setVisibility(View.GONE);
                cb_record_video_ing.setEnabled(true);
                cb_record_video_ing.setVisibility(View.VISIBLE);
                break;
            case R.id.iv_photo:
                if (mediaInfo!=null){
                    if (cb_record_video_ing.isChecked()){
                        doStopRecord();
                    }
                    PanoramaPreviewActivity.startActivity(this, mediaInfo, new ArrayMediaInfo(CommonUtil.listMediaInfo(PathConfig.getMediaFolder())));
                }

                break;
            case R.id.ib_take_photo:
                cvv_camera.doTakePhoto(1);
                break;
            case R.id.cb_record_video_ing:
                if (cb_record_video_ing.isChecked()){
                    if (FileSizeUtil.getAvailableMemorySize(PathConfig.getMediaFolder())>=MIN_AVAIL_MEMORY_SIZE) {
                        doStartRecord();
                    }else{
                        CToast.showToast(getString(R.string.not_free));
                    }
                }else{
                    doStopRecord();
                }
                break;
            case R.id.cb_filter:
                rv_filter.setVisibility(cb_filter.isChecked()?View.VISIBLE:View.GONE);

                break;
            case R.id.iv_view_mode:
                int vMode = view_mode.ordinal() + 1;
                view_mode = EnumElement.VIEW_MODE.values()[vMode % (EnumElement.VIEW_MODE.CRYSTAL_BALL.ordinal()+1)];
                updateViewMode(view_mode);
                break;
        }
    }

    public void updateViewMode(final EnumElement.VIEW_MODE view_mode){
        this.view_mode = view_mode;
        cvv_camera.doUpdateViewMode(view_mode);
        switch (view_mode){
            case FISH:
                iv_view_mode.setImageResource(R.drawable.selector_fisheye);
                break;
            case PERSPECTIVE:
                iv_view_mode.setImageResource(R.drawable.selector_perspective);
                break;
            case PLANET:
                iv_view_mode.setImageResource(R.drawable.selector_planet);
                break;
            case CRYSTAL_BALL:
                iv_view_mode.setImageResource(R.drawable.selector_crystal_ball);
                break;
        }
    }

    private UsbDevice usbDevice;
    private UsbManager usbManager;
    private PendingIntent pendingIntent;
    private UsbDeviceConnection connection;
    private void onConnect(final UsbDevice usbDevice){
        this.usbDevice  =   usbDevice;
        if(this.usbDevice != null) {
            connection = usbManager.openDevice(this.usbDevice);
            if (connection != null && usbManager.hasPermission(usbDevice)) {
                cvv_camera.setVisibility(View.VISIBLE);
                cvv_camera.doConnect(this.usbDevice.getVendorId(), this.usbDevice.getProductId(), connection.getFileDescriptor(), CameraUtil.getUsbFsName(usbDevice));
            } else {
                CToast.showToast(getString(R.string.connect_fail));
            }
        }else{
            CToast.showToast(getString(R.string.connect_fail));
        }
    }


    public void onCheckPermission() {
        UsbDevice usbDevice =   CameraUtil.getUsbDevice(this);
        if (usbDevice != null) {
            if (usbManager.hasPermission(usbDevice)) {
                onConnect(usbDevice);
            } else {
                usbManager.requestPermission(usbDevice, pendingIntent);
            }
        }
    }
    private void initCamera(){
        usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        pendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(CameraConnectEvent.ACTION_DEVICE_PERMISSION), 0);
        onCheckPermission();
    }

    @Override
    protected void onConnectCamera(boolean connect) {
        super.onConnectCamera(connect);
        if(!connect){
            cvv_camera.doDisConnect();
        }
    }

    private final void enableRecordTips(){
        final Drawable drawable = getResources().getDrawable(R.drawable.ic_record_video_tips);
        /// 这一步必须要做,否则不会显示.
        drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());
        tv_title.setCompoundDrawables(drawable,null,null,null);
    }

    private final void disableRecordTips(){
        tv_title.setCompoundDrawables(null,null,null,null);
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onRecordTimeEvent(RecordTimeEvent recordTimeEvent){
        switch (recordTimeEvent.recordState){
            case START:{
                enableRecordTips();
                cb_take_photo.setEnabled(false);
                cb_record_video_ing.setEnabled(true);
//                tv_time.setVisibility(View.VISIBLE);
                tv_title.setText(UnitConvertUtils.formatSecondTimeForHour(0));
            }
            break;
            case UPDATE_TIME:{
                final long time = recordTimeEvent.seconds / 1000;
                tv_title.setText(UnitConvertUtils.formatSecondTimeForHour(time));

                if(PreferenceModel.isRecordTimeLimitOpen()){
                    if (time == MAX_RECORD_TIME){
                        doStopRecord();
                    }
                }
            }
            break;
            case STOP:{
                disableRecordTips();
                cb_take_photo.setEnabled(true);
                tv_title.setText(R.string.record);
                String filePath = recordTimeEvent.filePath;
                File file = new File(filePath);
                mediaInfo = new MediaInfo(file);
                if(mediaInfo.valid) {
                    EventBus.getDefault().post(new CreateFileEvent(filePath));
                    Glide.with(this).load(file).into(iv_photo);
                    iv_photo.setVisibility(View.VISIBLE);
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
                recordState =   false;
            }
            break;
        }
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onTakePhoto(TakePhotoEvent takePhotoEvent){
        switch (takePhotoEvent.take_photo_result){
            case START:{
                if(PreferenceModel.isShutterSoundOpen()){
                    if(null != mSoundManage){
                        mSoundManage.playSound();
                    }
                }
                cb_take_photo.setEnabled(false);
                cb_record_video.setEnabled(false);
                CToast.showToast(getString(R.string.start_photo));
            }
            break;
            case NOT_CONNECT:{
                CToast.showToast(getString(R.string.not_connect_camera));
            }
            break;
            case PHOTOING:{
                CToast.showToast(getString(R.string.taking_photo));
            }
            break;
            case NOT_SPACE:{
                CToast.showToast(getString(R.string.not_free));
            }
            break;
            case COMPLETE:{
                cb_record_video.setEnabled(true);
                cb_take_photo.setEnabled(true);
                String filePath = takePhotoEvent.filePath;
                File file = new File(filePath);
                mediaInfo = new MediaInfo(file);
                if(mediaInfo.valid) {
                    EventBus.getDefault().post(new CreateFileEvent(filePath));
                    Glide.with(this).load(file).into(iv_photo);
                    iv_photo.setVisibility(View.VISIBLE);
                    MediaScannerConnection.scanFile(MainApplication.getInstance(), new String[] { mediaInfo.filePath },
                            null, new MediaScannerConnection.OnScanCompletedListener() {
                                public void onScanCompleted(final String path, Uri uri) {

                                }
                            });
                }else{
                    if (mediaInfo.delete) {
                        file.delete();
                    }
                }
            }
            break;
        }
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onConnectCamera(ConnectEvent connectEvent) {
        switch (connectEvent.connect_result){
            case SUCCESS:{
                CToast.showToast(getString(R.string.connect_success));
                cvv_camera.setVisibility(View.VISIBLE);
                rl_not_connect.setVisibility(View.GONE);
                updateView(true);
                //cvv_camera.doUpdateEffect("glass_d.png", 1);
            }
            break;
            case FAIL:{
                CToast.showToast(getString(R.string.connect_fail));
                updateView(false);
            }
            break;
        }
    }
    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onDisConnectCamera(DisConnectEvent disConnectEvent) {
        switch (disConnectEvent.disconnect_result){
            case SUCCESS:{
                CToast.showToast(getString(R.string.disconnect));
                updateView(false);
                usbDevice = null;
                if (connection != null) {
                    connection.close();
                    connection = null;
                }
            }
            break;
            case FAIL:{
                CToast.showToast(getString(R.string.disconnect_fail));
            }
            break;
        }
    }
    ArrayList<FilterInfo> filterInfos = new ArrayList<FilterInfo>();
    FilterAdapter filterAdapter;
    private void initFilter(){
        final int margin                           = getResources().getDimensionPixelOffset(R.dimen.dp_15);

        LinearLayoutManager linearLayoutManager    =   new LinearLayoutManager(this, LinearLayoutManager.HORIZONTAL, false);
        rv_filter.setLayoutManager(linearLayoutManager);
        rv_filter.addItemDecoration(new RecyclerView.ItemDecoration() {
            @Override
            public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
                super.onDraw(c, parent, state);
            }
            @Override
            public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                outRect.set(0, 0, margin, 0);
            }
        });

        filterInfos.add(new FilterInfo(getString(R.string.filter_name_original),
                R.drawable.ic_filter_original, "", true));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_gray_relief),
                R.drawable.ic_filter_gray_relief, "",
                false, EnumElement.FILTER_TYPE.GRAY_RELIEF));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_budapest), R.drawable.ic_filter_budapest, "1.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_chaplin), R.drawable.ic_filter_chaplin, "2.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_autumn), R.drawable.ic_filter_autumn, "3.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_oak), R.drawable.ic_filter_oak, "4.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_monroe), R.drawable.ic_filter_monroe, "5.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_dejavu), R.drawable.ic_filter_dejiavu, "6.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_lavie), R.drawable.ic_filter_lavie, "7.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_lucifer), R.drawable.ic_filter_lucifer, "8.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_memo), R.drawable.ic_filter_memo, "9.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_confess), R.drawable.ic_filter_confess, "10.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_sapporo), R.drawable.ic_filter_sapporo, "11.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_lp), R.drawable.ic_filter_lp, "12.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_blossom), R.drawable.ic_filter_blossom, "13.bmp", false));
        filterInfos.add(new FilterInfo(getString(R.string.filter_name_gray_relief), R.drawable.ic_filter_gray_relief, "", false, EnumElement.FILTER_TYPE.GRAY_RELIEF));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_glass_a), R.drawable.glass_a, "glass_a.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_glass_b), R.drawable.glass_b, "glass_b.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_glass_c), R.drawable.glass_c, "glass_c.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_glass_d), R.drawable.glass_d, "glass_d.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_glass_e), R.drawable.glass_e, "glass_e.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_a), R.drawable.hat_a, "hat_a.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_b), R.drawable.hat_b, "hat_b.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_c), R.drawable.hat_c, "hat_c.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_d), R.drawable.hat_d, "hat_d.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_e), R.drawable.hat_e, "hat_e.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_f), R.drawable.hat_f, "hat_f.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_g), R.drawable.hat_g, "hat_g.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_h), R.drawable.hat_h, "hat_h.png", false, true));
//        filterInfos.add(new FilterInfo(getString(R.string.filter_hat_i), R.drawable.hat_i, "hat_i.png", false, true));
        filterAdapter = new FilterAdapter();
        rv_filter.setAdapter(filterAdapter);
        filterAdapter.notifyDataSetChanged();
    }
    private RoundedImageView roundedImageView = null;
    class FilterAdapter extends RecyclerView.Adapter<FilterAdapter.GalleryViewHolder> {
        @Override
        public FilterAdapter.GalleryViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            FilterAdapter.GalleryViewHolder holder = new FilterAdapter.GalleryViewHolder(
                    LayoutInflater.from(PanoramaCameraActivity.this).inflate(R.layout.item_filter, parent, false));
            return holder;
        }
        @Override
        public void onBindViewHolder(FilterAdapter.GalleryViewHolder holder, int position) {
            FilterInfo filterInfo = filterInfos.get(position);
            holder.riv_filter.setTag(filterInfo);
            holder.riv_filter.setImageResource(filterInfo.resId);
            holder.tv_name.setText(filterInfo.filterName);
            if(filterInfo.isSelected) {
                holder.riv_filter.setBorderColor(R.color.C_FF_67_1B);
                roundedImageView = holder.riv_filter;
            }else{
                holder.riv_filter.setBorderColor(R.color.C_00_00_00_00);
            }
            holder.riv_filter.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    roundedImageView.setBorderColor(R.color.C_00_00_00_00);
                    FilterInfo roundFilterInfo = (FilterInfo)roundedImageView.getTag();
                    roundFilterInfo.isSelected = false;
                    FilterInfo filterInfo = (FilterInfo)v.getTag();
                    filterInfo.isSelected = true;
                    roundedImageView = (RoundedImageView)v;
                    roundedImageView.setBorderColor(R.color.C_FF_67_1B);
                    cvv_camera.doUpdateLutFilter(
                            PanoramaCameraActivity.this.getAssets(),
                            filterInfo.filterFileName, filterInfo.filterType,
                            filterInfo.useFaceDetector?1:0);
                }
            });
        }
        @Override
        public int getItemCount() {
            return filterInfos.size();
        }
        class GalleryViewHolder extends RecyclerView.ViewHolder {
            View view;
            RoundedImageView riv_filter;
            TextView tv_name;
            public GalleryViewHolder(View view) {
                super(view);
                this.view = view;
                riv_filter = (RoundedImageView)view.findViewById(R.id.riv_filter);
                tv_name = (TextView)view.findViewById(R.id.tv_name);
            }
        }
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onDeleteFile(DeleteFileEvent deleteFileEvent) {
        this.mediaInfo = null;
        this.iv_photo.setVisibility(View.INVISIBLE);
    }
}