package com.uni.pano.share;

import android.app.Activity;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import com.umeng.socialize.bean.SHARE_MEDIA;
import com.uni.common.adapter.AbstractRecyclerAdapter;
import com.uni.common.adapter.CommonRecyclerHolder;
import com.uni.common.dialog.CDialog;
import com.uni.common.util.NetUtil;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.activities.ShareActivity;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.bean.ShareObject;
import com.uni.pano.decoration.GridItemDecoration;
import com.uni.pano.widget.CToast;
import com.uni.pano.widget.MaskImageView;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.OnClick;

/**
 * @描述：     @分享弹出框
 * @作者：     @蒋诗朋
 * @创建时间： @2017-04-25
 */

public class ShareWindow extends CDialog {

    @BindView(R.id.rv_share_list)
    RecyclerView mRecyclerView;

    private FBHelper mFBHelper;
    private MediaInfo mShareMediaInfo;

    private AbstractRecyclerAdapter<ShareObject> mRecyclerAdapter;

    public ShareWindow(Activity activity,FBHelper helper,
                       MediaInfo mediaInfo,DialogListener listener) {
        super(activity,listener,false,R.style.CDialog_Theme);
        this.mFBHelper       = helper;
        this.mShareMediaInfo = mediaInfo;
    }

    @Override
    public int getLayoutId() {
        return R.layout.view_share;
    }


    @OnClick({R.id.txt_cancel})
    public void onClick(View view){
        switch (view.getId()){
            case R.id.txt_cancel:
                cancel();
                break;
        }
    }

    @Override
    protected void initUI(View view) {
        final GridLayoutManager manager = new GridLayoutManager(getActivity(),3);
        mRecyclerView.setLayoutManager(manager);
        mRecyclerView.setHasFixedSize(true);
        mRecyclerView.addItemDecoration(new GridItemDecoration(getActivity()));
        bindData();
    }

    private final void bindData(){
        buildData();
        mRecyclerAdapter = new AbstractRecyclerAdapter<ShareObject>(getActivity(),
                buildData(),R.layout.item_share) {
            @Override
            public void bindViewData(CommonRecyclerHolder holder,
                                     final ShareObject object, int position, boolean isScrolling) {
                final MaskImageView imageView = holder.getView(R.id.iv_share_icon);
                if(null != object){
//                        holder.setImageResource(R.id.iv_share_icon,object.icon);
                    imageView.setImageResource(object.icon);
                    holder.setText(R.id.tv_share_text,object.text);
                }

                imageView.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        dismiss();
                        SHARE_MEDIA media = null;
                        switch (object.icon){
                            //微信
                            case R.drawable.ic_share_weixin:
                                media = SHARE_MEDIA.WEIXIN;
                                break;
                            //朋友圈
                            case R.drawable.ic_share_friends:
                                media = SHARE_MEDIA.WEIXIN_CIRCLE;
                                break;
                            //facebook
                            case R.drawable.ic_share_facebook:
                                media = SHARE_MEDIA.FACEBOOK;
                                break;
                            //qq
                            case R.drawable.ic_share_qq:
                                media = SHARE_MEDIA.QQ;
                                break;
                            //qq空间
                            case R.drawable.ic_share_qzone:
                                media = SHARE_MEDIA.QZONE;
                                break;
                        }

                        //检查分享平台是否可用
                        if (!ShareUtil.isSharingPlatformAvailable(getActivity(), media)) {
                            String platformName = ShareUtil.convPlatformName(media);
                            String notInstall   = ResourceUtil.getString(
                                    R.string.play_tips_not_install, platformName);
                            CToast.showToast(notInstall);
                            return;
                        }

                        //检查网络是否可用
                        if (NetUtil.isNetworkAvailable(getActivity())) {
                            if(media == SHARE_MEDIA.FACEBOOK){
                                mFBHelper.share(mShareMediaInfo.filePath);
                                return;
                            }
                            ShareActivity.startActivity(getActivity(), media, mShareMediaInfo);
                        }else{
                            CToast.showToast("网络连接失败，请设置网络连接！");
                        }
                    }
                });
            }

        };


        mRecyclerView.setAdapter(mRecyclerAdapter);
        mRecyclerAdapter.setOnItemClickListener(new AbstractRecyclerAdapter.OnItemClickListener<ShareObject>() {
            @Override
            public void onItemClick(CommonRecyclerHolder holder,
                                    RecyclerView parent, View view, ShareObject shareObject,int position) {
                dismiss();
                SHARE_MEDIA media = null;
                switch (shareObject.icon){
                    //微信
                    case R.drawable.ic_share_weixin:
                        media = SHARE_MEDIA.WEIXIN;
                        break;
                    //朋友圈
                    case R.drawable.ic_share_friends:
                        media = SHARE_MEDIA.WEIXIN_CIRCLE;
                        break;
                    //facebook
                    case R.drawable.ic_share_facebook:
                        media = SHARE_MEDIA.FACEBOOK;
                        break;
                    //qq
                    case R.drawable.ic_share_qq:
                        media = SHARE_MEDIA.QQ;
                        break;
                    //qq空间
                    case R.drawable.ic_share_qzone:
                        media = SHARE_MEDIA.QZONE;
                        break;
                }

                //检查分享平台是否可用
                if (!ShareUtil.isSharingPlatformAvailable(getActivity(), media)) {
                    String platformName = ShareUtil.convPlatformName(media);
                    String notInstall   = ResourceUtil.getString(
                            R.string.play_tips_not_install, platformName);
                    CToast.showToast(notInstall);
                    return;
                }

                //检查网络是否可用
                if (NetUtil.isNetworkAvailable(getActivity())) {
                    if(media == SHARE_MEDIA.FACEBOOK){
                        mFBHelper.share(mShareMediaInfo.filePath);
                        return;
                    }
                    ShareActivity.startActivity(getActivity(), media, mShareMediaInfo);
                }else{
                    CToast.showToast("网络连接失败，请设置网络连接！");
                }


            }
        });


    }

    private final String getText(int stringId){
        return getActivity().getResources().getString(stringId);
    }

    private final List<ShareObject> buildData(){
        final List<ShareObject> lists= new ArrayList<>();
        final ShareObject weixin     = new ShareObject(R.drawable.ic_share_weixin,getText(R.string.share_platform_wechat));
        final ShareObject wxcircle   = new ShareObject(R.drawable.ic_share_friends,getText(R.string.share_platform_wechat_circle));
        final ShareObject facebook   = new ShareObject(R.drawable.ic_share_facebook,getText(R.string.share_platform_facebook));
        final ShareObject qq         = new ShareObject(R.drawable.ic_share_qq,getText(R.string.share_platform_qq));
        final ShareObject qqzone     = new ShareObject(R.drawable.ic_share_qzone,getText(R.string.share_platform_qq_zone));
        lists.add(weixin);
        lists.add(wxcircle);
        lists.add(facebook);
        lists.add(qq);
        lists.add(qqzone);
        return lists;
    }

}
