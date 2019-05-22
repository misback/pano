package com.uni.pano.activities;
import android.app.Activity;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.uni.common.concurrent.SingleThreadExecutor;
import com.uni.common.dialog.CDialog;
import com.uni.common.env.Env;
import com.uni.common.util.PackageUtil;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.api.ApiClient;
import com.uni.pano.api.ApiService;
import com.uni.pano.api.RetrofitCallback;
import com.uni.pano.base.BaseActivity;
import com.uni.pano.blur.UtilBitmap;
import com.uni.pano.blur.UtilScreenCapture;
import com.uni.pano.entity.Result;
import com.uni.pano.entity.VersionInfo;
import com.uni.pano.fragment.AlbumFragment;
import com.uni.pano.fragment.SettingFragment;
import com.uni.pano.share.FBHelper;
import com.uni.pano.widget.CToast;

import butterknife.BindView;
import butterknife.OnClick;
import retrofit2.Call;
import retrofit2.Response;

/**
 * @描述：     @主界面
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */
public class PandaActivity extends BaseActivity {

    private final static String TAG = "PandaActivity";

    @BindView(R.id.tv_left_menu)
    TextView mTvLeftMenu;

    @BindView(R.id.tv_title)
    TextView mTvTitle;

    @BindView(R.id.tv_right_menu)
    TextView mTvRightMenu;


    @BindView(R.id.tv_album)
    TextView mTvAlbum;

    @BindView(R.id.iv_album)
    ImageView mIvAlbum;

    @BindView(R.id.tv_setting)
    TextView mTvSetting;

    @BindView(R.id.iv_setting)
    ImageView mIvSetting;


    @BindView(R.id.frame_album)
    FrameLayout mFrameAlbum;

    @BindView(R.id.frame_setting)
    FrameLayout mFrameSetting;

    @BindView(R.id.frame_camera)
    FrameLayout mFrameCamera;

    @BindView(R.id.frame_tab)
    FrameLayout mFrameTab;

    @BindView(R.id.cl_bottom_toolbar)
    View mBottomToolbar;

    @BindView(R.id.iv_bottom_toolbar_delete)
    ImageView mIvDelete;

    @BindView(R.id.iv_blur)
    ImageView mIvBlur;

    @BindView(R.id.ll_content_container)
    LinearLayout mLLcontentContainer;

    private SettingFragment mFMSetting;

    private AlbumFragment mFMAlbum;

    private Which mWhich = Which.CHECKED;

    private FBHelper mFBHelper;

    private DeleteWindow mDeleteWindow;
    private UpdateWindow mUpdateWindow;

    private WindowType  mWindowType = WindowType.UPDATE;

    public static void startActivity(Activity activity){
        Intent intent = new Intent(activity, PandaActivity.class);
        activity.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mFBHelper = new FBHelper(this);
        setContentView(R.layout.av_panda);
        mFMAlbum = (AlbumFragment) getFragmentManager().findFragmentById(R.id.fm_album);
        mFMSetting = (SettingFragment) getFragmentManager().findFragmentById(R.id.fm_set);
        mFMSetting.setFBHelper(mFBHelper);
        setAlbumTabState();
        initWindow();
    }

    /**
     * 初始化窗口
     */
    private final void initWindow(){
        mDeleteWindow = new DeleteWindow(this,mDialogListener);
        mUpdateWindow = new UpdateWindow(this, UpdateWindow.FROM_PANDA_ACTIVITY,mDialogListener, new BlurListener() {
            @Override
            public void showBlur() {
                PandaActivity.this.showBlur();
            }

            @Override
            public void hideBlur() {
                PandaActivity.this.hideBlur();
            }
        });
        isRunning();
        mUpdateWindow.checkVersion();
    }

    private final void isRunning(){
        SingleThreadExecutor.getInstance().execute(new Runnable() {
            @Override
            public void run() {
                final ApiService apiService          = ApiClient.retrofit().create(ApiService.class);
                final Call<Result<Boolean>> call     = apiService.isRunning(PackageUtil.getPackageName(Env.getContext()));
                call.enqueue(new RetrofitCallback<Result<Boolean>>() {
                    @Override
                    public void onSuccess(Call<Result<Boolean>> call, Response<Result<Boolean>> response) {
                        final Result<Boolean> result = response.body();
                        if(null != result){
                            if(result.rtn == 1 &&
                                    result.data.booleanValue() == false){
                                //退出app
                                finish();
                            }
                        }

                    }

                    @Override
                    public void onFailure(Call<Result<Boolean>> call, Throwable t) {

                    }
                });
            }
        });
        SingleThreadExecutor.getInstance().shutdown();

    }
    private final CDialog.DialogListener mDialogListener = new CDialog.DialogListener() {
        @Override
        public void onCancelListener() {
            if(mWindowType == WindowType.DELETE){
                hideBlur();
            }
        }

        @Override
        public void onConfrimListener() {
            if(mWindowType == WindowType.DELETE){
                hideBlur();
                mFMAlbum.delete();
                updateHeaderAndBottomInfo();
                if(!mFMAlbum.hasFile()){
                    mWhich = Which.UNCHECKED;
                    switchCheckStatus(false);
                }
            }
        }

        @Override
        public boolean onKeyListener() {
            switch (mWindowType){
                case DELETE:
                    hideBlur();
                    break;
                case UPDATE:
                    if(mUpdateWindow.isRecommend()){
                        hideBlur();
                    }else{
                        return false;
                    }

            }
            return true;
        }
    };

    //设置相册为选中状态
    private final void setAlbumTabState(){
        FragmentTransaction transaction =   this.getFragmentManager().beginTransaction();
        transaction.hide(mFMSetting).show(mFMAlbum).commit();
        mTvLeftMenu.setVisibility(View.VISIBLE);
        mTvTitle.setText(getString(R.string.album));
        mTvAlbum.setTextColor(getResources().getColor(R.color.C_FF_74_3A));
        mTvSetting.setTextColor(getResources().getColor(R.color.C_64_5E_66));
        mIvAlbum.setBackgroundResource(R.drawable.ic_album_s);
        mIvSetting.setBackgroundResource(R.drawable.ic_setting_n);
    }

    //设置设置为选中状态
    private final void setSettingTabState(){
        FragmentTransaction transaction =   this.getFragmentManager().beginTransaction();
        transaction.hide(mFMAlbum).show(mFMSetting).commit();
        mTvTitle.setText(getString(R.string.settings));
        mTvAlbum.setTextColor(getResources().getColor(R.color.C_64_5E_66));
        mTvSetting.setTextColor(getResources().getColor(R.color.C_FF_74_3A));
        mIvAlbum.setBackgroundResource(R.drawable.ic_album_n);
        mIvSetting.setBackgroundResource(R.drawable.ic_setting_s);
        mTvLeftMenu.setVisibility(View.GONE);
    }

    @OnClick({R.id.frame_album, R.id.frame_camera, R.id.frame_setting,R.id.tv_left_menu, R.id.tv_right_menu,R.id.cl_bottom_toolbar})
    public void onAlbumAndFilterClick(View view) {
        switch (view.getId()){
            case R.id.frame_album:
                setAlbumTabState();
                break;
            case R.id.frame_camera:
                PanoramaCameraActivity.startActivity(this);
                break;
            case R.id.frame_setting:
                setSettingTabState();
                break;
            case R.id.tv_left_menu:
                switchCheckStatus(false);
                break;
            case R.id.tv_right_menu:
                if(mWhich == Which.CANCEL_SELECT_ALL){
                    switchCheckStatus(true);
                }else{
                    mWhich = Which.CANCEL_SELECT_ALL;
                    mFMAlbum.selectAll();
                    mTvRightMenu.setText(R.string.cancel_select_all);
                }
                updateHeaderAndBottomInfo();
                break;
            case R.id.cl_bottom_toolbar:
                if(mDeleteWindow.getNumber() > 0){
                    mWindowType = WindowType.DELETE;
                    showDeletWindow();
                }else{
                    CToast.showToast(R.string.select_item);
                }
                break;
        }
    }

    /**
     * 显示删除窗口
     */
    public final void showDeletWindow(){
        if(null != mDeleteWindow){
            showBlur();
            mDeleteWindow.show();
        }
    }

    /**
     * 显示毛玻璃效果
     */
    public final void showBlur(){
        Bitmap bitmap = UtilScreenCapture.getDrawing(this);
        if (bitmap != null) {
            mIvBlur.setVisibility(View.VISIBLE);
            mLLcontentContainer.setVisibility(View.INVISIBLE);
            // 将截屏Bitma放入ImageView
            mIvBlur.setImageBitmap(bitmap);
            // 将ImageView进行高斯模糊【25是最高模糊等级】【0x77000000是蒙上一层颜色，此参数可不填】
            UtilBitmap.blurImageView(this,mIvBlur,10, 0x77000000);
        } else {
            // 获取的Bitmap为null时，用半透明代替
            mIvBlur.setBackgroundColor(0x77000000);
        }
    }

    /**
     * 隐藏毛玻璃效果
     */
    public final void hideBlur(){
        mIvBlur.setImageBitmap(null);
        mIvBlur.setVisibility(View.INVISIBLE);
        mLLcontentContainer.setVisibility(View.VISIBLE);
    }

    public final Which getCheckStatus(){
        return mWhich;
    }

    /**
     * 更新选择数量
     */
    public final void updateHeaderAndBottomInfo(){
        final int number = mFMAlbum.getCheckedNumber();
        if(number > 0){
            mTvTitle.setText(String.format(getString(R.string.already_select_item),
                    String.valueOf(number)));
            mDeleteWindow.setNumber(number);
            mIvDelete.setImageResource(R.drawable.ic_delete_s);
        }else{
            mTvTitle.setText(R.string.select_item);
            mDeleteWindow.setNumber(0);
            mIvDelete.setImageResource(R.drawable.ic_delete_n);
        }
    }

    /**
     * 设置状态
     * @param which
     */
    public final void setCheckStatus(Which which){
        this.mWhich = which;
    }

    /**
     * 切换选中状态
     */
    public final void switchCheckStatus(boolean cancelSelectAll){
        switch (mWhich){
            case CHECKED:
                if(mFMAlbum.hasFile()){
                    mWhich = Which.UNCHECKED;
                    mTvLeftMenu.setText(R.string.cancel);
                    mTvLeftMenu.setTextColor(getResources().getColor(R.color.C_FF_74_3A));
                    mTvRightMenu.setVisibility(View.VISIBLE);
                    mTvRightMenu.setText(R.string.select_all);
                    mTvRightMenu.setTextColor(getResources().getColor(R.color.C_FF_74_3A));
                    mBottomToolbar.setVisibility(View.VISIBLE);
                    mFrameTab.setVisibility(View.GONE);
                    mFMAlbum.showItemSelectable(true);
                }else{
                    CToast.showToast(R.string.no_file_select);
                }
                break;
            case UNCHECKED:
                exitSelect();
                break;
            case CANCEL_SELECT_ALL:
                mWhich = Which.UNCHECKED;
                mTvLeftMenu.setText(R.string.cancel);
                mTvLeftMenu.setTextColor(getResources().getColor(R.color.C_FF_74_3A));
                mTvRightMenu.setVisibility(View.VISIBLE);
                mTvRightMenu.setText(R.string.select_all);
                if(cancelSelectAll){
                    mFMAlbum.cancelSelectAll();
                }
                break;
        }
    }

    //退出选择
    private final void exitSelect(){
        mWhich = Which.CHECKED;
        mTvLeftMenu.setText(R.string.select);
        mTvLeftMenu.setTextColor(getResources().getColor(R.color.C_FF_00_00_00));
        mTvRightMenu.setVisibility(View.INVISIBLE);
        mFrameTab.setVisibility(View.VISIBLE);
        mIvDelete.setImageResource(R.drawable.ic_delete_n);
        mBottomToolbar.setVisibility(View.GONE);
        mFMAlbum.showItemSelectable(false);
        mTvTitle.setText(R.string.album);
    }


    public enum Which{
        CHECKED,UNCHECKED,CANCEL_SELECT_ALL
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        mFBHelper.registerCallbackActivityResult(requestCode,resultCode,data);
    }



    //记录第一次点击的时间
    private long mClickTime = 0;

    //时间间隔
    public static final long TIME_LIMIT = 2000;

    @Override
    public void onBackPressed() {
        if(mWhich == Which.UNCHECKED){
            exitSelect();
            return;
        }

        if ((System.currentTimeMillis() - mClickTime) > TIME_LIMIT) {
            CToast.showToast(ResourceUtil.getString(R.string.exit_app_toast));
            mClickTime = System.currentTimeMillis();
        } else {
            super.onBackPressed();
        }
    }

}