package com.uni.pano.share;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.facebook.AccessToken;
import com.facebook.AccessTokenTracker;
import com.facebook.CallbackManager;
import com.facebook.FacebookCallback;
import com.facebook.FacebookException;
import com.facebook.Profile;
import com.facebook.login.LoginManager;
import com.facebook.login.LoginResult;
import com.facebook.share.ShareApi;
import com.facebook.share.Sharer;
import com.facebook.share.model.SharePhoto;
import com.facebook.share.model.SharePhotoContent;
import com.facebook.share.model.ShareVideo;
import com.facebook.share.model.ShareVideoContent;
import com.facebook.share.widget.ShareDialog;
import com.uni.pano.R;
import com.uniunivr.injector.photo.PhotoInjector;
import com.uniunivr.injector.video.VideoInjector;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

/**
 * Created by terrence on 17-5-8.
 */

public class FBHelper {

    private static final String TAG            = "FBHelper";

    private static final String PERMISSION     = "publish_actions";

    private final List<String> mPermissions    = Arrays.asList("publish_actions");
    private CallbackManager mCallbackManager;

    private LoginManager mLoginManager;

    private boolean canPresentShareDialogWithPhotos;

    private boolean canPresentShareDialogWithVideos;

    private ShareDialog mSharePhotoDialog;

    private ShareDialog mShareVideoDialog;

    private FBLoginCallback mFBLoginCallback;

    private Activity mAcitivity;

    private String mFilePath  = null;

    private String mDesPath   = null;

//    private
    public FBHelper(Activity activity){
        mAcitivity            = activity;
        mCallbackManager      = CallbackManager.Factory.create();
        mLoginManager         = LoginManager.getInstance();
        mLoginManager.registerCallback(mCallbackManager,mLoginCallback);

        //token监听器
        new AccessTokenTracker() {
            @Override
            protected void onCurrentAccessTokenChanged(AccessToken oldAccessToken,
                                                       AccessToken currentAccessToken) {
                updateWithToken(currentAccessToken);
            }
        };

        //照片分享对话框
        mSharePhotoDialog     = new ShareDialog(mAcitivity);
        mSharePhotoDialog.registerCallback(
                mCallbackManager,
                mShareCallback);
        canPresentShareDialogWithPhotos = ShareDialog.canShow(
                SharePhotoContent.class);

        //视频分享对话框
        mShareVideoDialog     = new ShareDialog(mAcitivity);
        mShareVideoDialog.registerCallback(
                mCallbackManager,
                mShareCallback);
        canPresentShareDialogWithVideos = ShareDialog.canShow(
                ShareVideoContent.class);
    }

    private final void updateWithToken(AccessToken currentAccessToken) {
        if(null != AccessToken.getCurrentAccessToken()){
            Set<String> permissions = AccessToken.getCurrentAccessToken().getPermissions();
            if (currentAccessToken == null
                    || !currentAccessToken.getPermissions().contains(PERMISSION)) {
//            showRejectedPermissionError();
                return;
            }
        }
    }

    private final FacebookCallback<LoginResult> mLoginCallback = new FacebookCallback<LoginResult>() {
        @Override
        public void onSuccess(LoginResult loginResult) {
            Log.v(TAG,"登录成功---->" + mFilePath);
            if(!TextUtils.isEmpty(mFilePath)){
                share(mFilePath);
            }

            //登录成功回调
            if(null !=mFBLoginCallback){
                mFBLoginCallback.onSuccess(loginResult);
                mFBLoginCallback = null;
            }

        }

        @Override
        public void onCancel() {
            Log.v(TAG,"登录取消");
        }

        @Override
        public void onError(FacebookException error) {
            Log.v(TAG,"登录发生错误-->" + error.toString());

            if(null != mFBLoginCallback){
                mFBLoginCallback.onError(error);
                mFBLoginCallback = null;
            }
        }
    };

    /**
     * 分享回调
     */
    private FacebookCallback<Sharer.Result> mShareCallback = new FacebookCallback<Sharer.Result>() {
        @Override
        public void onCancel() {
            Log.v(TAG, "Canceled");
        }

        @Override
        public void onError(FacebookException error) {
            Log.v(TAG,"分享出错" + error.getMessage());
            showToast("分享失败");
            deleteFile();
        }

        @Override
        public void onSuccess(Sharer.Result result) {
            deleteFile();
            showToast("分享成功");
            Log.v(TAG, "Success!");
        }


    };

    private final void deleteFile(){
        mFilePath       = null;
        if(!TextUtils.isEmpty(mDesPath)){
            final File file = new File(mDesPath);
            if(file.exists()){
                file.delete();
                Log.v(TAG, "delete file " + mDesPath + " success !");
            }
        }
    }

    public final boolean isLoggedIn() {
        AccessToken accesstoken = AccessToken.getCurrentAccessToken();
        return !(accesstoken == null || accesstoken.getPermissions().isEmpty());
    }


    /**
     * 获取绑定账号用户名
     * @return
     */
    public final String getUserName(){
        Profile profile = Profile.getCurrentProfile();
        if (profile != null) {
            return profile.getLastName() + profile.getFirstName();
        }
        return null;
    }

    /**
     * 处理回调
     * @param requestCode
     * @param resultCode
     * @param data
     */
    public final void registerCallbackActivityResult(int requestCode, int resultCode, Intent data){
        mCallbackManager.onActivityResult(requestCode, resultCode, data);
    }

    //获取文件前缀
    private final String getFileSuffix(String fileName){
        final String suffix  = fileName.
                substring(0,fileName.lastIndexOf("."));
        return suffix;
    }

    /**
     * 获取文件后续
     * @param fileName
     * @return
     */
    private final String getExtension(String fileName){
        String extensionName  = fileName.substring(fileName.lastIndexOf(".")+1);
        return extensionName;
    }

    public final void share(String filePath){
        mFilePath                      = filePath;
        if(isLoggedIn()){
            final File   srcFile       = new File(filePath);
            final String extensionName = getExtension(srcFile.getName());
            showToast("开始上传文件到facebook");
            if("jpeg".equals(extensionName)
                    || "jpg".equals(extensionName)
                    || "png".equals(extensionName)){
                sharePhoto(filePath);
            }else if("mp4".equals(extensionName)){
                shareVideo(filePath);
            }
        }else{
            mLoginManager.logInWithPublishPermissions(mAcitivity,mPermissions);
        }
    }


    /**
     * 登录facebook
     * @param callback
     */
    public final void loginFacebook(FBLoginCallback callback){
        this.mFBLoginCallback = callback;
        mLoginManager.logInWithPublishPermissions(mAcitivity,mPermissions);
    }

    /**
     * 分享图片
     */
    private final void sharePhoto(String filePath){
        Log.v(TAG,"share photo 01");
        final File   srcFile = new File(filePath);
        final String desPath = filePath.substring(0,filePath.lastIndexOf(File.separator) + 1)
                + getFileSuffix(srcFile.getName()) + "_fb" + "." + getExtension(srcFile.getName());
        mDesPath             = desPath;
        final PhotoInjector photoInjector = new PhotoInjector(
                this.mAcitivity.getResources().openRawResource(R.raw.xmp),true);
        Log.v(TAG,"share photo 02");
        if(photoInjector.injectorPhoto(new File(filePath),new File(desPath))){
            Log.v(TAG,"share photo 03");
            final Uri uri    = Uri.parse("file://"+desPath);
            SharePhoto photo = new SharePhoto.Builder()
                    .setImageUrl(uri)
//                    .setParameter("allow_spherical_photo","true")
                    .setCaption("分享第一个全景照片")
                    .build();
            ArrayList<SharePhoto> photos = new ArrayList<>();
            photos.add(photo);

            SharePhotoContent content    = new SharePhotoContent.Builder()
                    .setPhotos(photos)
                    .build();

//            ShareApi.share(content,mShareCallback);

            if(canPresentShareDialogWithPhotos){
                mSharePhotoDialog.show(content);
            }
            Log.v(TAG,"share photo 04");
        }else{
            showToast("分享全景照片失败");
        }
    }

    /**
     * 分享全景视频
     */
    private final void shareVideo(String filePath){
        final File   srcFile = new File(filePath);
        if(srcFile.exists()){
            final String desPath = filePath.substring(0,filePath.lastIndexOf(File.separator) + 1)
                    + getFileSuffix(srcFile.getName()) + "_fb" + "." + getExtension(srcFile.getName());
            mDesPath             = desPath;
            final VideoInjector videoInjector = new VideoInjector();
            if(videoInjector.injectVideo(filePath,desPath)){
                final Uri uri    = Uri.parse("file://"+desPath);
                ShareVideo video = new ShareVideo.Builder()
                        .setLocalUrl(uri)
                        .build();

                ShareVideoContent videoContent  = new ShareVideoContent.Builder()
                        .setVideo(video)
                        .build();
                if(canPresentShareDialogWithVideos){
                    mShareVideoDialog.show(videoContent);
                }
            }else{
                showToast("分享全景视频失败");
            }
        }
    }

    private final void showToast(final String text){
        mAcitivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(mAcitivity,text,Toast.LENGTH_SHORT).show();
            }
        });
    }


    public static interface FBLoginCallback{
        /**
         * 登录成功
         * @param loginResult
         */
        public void onSuccess(LoginResult loginResult);

        /**
         * 登录发生错误
         * @param error
         */
        public void onError(FacebookException error);

    }
}
