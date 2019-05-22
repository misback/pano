/*
 * Copyright (C) 2016 Nico(duyouhua1214@163.com), Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.uni.common.image;

import android.support.annotation.IntRange;
import android.support.annotation.NonNull;

/**
 * Glide加载图片时请求与ImageView显示尺寸相匹配的interface实现类
 * <br><br>
 * Created by duyouhua on 16/8/28.
 */

public class SmartImageSizeModelImp implements SmartImageSizeModel {

    String mUrl;

    /**
     * 构造
     *
     * @param url 原始图片url
     */
    public SmartImageSizeModelImp(@NonNull String url) {
        this.mUrl = url;
    }

    /**
     * 封装CDN裁剪图片的url
     *
     * @param width  ImageView的宽
     * @param height ImageView的高
     * @return 返回带CDN裁剪图片的url
     */
    @Override
    public String requestSmartSizeUrl(@IntRange(from = 1) int width, @IntRange(from = 1) int height) {
        StringBuffer url = new StringBuffer();
        int wPos = mUrl.indexOf("/w/");
        int hPos = mUrl.indexOf("/h/");
        if( mUrl.contains("qiniu.idooboo.com")&& wPos> 0&& hPos >0){
            url.append(mUrl.substring(0, wPos));
            url.append("/w/"+width);
            url.append("/h/"+height);
        } else if(mUrl.contains("bkt.clouddn.com")) {
            url.append(mUrl);
            url.append("?imageMogr2/thumbnail/");
            url.append(width+"x"+height);
        } else {
            url.append(mUrl);
        }
        return url.toString();
    }

}