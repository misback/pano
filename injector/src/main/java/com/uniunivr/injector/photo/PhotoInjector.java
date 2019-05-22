package com.uniunivr.injector.photo;

import android.os.Environment;
import android.util.Log;

import org.w3c.dom.Document;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Map;

import pixy.meta.Metadata;
import pixy.meta.MetadataType;
import pixy.meta.exif.Exif;
import pixy.meta.jpeg.JPEGMeta;
import pixy.meta.jpeg.JpegXMP;
import pixy.meta.xmp.XMP;
import pixy.string.XMLUtils;

/**
 * @描述：     @图片注入元数据操作类
 * @作者：     @terrence
 * @创建时间： @2016-12-25
 */
public class PhotoInjector {

    private final static String TAG = "PhotoInjector";

    private File mFileClear;
    private File mFileExif;
    private String mXmlString;
    private boolean showlog ;
    public PhotoInjector(InputStream is,boolean showlog){
        mXmlString      = InjectorUtils.readXmpFile(is);
        this.showlog    = showlog;
    }

    /**
     * 注入元数据
     */
    public final boolean injectorPhoto(File fileTarget, File desFile){
        try{
            File clear = removeMetadata(fileTarget);
            writeMetadata(clear, desFile);
            clearTempFiles();
            return true;
        }catch (IOException e){
            return false;
        }
    }

    /**
     * 移除元数据
     * @param file
     * @return
     * @throws IOException
     */
    private final File removeMetadata(File file) throws IOException{
        mFileClear           = new File(Environment.getExternalStorageDirectory(),
                "fileClear.jpg");
        FileInputStream is   = new FileInputStream(file);
        FileOutputStream os  = new FileOutputStream(mFileClear);
        Metadata.removeMetadata(is, os, MetadataType.IMAGE, MetadataType.XMP, MetadataType.JPG_JFIF);
        if(showlog){
            Log.v(TAG,"metadata remove success");
        }
        return mFileClear;
    }

    /**
     * 写入元数据
     * @param fileTarget
     * @param destinyFile
     * @throws IOException
     */
    private final void writeMetadata(File fileTarget, File destinyFile) throws IOException {
        mFileExif = new File(Environment.getExternalStorageDirectory(), "fileExif.jpg");
        Map<MetadataType, Metadata> metadataMap = Metadata.readMetadata(fileTarget);
        /*insert exif*/
        Exif exif = InjectorUtils.createExif(metadataMap,showlog);
        FileInputStream in   = new FileInputStream(fileTarget);
        FileOutputStream out = new FileOutputStream(mFileExif);
        Metadata.insertExif(in, out, exif);
        in.close();
        out.close();
        if(showlog){
            Log.i(TAG, "metadata exif injected success");
        }
        /*insert xmp*/
        final XMP xmp           = new JpegXMP(mXmlString);
        final Document xmpDoc   = xmp.getXmpDocument();

        FileInputStream inXmp   = new FileInputStream(mFileExif);
        FileOutputStream outXmp = new FileOutputStream(destinyFile);

        if (!xmp.hasExtendedXmp()) {
            JPEGMeta.insertXMP(inXmp, outXmp, xmp);
        } else {
            Document extendedXmpDoc = xmp.getExtendedXmpDocument();
            JPEGMeta.insertXMP(inXmp, outXmp,
                    XMLUtils.serializeToString(xmpDoc.getDocumentElement(), "UTF-8"),
                    XMLUtils.serializeToString(extendedXmpDoc));
        }
        in.close();
        out.close();
        if(showlog){
            Log.v(TAG, "metadata exif injected success");
        }
    }

    private final void clearTempFiles() {
        mFileClear.delete();
        mFileExif.delete();
    }
}
