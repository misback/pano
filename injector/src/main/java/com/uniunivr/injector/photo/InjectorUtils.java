package com.uniunivr.injector.photo;

import android.content.Context;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;

import pixy.image.tiff.FieldType;
import pixy.image.tiff.TiffTag;
import pixy.meta.Metadata;
import pixy.meta.MetadataType;
import pixy.meta.exif.Exif;
import pixy.meta.exif.ExifTag;
import pixy.meta.exif.JpegExif;

/**
 * @描述：     @图片注入工具类
 * @作者：     @蒋诗朋
 * @创建时间： @2016-12-25
 */
public class InjectorUtils {

    public static final String TAG = "InjectorUtils";

    /**
     * 读取xml元数据配置文件
     * @param is
     * @return
     */
    public static String readXmpFile(InputStream is){
        InputStreamReader reader = new InputStreamReader(is);
        BufferedReader buf = new BufferedReader(reader);
        StringBuilder xmlBuilder = new StringBuilder();
        try {
            String line;
            while ((line = buf.readLine()) != null) {
                xmlBuilder.append(line).append('\n');
            }
        } catch (IOException e) {
            return null;
        }
        return xmlBuilder.toString();
    }

    /**
     * 创建exif头文件信息
     * @param metadataMap
     * @return
     */
    public static Exif createExif(Map<MetadataType, Metadata> metadataMap,boolean showlog) {
        Exif exif = (Exif) metadataMap.get(MetadataType.EXIF);
        DateFormat dateFormat = new SimpleDateFormat("yyyy:MM:dd HH:mm:ss");
        Date date = new Date();
        if(exif == null){
            if(showlog){
                Log.v(TAG, "No exif found, creating new exif");
            }
            exif = new JpegExif();
        }
        exif.addExifField(ExifTag.DATE_TIME_ORIGINAL, FieldType.ASCII, dateFormat.format(date));
        exif.addExifField(ExifTag.DATE_TIME_DIGITIZED, FieldType.ASCII, dateFormat.format(date));
        exif.addImageField(TiffTag.SOFTWARE, FieldType.ASCII, "pedroSG94");
        exif.addImageField(TiffTag.MODEL, FieldType.ASCII, "pedroSG94");
        exif.addImageField(TiffTag.MAKE, FieldType.ASCII, "pedroSG94");
        return exif;
    }


}
