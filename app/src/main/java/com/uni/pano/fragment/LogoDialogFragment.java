package com.uni.pano.fragment;

import android.app.FragmentManager;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.TabLayout;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.kevin.crop.UCrop;
import com.lwm.shapeimageview.CircularImageView;
import com.uni.common.config.PathConfig;
import com.uni.common.util.FileUtil;
import com.uni.common.util.LanguageUtil;
import com.uni.common.util.PreferenceModel;
import com.uni.pano.MainApplication;
import com.uni.pano.R;
import com.uni.pano.activities.UCropActivity;
import com.uni.pano.base.BaseDialogFragment;
import com.uni.pano.bean.LogoInfo;
import com.uni.pano.widget.CToast;
import com.uni.vr.PanoramaImageView;

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Locale;

import butterknife.BindView;
import butterknife.OnClick;

import static android.app.Activity.RESULT_OK;

public class LogoDialogFragment extends BaseDialogFragment {
    public final static String TAG = LogoDialogFragment.class.getSimpleName();
    public final static int REQUEST_SELECT_PICTURE = 90;
    public final static int REQUEST_STORAGE_READ_ACCESS_PERMISSION = 101;
    public final static int REQUEST_STORAGE_WRITE_ACCESS_PERMISSION = 102;
    public final static String SAMPLE_CROPPED_IMAGE_NAME = "SampleCropImage.jpeg";
    @BindView(R.id.piv_play)
    PanoramaImageView piv_play;
    @BindView(R.id.rv_unipano)
    RecyclerView rv_unipano;
    @BindView(R.id.rv_self)
    RecyclerView rv_self;
    @BindView(R.id.rv_none)
    RecyclerView rv_none;
    @BindView(R.id.ll_adjust)
    LinearLayout ll_adjust;
    @BindView(R.id.tl_type)
    TabLayout tl_type;
    @BindView(R.id.ib_delete)
    ImageButton ib_delete;
    @BindView(R.id.sb_zoom)
    SeekBar sb_zoom;
    @BindView(R.id.tv_zoom)
    TextView tv_zoom;

    String logoFileName;
    private Uri mDestinationUri;

    public static void show(FragmentManager fragmentManager){
        LogoDialogFragment logoDialogFragment = new LogoDialogFragment();
        logoDialogFragment.show(fragmentManager, LogoDialogFragment.TAG);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mDestinationUri = Uri.fromFile(new File(this.getActivity().getCacheDir(), SAMPLE_CROPPED_IMAGE_NAME));
    }

    @Override
    protected void initView() {
        setTitle(R.string.watermark);
        logoFileName = PreferenceModel.getString("logoFileName", "logo_none.png");
        initLogo();
        piv_play.loadImage("sample.jpg", this.getActivity().getAssets());
        piv_play.doUpdateRotate(0.f, 89.f, true);
        tl_type.addTab(tl_type.newTab().setText(R.string.app_name));
        tl_type.addTab(tl_type.newTab().setText(R.string.user_defined));
        tl_type.addTab(tl_type.newTab().setText(R.string.none));
        tl_type.addTab(tl_type.newTab().setText(R.string.adjust));
        tl_type.setOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                switch (tab.getPosition()){
                    case 0:{
                        rv_unipano.setVisibility(View.VISIBLE);
                        rv_self.setVisibility(View.GONE);
                        rv_none.setVisibility(View.GONE);
                        ib_delete.setVisibility(View.GONE);
                        ll_adjust.setVisibility(View.GONE);
                    }
                        break;
                    case 1:{
                        rv_unipano.setVisibility(View.GONE);
                        rv_self.setVisibility(View.VISIBLE);
                        rv_none.setVisibility(View.GONE);
                        ib_delete.setVisibility(View.GONE);
                        ll_adjust.setVisibility(View.GONE);
                        for (LogoInfo logoInfo:selfLogoInfos){
                            if (logoInfo.isSelected){
                                ib_delete.setVisibility(View.VISIBLE);
                                break;
                            }
                        }
                    }
                        break;
                    case 2:{

                        rv_unipano.setVisibility(View.GONE);
                        rv_self.setVisibility(View.GONE);
                        rv_none.setVisibility(View.VISIBLE);
                        ib_delete.setVisibility(View.GONE);
                        ll_adjust.setVisibility(View.GONE);
                    }
                        break;
                    case 3:{
                        rv_unipano.setVisibility(View.GONE);
                        rv_self.setVisibility(View.GONE);
                        rv_none.setVisibility(View.GONE);
                        ib_delete.setVisibility(View.GONE);
                        ll_adjust.setVisibility(View.VISIBLE);
                    }
                        break;
                }
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {

            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {

            }
        });
        int zoom = PreferenceModel.getInt("ZOOM", 15);
        sb_zoom.setProgress(zoom-5);
        tv_zoom.setText(sb_zoom.getProgress()+5+"°");
        sb_zoom.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tv_zoom.setText(progress+5+"°");
                PreferenceModel.putInt("ZOOM", progress+5);
                piv_play.doUpdateLogoAngle(progress+5);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    @Override
    public void onResume() {
        super.onResume();
        piv_play.onResume();
    }

    @Override
    public void onPause() {
        piv_play.onPause();
        super.onPause();
    }

    @Override
    public void onDestroy() {
        piv_play.onDestroy();
        super.onDestroy();
    }

    @OnClick({R.id.tv_back, R.id.ib_delete})
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.tv_back:{
                dismiss();
            }
                break;
            case R.id.ib_delete:{
                ib_delete.setVisibility(View.GONE);
                Iterator<LogoInfo> iterator = selfLogoInfos.iterator();
                int i=0;
                while(iterator.hasNext()){
                    LogoInfo logoInfo = iterator.next();
                    if (logoFileName.equals(logoInfo.logoFileName)){
                        FileUtil.deleteFile(logoFileName);
                        iterator.remove();
                        break;
                    }
                    i++;
                }
                selfLogoAdapter.notifyItemRemoved(i);
                PreferenceModel.remove("logoFileName");
                logoFileName = PreferenceModel.getString("logoFileName", "logo_none.png");
                piv_play.doLoadLogoImage(LogoDialogFragment.this.getActivity().getAssets(), logoFileName);

            }
                break;
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        getDialog().requestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreateView(inflater, container, savedInstanceState);
        viewInject(inflater, container, R.layout.dfm_logo);
        return mView;
    }

    ArrayList<LogoInfo> uniLogoInfos = new ArrayList<LogoInfo>();
    ArrayList<LogoInfo> selfLogoInfos = new ArrayList<LogoInfo>();
    ArrayList<LogoInfo> noneLogoInfos = new ArrayList<LogoInfo>();
    UniLogoAdapter uniLogoAdapter;
    SelfLogoAdapter selfLogoAdapter;
    NoneLogoAdapter noneLogoAdapter;
    private void initLogo(){
        {
            LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this.getActivity(), LinearLayoutManager.HORIZONTAL, false);
            rv_unipano.setLayoutManager(linearLayoutManager);
            rv_unipano.addItemDecoration(new RecyclerView.ItemDecoration() {
                @Override
                public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
                    super.onDraw(c, parent, state);
                }

                @Override
                public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                    outRect.set(0, 0, 0, 0);
                }
            });
        }
        {
            LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this.getActivity(), LinearLayoutManager.HORIZONTAL, false);
            rv_self.setLayoutManager(linearLayoutManager);
            rv_self.addItemDecoration(new RecyclerView.ItemDecoration() {
                @Override
                public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
                    super.onDraw(c, parent, state);
                }

                @Override
                public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                    outRect.set(0, 0,0, 0);
                }
            });
        }
        {
            LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this.getActivity(), LinearLayoutManager.HORIZONTAL, false);
            rv_none.setLayoutManager(linearLayoutManager);
            rv_none.addItemDecoration(new RecyclerView.ItemDecoration() {
                @Override
                public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
                    super.onDraw(c, parent, state);
                }

                @Override
                public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                    outRect.set(0, 0, 0, 0);
                }
            });
        }

        String language     = getResources().getConfiguration().locale.getLanguage();
        String saveLanguage = PreferenceModel.getString(LanguageUtil.class.getSimpleName(), language);
        boolean isChinese   = saveLanguage.equals(Locale.CHINESE.getLanguage());
//        uniLogoInfos.add(new LogoInfo(isChinese?R.drawable.setting_ic_logoblack_zh:R.drawable.setting_ic_logoblack_en, "pano_logo_black_zh.png", logoFileName.equals("pano_logo_black_zh.png")));
        uniLogoInfos.add(new LogoInfo(isChinese?R.drawable.logo_black_bg_zh:R.drawable.logo_black_bg_en,
                "logo_black_bg_zh.png", logoFileName.equals("logo_black_bg_en.png")));
        uniLogoInfos.add(new LogoInfo(isChinese?R.drawable.logo_white_bg_zh:R.drawable.logo_white_bg_en,
                "logo_white_bg_zh.png", logoFileName.equals("logo_white_bg_en.png")));
//        uniLogoInfos.add(new LogoInfo(R.drawable.logo_uu360, "logo_uu360.png",
//                logoFileName.equals("logo_uu360.png")));

        uniLogoInfos.add(new LogoInfo(R.drawable.logo_lp360, "logo_lp360.png",
                logoFileName.equals("logo_lp360.png")));

//        uniLogoInfos.add(new LogoInfo(R.mipmap.ico_laucher_plus_dot, "ico_laucher_plus_dot.png", logoFileName.equals("ico_laucher_plus_dot.png")));

        uniLogoAdapter = new UniLogoAdapter();
        rv_unipano.setAdapter(uniLogoAdapter);
        uniLogoAdapter.notifyDataSetChanged();

        selfLogoInfos.add(new LogoInfo(R.drawable.ic_add_logo, "", false, true));
        File mediaDir = new File(PathConfig.getSelfLogoDir());
        mediaDir.listFiles(new FileFilter() {
            @Override
            public boolean accept(File pathname) {
                String name = pathname.getName();
                if (name.endsWith(".jpg") || name.endsWith(".jpeg")) {
                    selfLogoInfos.add(new LogoInfo(R.drawable.ic_add_logo, pathname.getAbsolutePath(), logoFileName.equals(pathname.getAbsolutePath()), false));
                    return true;
                }
                return false;
            }
        });
        selfLogoAdapter = new SelfLogoAdapter();
        rv_self.setAdapter(selfLogoAdapter);
        selfLogoAdapter.notifyDataSetChanged();

        noneLogoInfos.add(new LogoInfo(R.drawable.logo_black,
                "logo_black.png",
                logoFileName.equals("logo_black.png")));
        noneLogoInfos.add(new LogoInfo(R.drawable.logo_white,
                "logo_white.png",
                logoFileName.equals("logo_white.png")));
        noneLogoInfos.add(new LogoInfo(isChinese?R.drawable.logo_none_zh :
                R.drawable.logo_none_en,
                "logo_none.png",
                logoFileName.equals("logo_none.png")));
        noneLogoAdapter = new NoneLogoAdapter();
        rv_none.setAdapter(noneLogoAdapter);
        noneLogoAdapter.notifyDataSetChanged();

    }
    private CircularImageView circularImageView = null;
    class UniLogoAdapter extends RecyclerView.Adapter<UniLogoAdapter.UniLogoViewHolder> {
        @Override
        public UniLogoViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            UniLogoViewHolder holder = new UniLogoViewHolder(LayoutInflater.from(MainApplication.getInstance()).inflate(R.layout.item_logo, parent, false));
            return holder;
        }
        @Override
        public void onBindViewHolder(UniLogoViewHolder holder, int position) {
            LogoInfo logoInfo = uniLogoInfos.get(position);
            holder.riv_logo.setTag(logoInfo);
            holder.riv_logo.setImageResource(logoInfo.resId);
            if(logoInfo.isSelected) {
                holder.riv_logo.setBorderColor(getResources().getColor(R.color.C_FF_74_3A));
                circularImageView = holder.riv_logo;
            }else{
                holder.riv_logo.setBorderColor(getResources().getColor(R.color.C_B0_B0_B0));
            }
            holder.riv_logo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if(circularImageView != null) {
                        circularImageView.setBorderColor(getResources().getColor(R.color.C_B0_B0_B0));
                        LogoInfo roundLogoInfo = (LogoInfo) circularImageView.getTag();
                        roundLogoInfo.isSelected = false;
                    }else{
                        for (LogoInfo logoInfo1:uniLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                        for (LogoInfo logoInfo1:selfLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                        for (LogoInfo logoInfo1:noneLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                    }
                    LogoInfo logoInfo = (LogoInfo)v.getTag();
                    logoInfo.isSelected = true;
                    circularImageView = (CircularImageView)v;
                    circularImageView.setBorderColor(getResources().getColor(R.color.C_FF_74_3A));
                    piv_play.doLoadLogoImage(LogoDialogFragment.this.getActivity().getAssets(), logoInfo.logoFileName);
                    PreferenceModel.putString("logoFileName", logoInfo.logoFileName);
                    logoFileName = logoInfo.logoFileName;
                }
            });
        }
        @Override
        public int getItemCount() {
            return uniLogoInfos.size();
        }
        class UniLogoViewHolder extends RecyclerView.ViewHolder {
            View view;
            CircularImageView riv_logo;
            public UniLogoViewHolder(View view) {
                super(view);
                this.view = view;
                riv_logo = (CircularImageView)view.findViewById(R.id.riv_logo);
            }
        }
    }
    class SelfLogoAdapter extends RecyclerView.Adapter<SelfLogoAdapter.SelfLogoViewHolder> {
        @Override
        public SelfLogoViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            SelfLogoViewHolder holder = new SelfLogoViewHolder(LayoutInflater.from(MainApplication.getInstance()).inflate(R.layout.item_logo, parent, false));
            return holder;
        }
        @Override
        public void onBindViewHolder(SelfLogoViewHolder holder, int position) {
            LogoInfo logoInfo = selfLogoInfos.get(position);
            holder.riv_logo.setTag(logoInfo);
            if(logoInfo.isPlusBtn) {
                holder.riv_logo.setImageResource(logoInfo.resId);
            }else{
                holder.riv_logo.setImageBitmap(BitmapFactory.decodeFile(logoInfo.logoFileName));
            }
            if(logoInfo.isSelected) {
                holder.riv_logo.setBorderColor(getResources().getColor(R.color.C_FF_74_3A));
                circularImageView = holder.riv_logo;
            }else{
                holder.riv_logo.setBorderColor(getResources().getColor(R.color.C_B0_B0_B0));
            }
            holder.riv_logo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    LogoInfo logoInfo = (LogoInfo)v.getTag();
                    if (logoInfo!=null && logoInfo.isPlusBtn){
                        pickFromGallery();
                        ib_delete.setVisibility(View.GONE);
                        return;
                    }
                    ib_delete.setVisibility(View.VISIBLE);
                    if(circularImageView != null) {
                        circularImageView.setBorderColor(getResources().getColor(R.color.C_B0_B0_B0));
                        LogoInfo roundLogoInfo = (LogoInfo) circularImageView.getTag();
                        roundLogoInfo.isSelected = false;
                    }else{
                        for (LogoInfo logoInfo1:uniLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                        for (LogoInfo logoInfo1:selfLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                        for (LogoInfo logoInfo1:noneLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                    }
                    logoInfo.isSelected = true;
                    circularImageView = (CircularImageView)v;
                    circularImageView.setBorderColor(getResources().getColor(R.color.C_FF_74_3A));
                    piv_play.doLoadLogoImage(null, logoInfo.logoFileName);
                    PreferenceModel.putString("logoFileName", logoInfo.logoFileName);
                    logoFileName = logoInfo.logoFileName;
                }
            });
        }
        @Override
        public int getItemCount() {
            return selfLogoInfos.size();
        }
        class SelfLogoViewHolder extends RecyclerView.ViewHolder {
            View view;
            CircularImageView riv_logo;
            public SelfLogoViewHolder(View view) {
                super(view);
                this.view = view;
                riv_logo = (CircularImageView)view.findViewById(R.id.riv_logo);
            }
        }
    }

    class NoneLogoAdapter extends RecyclerView.Adapter<NoneLogoAdapter.NoneLogoViewHolder> {
        @Override
        public NoneLogoViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            NoneLogoViewHolder holder = new NoneLogoViewHolder(LayoutInflater.from(MainApplication.getInstance()).inflate(R.layout.item_logo, parent, false));
            return holder;
        }
        @Override
        public void onBindViewHolder(NoneLogoViewHolder holder, int position) {
            LogoInfo logoInfo = noneLogoInfos.get(position);
            holder.riv_logo.setTag(logoInfo);
            holder.riv_logo.setImageResource(logoInfo.resId);
            if(logoInfo.isSelected) {
                holder.riv_logo.setBorderColor(getResources().getColor(R.color.C_FF_74_3A));
                circularImageView = holder.riv_logo;
            }else{
                holder.riv_logo.setBorderColor(getResources().getColor(R.color.C_B0_B0_B0));
            }
            holder.riv_logo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if(circularImageView != null) {
                        circularImageView.setBorderColor(getResources().getColor(R.color.C_B0_B0_B0));
                        LogoInfo roundLogoInfo = (LogoInfo) circularImageView.getTag();
                        roundLogoInfo.isSelected = false;
                    }else{
                        for (LogoInfo logoInfo1:uniLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                        for (LogoInfo logoInfo1:selfLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                        for (LogoInfo logoInfo1:noneLogoInfos){
                            logoInfo1.isSelected = false;
                        }
                    }
                    LogoInfo logoInfo = (LogoInfo)v.getTag();
                    logoInfo.isSelected = true;
                    circularImageView = (CircularImageView)v;
                    circularImageView.setBorderColor(getResources().getColor(R.color.C_FF_74_3A));
                    piv_play.doLoadLogoImage(LogoDialogFragment.this.getActivity().getAssets(), logoInfo.logoFileName);
                    PreferenceModel.putString("logoFileName", logoInfo.logoFileName);
                    logoFileName = logoInfo.logoFileName;
                }
            });
        }
        @Override
        public int getItemCount() {
            return noneLogoInfos.size();
        }
        class NoneLogoViewHolder extends RecyclerView.ViewHolder {
            View view;
            CircularImageView riv_logo;
            public NoneLogoViewHolder(View view) {
                super(view);
                this.view = view;
                riv_logo = (CircularImageView)view.findViewById(R.id.riv_logo);
            }
        }
    }

    private void handleCropResult(@NonNull Intent result) {
        String fileName = result.getStringExtra(UCrop.EXTRA_OUTPUT_URI);
        if (fileName != null && !fileName.equals("")) {
            for (LogoInfo logoInfo1:uniLogoInfos){
                logoInfo1.isSelected = false;
            }
            for (LogoInfo logoInfo1:selfLogoInfos){
                logoInfo1.isSelected = false;
            }
            for (LogoInfo logoInfo1:noneLogoInfos){
                logoInfo1.isSelected = false;
            }
            logoFileName = fileName;
            PreferenceModel.putString("logoFileName", logoFileName);
            piv_play.doLoadLogoImage(null, logoFileName);
            ib_delete.setVisibility(View.VISIBLE);
            selfLogoInfos.add(new LogoInfo(R.drawable.ic_plus_u, fileName, true, false));
            selfLogoAdapter.notifyItemRangeChanged(selfLogoInfos.size()-2, 2);
            rv_self.scrollToPosition(selfLogoInfos.size()-1);
        }
    }

    private void pickFromGallery() {
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN // Permission was added in API Level 16
//                && ActivityCompat.checkSelfPermission(this.getActivity(), Manifest.permission.READ_EXTERNAL_STORAGE)
//                != PackageManager.PERMISSION_GRANTED) {
//            this.getActivity().requestPermission(Manifest.permission.READ_EXTERNAL_STORAGE,
//                    getString(R.string.permission_read_storage_rationale),
//                    REQUEST_STORAGE_READ_ACCESS_PERMISSION);
//        } else {
            Intent intent = new Intent();
            intent.setType("image/*");
            intent.setAction(Intent.ACTION_GET_CONTENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            startActivityForResult(Intent.createChooser(intent, getString(R.string.select_picture)), REQUEST_SELECT_PICTURE);
//        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        switch (requestCode) {
            case REQUEST_STORAGE_READ_ACCESS_PERMISSION:
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    pickFromGallery();
                }
                break;
            default:
                super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            if (requestCode == REQUEST_SELECT_PICTURE) {
                final Uri selectedUri = data.getData();
                if (selectedUri != null) {
                    startCropActivity(data.getData());
                } else {
                    CToast.showToast(R.string.not_select_picture);
                }
            } else if (requestCode == UCrop.REQUEST_CROP) {
                handleCropResult(data);
            }
        }
        if (resultCode == UCrop.RESULT_ERROR) {
            handleCropError(data);
        }
    }

    private void startCropActivity(@NonNull Uri uri) {
        UCrop uCrop = UCrop.of(uri, mDestinationUri);
        uCrop.withAspectRatio(1,1);
        UCrop.Options options = new UCrop.Options();
        options.setCompressionFormat(Bitmap.CompressFormat.JPEG);
        uCrop.withOptions(options);
        uCrop.withTargetActivity(UCropActivity.class);
        uCrop.start(this.getActivity(), this);
    }
    @SuppressWarnings("ThrowableResultOfMethodCallIgnored")
    private void handleCropError(@NonNull Intent result) {
        final Throwable cropError = UCrop.getError(result);
        if (cropError != null) {
            CToast.showToast(cropError.getMessage());
        }
    }
}
