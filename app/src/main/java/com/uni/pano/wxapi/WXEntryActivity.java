package com.uni.pano.wxapi;


import com.tencent.mm.opensdk.modelbase.BaseResp;
import com.umeng.socialize.weixin.view.WXCallbackActivity;
import com.uni.pano.widget.CToast;

/**
 * Created by ntop on 15/9/4.
 */
public class WXEntryActivity extends WXCallbackActivity {

//
//    @Override
//    public void onResp(BaseResp resp) {
//        super.onResp(resp);
//
//        String result = "";
//
//        switch (resp.errCode) {
//            case BaseResp.ErrCode.ERR_OK:
//                result = "OK";
//                break;
//            case BaseResp.ErrCode.ERR_USER_CANCEL:
//                result = "cancel";
//                break;
//            case BaseResp.ErrCode.ERR_AUTH_DENIED:
//                result = "error";
//                break;
//            default:
//                result = "unknown";
//                break;
//        }
//
//        Toast.makeText(this, result, Toast.LENGTH_LONG).show();
//        finish();
//    }
}
