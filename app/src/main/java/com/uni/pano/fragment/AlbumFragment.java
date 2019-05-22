package com.uni.pano.fragment;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.View;

import com.uni.common.adapter.AbstractRecyclerAdapter;
import com.uni.common.adapter.CommonRecyclerHolder;
import com.uni.common.config.PathConfig;
import com.uni.common.util.DateTimeUtil;
import com.uni.common.util.FileSizeUtil;
import com.uni.common.util.FileUtil;
import com.uni.pano.R;
import com.uni.pano.activities.PandaActivity;
import com.uni.pano.activities.PanoramaPreviewActivity;
import com.uni.pano.base.BaseFragment;
import com.uni.pano.bean.ArrayMediaInfo;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.decoration.TitleItemDecoration;
import com.uni.pano.event.CreateFileEvent;
import com.uni.pano.event.DeleteFileEvent;
import com.uni.pano.event.GalleryMediaInfoListUpdateEvent;
import com.uni.pano.utils.CommonUtil;
import com.uni.pano.widget.CRecyclerView;
import com.uni.pano.widget.CToast;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import butterknife.BindView;

/**
 * @描述：     @相册
 * @作者：     @蒋诗朋
 * @创建时间： @2017-05-10
 */
public class AlbumFragment extends BaseFragment{

    private static final String TAG = "AlbumFragment";

    @BindView(R.id.srl_gallery)
    SwipeRefreshLayout mSwipeRefresh;

    @BindView(R.id.rv_album)
    CRecyclerView mRecyclerView;

    @BindView(R.id.empty_view)
    View mEmptyView;

    private AbstractRecyclerAdapter<MediaInfo> mRecyclerAdapter;

    private List<MediaInfo> mediaInfoList = new ArrayList<>();

    @Override
    protected void initUI(@Nullable Bundle savedInstanceState) {
        setContentView(R.layout.fm_album);
        initView();
        bindData();
    }

    private final void initView(){
        //改变加载显示的颜色
        mSwipeRefresh.setColorSchemeColors(getResources().getColor(R.color.C_FF_74_3A));
        mRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity(),LinearLayoutManager.VERTICAL,false));
        mRecyclerView.setHasFixedSize(true);
        mRecyclerView.addItemDecoration(new RecyclerView.ItemDecoration() {
            @Override
            public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
                super.onDraw(c, parent, state);
            }
            @Override
            public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                outRect.set(0, 0, 0, getBottomMargin());
            }
        });
        mSwipeRefresh.setRefreshing(true);
        mSwipeRefresh.setOnRefreshListener(new SwipeRefreshLayout.OnRefreshListener() {
            @Override
            public void onRefresh() {
                mSwipeRefresh.setRefreshing(false);
//                if(mediaInfoList.size()>0){
//                    srl_gallery.setRefreshing(false);
//                }else {
//                    srl_gallery.setRefreshing(true);
//                    requestMediaInfoList();
//                }
            }
        });
    }

    private final void bindData(){
        mRecyclerAdapter = new AbstractRecyclerAdapter<MediaInfo>(getActivity(),
                mediaInfoList,R.layout.item_album) {
            @Override
            public void bindViewData(CommonRecyclerHolder holder,
                                     MediaInfo mediaInfo, int position, boolean isScrolling) {
                if(null != mediaInfo){
                    holder.setText(R.id.tv_size,FileSizeUtil.convertFileSize(mediaInfo.length, 2));
                }

                if(mediaInfo.selected){
                    holder.setVisibility(R.id.tv_media_check,true);
                    if(mediaInfo.checked){
                        holder.setImageResource(R.id.tv_media_check,R.drawable.chx_item_s);
                    }else{
                        holder.setImageResource(R.id.tv_media_check,R.drawable.chx_item_n);
                    }
                }else{
                    holder.setVisibility(R.id.tv_media_check,false);
                }

                switch (mediaInfo.type) {
                    case MP4:
                        holder.setVisibility(R.id.tv_record_time,true);
                        holder.setText(R.id.tv_record_time,DateTimeUtil.getFormatTime(mediaInfo.duration/1000));
                        holder.setVideoThumbUrl(R.id.iv_media_file,mediaInfo.filePath,R.drawable.bg_media_file_lp360);
                        break;
                    default:
                        holder.setVisibility(R.id.tv_record_time,false);
                        holder.setImageByUrl(R.id.iv_media_file,mediaInfo.filePath,R.drawable.bg_media_file_lp360);
                        break;
                }
            }

        };
        mRecyclerView.setEmptyView(mEmptyView,mSwipeRefresh);
        mRecyclerView.setAdapter(mRecyclerAdapter);
        mRecyclerAdapter.setOnItemClickListener(new AbstractRecyclerAdapter.OnItemClickListener<MediaInfo>() {
            @Override
            public void onItemClick(CommonRecyclerHolder holder,
                                    RecyclerView parent, View view, MediaInfo mediaInfo,int position) {
                final PandaActivity.Which which = getPandaOneActivity().getCheckStatus();
                switch (which){
                    case CHECKED:
                        PanoramaPreviewActivity.startActivity(getActivity(),
                                mediaInfoList.get(position),
                                new ArrayMediaInfo(mediaInfoList));
                        break;
                    case UNCHECKED:
                        if(mediaInfo.checked){
                            mediaInfo.checked = false;
                            holder.setImageResource(R.id.tv_media_check,
                                    R.drawable.chx_item_n);
                        }else{
                            mediaInfo.checked = true;
                            holder.setImageResource(R.id.tv_media_check,
                                    R.drawable.chx_item_s);
                        }
                        getPandaOneActivity().updateHeaderAndBottomInfo();
                        mRecyclerAdapter.notifyItemChanged(position);
                        break;
                    //全选取消状态
                    case CANCEL_SELECT_ALL:
                        if(mediaInfo.checked){
                            mediaInfo.checked = false;
                            holder.setImageResource(R.id.tv_media_check,
                                    R.drawable.chx_item_n);
                        }else{
                            mediaInfo.checked = true;
                            holder.setImageResource(R.id.tv_media_check,
                                    R.drawable.chx_item_s);
                        }
                        getPandaOneActivity().updateHeaderAndBottomInfo();
                        mRecyclerAdapter.notifyItemChanged(position);
                        getPandaOneActivity().setCheckStatus(PandaActivity.Which.CHECKED);
                        getPandaOneActivity().switchCheckStatus(false);
                        break;
                }
            }
        });

        mRecyclerAdapter.setOnItemLongClickListener(new AbstractRecyclerAdapter.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(RecyclerView parent, View view, int position) {
                getPandaOneActivity().switchCheckStatus(false);
                return true;
            }
        });
        requestMediaInfoList();
    }

    public final int getCheckedNumber(){
        int number = 0;
        for (MediaInfo mediaInfo:mediaInfoList){
            if(mediaInfo.checked){
                number ++ ;
                Log.v(TAG,"number --->" + number);
            }
        }
        return number;
    }

    /**
     * 判断文件列表是否还有文件
     * @return
     */
    public final boolean hasFile(){
        return mediaInfoList.size() > 0?true:false;
    }

    /**
     * 显示列表为可选状态
     */
    public final void showItemSelectable(boolean selected){
        for (MediaInfo mediaInfo:mediaInfoList){
            if(selected){
                mediaInfo.selected = true;
            }else{
                mediaInfo.selected = false;
                mediaInfo.checked  = false;
            }
        }
        mRecyclerAdapter.notifyDataSetChanged();
    }

    /**
     * 全选
     */
    public final void selectAll(){
        for (MediaInfo mediaInfo:mediaInfoList){
            mediaInfo.selected= true;
            mediaInfo.checked = true;
        }
        mRecyclerAdapter.notifyDataSetChanged();
    }

    /**
     * 删除
     * @return
     */
    public final void delete(){
        for (int i = 0; i < mediaInfoList.size();i ++){
            if(mediaInfoList.get(i).checked){
                mRecyclerAdapter.notifyItemRemoved(i);
            }
        }

        Iterator<MediaInfo> sListIterator = mediaInfoList.iterator();
        while(sListIterator.hasNext()){
            MediaInfo mediaInfo = sListIterator.next();
            if(mediaInfo.checked){
                FileUtil.deleteFile(mediaInfo.filePath);
                sListIterator.remove();
            }
        }

        CToast.showToast(R.string.delete_success);


    }


    /**
     * 取消全选
     */
    public final void cancelSelectAll(){
        for (MediaInfo mediaInfo:mediaInfoList){
            mediaInfo.selected= true;
            mediaInfo.checked = false;
        }
        mRecyclerAdapter.notifyDataSetChanged();
    }

    private final PandaActivity getPandaOneActivity(){
        return (PandaActivity) getActivity();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EventBus.getDefault().register(this);
    }

    @Override
    public void onDestroy() {
        EventBus.getDefault().unregister(this);
        super.onDestroy();
    }


    private final int getBottomMargin(){
        return (int) getActivity().getResources().
                getDimension(R.dimen.dp_8);
    }

    private final void requestMediaInfoList(){
        mSwipeRefresh.setRefreshing(true);
        ExecutorService singleThreadExecutor = Executors.newSingleThreadExecutor();
        singleThreadExecutor.execute(new Runnable() {
            @Override
            public void run() {
                CommonUtil.listMediaInfo(PathConfig.getMediaFolder(), mediaInfoList);
                EventBus.getDefault().post(new GalleryMediaInfoListUpdateEvent(mediaInfoList));
            }
        });
        singleThreadExecutor.shutdown();
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onGalleryMediaInfoListUpdateEvent(GalleryMediaInfoListUpdateEvent galleryMediaInfoListUpdateEvent) {
        mRecyclerView.addItemDecoration(new TitleItemDecoration(this.getActivity(), mediaInfoList));
        mRecyclerAdapter.notifyDataSetChanged();
        mSwipeRefresh.setRefreshing(false);

    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onCreateFile(CreateFileEvent createFileEvent) {
        String pathName = createFileEvent.fileName;
        File file = new File(pathName);
        MediaInfo mediaInfo = new MediaInfo(file);
        if(mediaInfo.valid) {
            mediaInfoList.add(0, mediaInfo);
            mRecyclerAdapter.notifyItemInserted(0);
        }
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onDeleteFile(DeleteFileEvent deleteFileEvent) {
        int index = 0;
        for (MediaInfo mediaInfo:mediaInfoList){
            if (mediaInfo.isSameRes(deleteFileEvent.mediaInfo)){
                mRecyclerAdapter.notifyItemRemoved(index);
                mediaInfoList.remove(index);
                break;
            }
            index++;
        }
    }

}
