#include<sys/types.h>
#include "PanoramaImageRender.h"
#include "JniHelper.h"
extern "C" {
    JNIEXPORT jlong JNICALL Java_com_uni_vr_PanoramaImageRender_nativeOnCreate(JNIEnv* env, jclass clazz, jobject context, jstring mediaDir, jobject jPanoramaImageCallback){
        JniHelper::setClassLoaderFrom(context);
        auto cMediaDir = env->GetStringUTFChars(mediaDir, JNI_FALSE);
        jobject panorama_callback_obj       = env->NewGlobalRef(jPanoramaImageCallback);
        jmethodID callbackScreenShotMethod  = env->GetMethodID(clazz, "callback", "(Ljava/lang/String;I)V");
        auto panoramaImageRender = new PanoramaImageRender(cMediaDir, panorama_callback_obj, callbackScreenShotMethod);
        env->ReleaseStringUTFChars(mediaDir, cMediaDir);
        return ((jlong)panoramaImageRender);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeInitGL(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->nativeInitGL();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeOnSurfaceChanged(JNIEnv* env, jclass clazz, jlong objPtr, jint pNewWidth, jint pNewHeight){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->nativeOnSurfaceChanged(pNewWidth, pNewHeight);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeDrawFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->nativeDrawFrame();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeOnResume(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->nativeOnResume();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeOnPause(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->nativeOnPause();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeLoadImage(JNIEnv* env, jclass clazz, jlong objPtr, jobject assetMgr, jstring filename){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        // convert Java string to UTF-8
        const char *filePath = env->GetStringUTFChars(filename, nullptr);
        LOGE("opening %s", filePath);
        unsigned char* imageData = nullptr;
        ssize_t unpackedLen = 0;
        if(assetMgr != nullptr){
             AAssetManager* mgr                     = AAssetManager_fromJava(env, assetMgr);
             AAsset* asset 							= AAssetManager_open(mgr, filePath, AASSET_MODE_UNKNOWN);
             if(asset != nullptr){
                 off_t fileSize 				    = AAsset_getLength(asset);
                 imageData 			                = new unsigned char[fileSize];
                 unpackedLen  						= AAsset_read(asset, (void*)imageData, fileSize);
                 AAsset_close(asset);
             }else{
                LOGE("read_JPEG_file%s failed!", filePath);
                return;
             }
        }else{
            auto fp        = fopen(filePath, "rb");
            if (fp != nullptr) {
                fseek(fp,0,SEEK_END);
                unpackedLen = ftell(fp);
                imageData  =   new unsigned char[unpackedLen];
                fseek(fp,0,SEEK_SET);
                fread(imageData, 1, unpackedLen, fp);
                fflush(fp);
                fclose(fp);
            } else {
                LOGE("read_JPEG_file%s failed!", filePath);
                fclose(fp);
                return;
            }
        }
        panoramaImageRender->nativeLoadImage(imageData, unpackedLen);
        env->ReleaseStringUTFChars(filename, filePath);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeLoadLogoImage(JNIEnv* env, jclass clazz, jlong objPtr, jobject assetMgr, jstring filename){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        // convert Java string to UTF-8
        const char *filePath = env->GetStringUTFChars(filename, nullptr);
        LOGE("opening %s", filePath);
        if(strcmp(filePath,"") == 0){
            panoramaImageRender->nativeLoadLogoImage(nullptr, 0);
            return;
        }
        unsigned char* imageData = nullptr;
        ssize_t unpackedLen = 0;
        if(assetMgr != nullptr){
             AAssetManager* mgr                     = AAssetManager_fromJava(env, assetMgr);
             AAsset* asset 							= AAssetManager_open(mgr, filePath, AASSET_MODE_UNKNOWN);
             if(asset != nullptr){
                 off_t fileSize 				    = AAsset_getLength(asset);
                 imageData 			                = new unsigned char[fileSize];
                 unpackedLen  						= AAsset_read(asset, (void*)imageData, fileSize);
                 AAsset_close(asset);
             }else{
                LOGE("read_JPEG_file%s failed!", filePath);
                return;
             }
        }else{
            auto fp        = fopen(filePath, "rb");
            if (fp != nullptr) {
                fseek(fp,0,SEEK_END);
                unpackedLen = ftell(fp);
                imageData  =   new unsigned char[unpackedLen];
                fseek(fp,0,SEEK_SET);
                fread(imageData, 1, unpackedLen, fp);
                fflush(fp);
                fclose(fp);
            } else {
                LOGE("read_JPEG_file%s failed!", filePath);
                fclose(fp);
                return;
            }
        }
        panoramaImageRender->nativeLoadLogoImage(imageData, unpackedLen);
        env->ReleaseStringUTFChars(filename, filePath);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateLogoAngle(JNIEnv * env, jobject thiz, jlong objPtr, jfloat logoAngle){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateLogoAngle(logoAngle);
    }

    // shut down the native media system
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeOnDestroy(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        delete panoramaImageRender;
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeSaveFilterImage(JNIEnv* env, jclass clazz, jlong objPtr, jstring jdir){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        const char *dir = env->GetStringUTFChars(jdir, nullptr);
        LOGE("screen Dir %s", dir);
        if(strcmp(dir,"") == 0){
            return;
        }
        panoramaImageRender->nativeSaveFilterFrame(dir);
    }


    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeScreenShot(JNIEnv* env, jclass clazz, jlong objPtr, jstring jScreenDir){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        const char *screenDir = env->GetStringUTFChars(jScreenDir, nullptr);
        LOGE("screen Dir %s", screenDir);
        if(strcmp(screenDir,"") == 0){
            return;
        }
        panoramaImageRender->nativeScreenShot(screenDir);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateScale(JNIEnv * env, jobject thiz, jlong objPtr, jfloat scale){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateScale(scale);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloat xMove, jfloat yMove, jint notAnimation){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateRotate(xMove, yMove, notAnimation);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateRotateFling(JNIEnv * env, jobject thiz, jlong objPtr, jfloat velocityX, jfloat velocityY){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateRotateFling(velocityX, velocityY);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateViewMode(JNIEnv * env, jobject thiz, jlong objPtr, jint viewMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateViewMode((ViewMode)viewMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateRenderMode(JNIEnv * env, jobject thiz, jlong objPtr, jint renderMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateRenderMode((RenderMode)renderMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateOptionMode(JNIEnv * env, jobject thiz, jlong objPtr, jint optionMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        panoramaImageRender->updateOptionMode((OptionMode)optionMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateSensorRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloatArray sRotateMatArray){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        auto sRotateArr = env->GetFloatArrayElements(sRotateMatArray, nullptr);//得到一个指向原始数据类型内容的指针
        panoramaImageRender->updateSensorRotate(glm::make_mat4(sRotateArr));
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaImageRender_nativeUpdateLutFilter(JNIEnv * env, jobject thiz, jlong objPtr, jobject assetMgr, jstring jFilePath, jint filterType){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaImageRender = LONG_TO_OBJ_PTR(PanoramaImageRender, objPtr);
        const char *filePath = env->GetStringUTFChars(jFilePath, nullptr);
        if(assetMgr != nullptr){
             AAssetManager* mgr = AAssetManager_fromJava(env, assetMgr);
             panoramaImageRender->updateLutFilter(mgr, filePath, filterType);
        }
        env->ReleaseStringUTFChars(jFilePath, filePath);
    }    
}