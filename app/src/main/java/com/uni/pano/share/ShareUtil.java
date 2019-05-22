package com.uni.pano.share;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

import com.uni.common.util.NetUtil;
import com.uni.common.util.ResourceUtil;
import com.uni.pano.R;
import com.uni.pano.bean.MediaInfo;
import com.uni.pano.widget.CToast;
import com.umeng.socialize.bean.SHARE_MEDIA;

/**
 * 用于分享业务的工具类
 * Created by ZachLi on 2016/7/1.
 */
public class ShareUtil {

    /**
     * 分享平台转包名,仅关注需要使用的平台
     *
     * @param media
     * @return
     */
    public static String media2PackageName(SHARE_MEDIA media) {
        if (media == SHARE_MEDIA.FACEBOOK) {
            return "com.facebook.katana";
        } else if (media == SHARE_MEDIA.SINA) {
            return "com.sina.weibo";
        } else if (media == SHARE_MEDIA.TWITTER) {
            return "com.twitter.android";
        } else if (media == SHARE_MEDIA.QQ || media == SHARE_MEDIA.QZONE) {
            return "com.tencent.mobileqq";
        } else if (media == SHARE_MEDIA.WEIXIN || media == SHARE_MEDIA.WEIXIN_CIRCLE) {
            return "com.tencent.mm";
        } else {
            return "";
        }
    }

    /**
     * 分享平台转应用名,仅关注需要使用的平台
     *
     * @param media
     * @return
     */
    public static String convPlatformName(SHARE_MEDIA media) {

        if (media == SHARE_MEDIA.FACEBOOK) {
            return ResourceUtil.getString(R.string.share_platform_facebook);
        } else if (media == SHARE_MEDIA.SINA) {
            return ResourceUtil.getString(R.string.share_platform_sina_weibo);
        } else if (media == SHARE_MEDIA.QQ || media == SHARE_MEDIA.QZONE) {
            return ResourceUtil.getString(R.string.share_platform_qq);
        } else if (media == SHARE_MEDIA.WEIXIN || media == SHARE_MEDIA.WEIXIN_CIRCLE) {
            return ResourceUtil.getString(R.string.share_platform_wechat);
        } else if (media == SHARE_MEDIA.FACEBOOK) {
            return "facebook";
        } else if (media == SHARE_MEDIA.TWITTER) {
            return "twitter";
        } else {
            return "";
        }
    }



//    public static void openSharingPanel(final Activity activity,final FBHelper helper,final MediaInfo mediaInfo, boolean isFullScreen,
//                                        DialogInterface.OnDismissListener listener) {
//
//        ShareDialog.OnSharePanelItemClickListener sharePanelItemClickListener =
//                new ShareDialog.OnSharePanelItemClickListener() {
//
//                    @Override
//                    public void onSharePanelItemClick(Dialog dialog, final SHARE_MEDIA media) {
//                        dialog.dismiss();
//
//                        // PS: 新浪微博没有安装可以使用浏览器分享
//                        // 未安装平台处理
//                        if (!isSharingPlatformAvailable(activity, media)) {
//
//                            String platformName = ShareUtil.convPlatformName(media);
//                            String notInstall = ResourceUtil.getString(R.string.play_tips_not_install, platformName);
//                            CToast.showToast(notInstall);
//                            return;
//                        }
//                        if (NetUtil.isNetworkAvailable(activity)) {
//
//                            if(media == SHARE_MEDIA.FACEBOOK){
//                                helper.share(mediaInfo.filePath);
//                                return;
//                            }
//
//                            ShareActivity.startActivity(activity, media, mediaInfo);
//                        }else{
//                            CToast.showToast("网络连接失败，请设置网络连接！");
//                        }
//                    }
//                };
//
////        ShareDialog shareDialog = new ShareDialog(activity);
////        if(isFullScreen)
////            shareDialog.fullScreen();
////        shareDialog.setOnSharePanelItemClickListener(sharePanelItemClickListener);
////
////        if (listener != null) {
////            shareDialog.setOnDismissListener(listener);
////        }
////
////        shareDialog.show();
//
////        final ShareWindow window = new ShareWindow(activity,null);
////        window.show();
//
//
//    }

    /**
     * 检测应用是否安装
     * <p>通过包名去获取{@link PackageInfo},
     * 如果没有抛出{@link PackageManager.NameNotFoundException}且返回值不为 null 视为这个应用已安装</p>
     *
     * @param context
     * @param media
     * @return
     */
    private static boolean isInstall(Context context, SHARE_MEDIA media) {

        try {
            PackageManager pm = context.getPackageManager();
            String packageName = media2PackageName(media);
            PackageInfo packageInfo = pm.getPackageInfo(packageName, PackageManager.GET_ACTIVITIES);
            return packageInfo != null;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }


    /**
     * 检测分享平台是否可用
     *
     * @param context
     * @param media
     * @return
     */
    public static boolean isSharingPlatformAvailable(Context context, SHARE_MEDIA media) {

        if (media == SHARE_MEDIA.SINA)
            return true;
        else
            return isInstall(context, media);

    }

}
