#include "PanoramaCameraRender.h"
#include<sys/types.h>
#include "JniHelper.h"
extern "C" {
    JNIEXPORT jlong JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeOnCreate(JNIEnv* env, jclass clazz, jobject context, jstring mediaDir, jobject jPanoramaCallback){
        JniHelper::setClassLoaderFrom(context);
        auto cMediaDir = env->GetStringUTFChars(mediaDir, JNI_FALSE);
        jobject panorama_callback_obj       = env->NewGlobalRef(jPanoramaCallback);
        jmethodID callbackScreenShotMethod  = env->GetMethodID(clazz, "callback", "(Ljava/lang/String;I)V");
        jmethodID callbackSaveFilterMethod  = env->GetMethodID(clazz, "callbackAudioBuffer", "(Ljava/nio/ByteBuffer;I)V");
        auto panoramaCameraRender = new PanoramaCameraRender(cMediaDir, panorama_callback_obj, callbackScreenShotMethod, callbackSaveFilterMethod);
        env->ReleaseStringUTFChars(mediaDir, cMediaDir);
        return ((jlong)panoramaCameraRender);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeInitGL(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeInitGL();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeOnSurfaceChanged(JNIEnv* env, jclass clazz, jlong objPtr, jint pNewWidth, jint pNewHeight){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeOnSurfaceChanged(pNewWidth, pNewHeight);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeDrawFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeDrawFrame();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeDrawFilterFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeDrawFilterFrame();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeOnResume(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeOnResume();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeOnPause(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeOnPause();
    }
    JNIEXPORT jint Java_com_uni_vr_PanoramaCameraRender_nativeGetTextureId1(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return 0L;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        return panoramaCameraRender->nativeGetTextureId1();
    }

    JNIEXPORT jint Java_com_uni_vr_PanoramaCameraRender_nativeGetTextureId2(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return 0L;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        return panoramaCameraRender->nativeGetTextureId2();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeLoadVideo(JNIEnv* env, jclass clazz, jlong objPtr, jobject surface, jobject assetMgr, jstring filename, jint jwidth, jint jheight){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        // convert Java string to UTF-8
        const char *filePath = env->GetStringUTFChars(filename, nullptr);
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        LOGE("opening %s", filePath);
        if(assetMgr != nullptr){
            int fd;
            off_t outStart, outLen;
            fd = AAsset_openFileDescriptor(AAssetManager_open(AAssetManager_fromJava(env, assetMgr), filePath, 0),&outStart, &outLen);
            if (fd < 0) {
                LOGE("failed to open file: %s %d (%s)", filePath, fd, strerror(errno));
            }
            panoramaCameraRender->nativeLoadAssets(fd, outStart, outLen, window, jwidth, jheight);
        }else{
            panoramaCameraRender->nativeLoadSource(filePath, window, jwidth, jheight);
        }
        env->ReleaseStringUTFChars(filename, filePath);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeLoadLogoImage(JNIEnv* env, jclass clazz, jlong objPtr, jobject assetMgr, jstring filename){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        // convert Java string to UTF-8
        const char *filePath = env->GetStringUTFChars(filename, nullptr);
        LOGE("opening %s", filePath);
        if(strcmp(filePath,"") == 0){
            panoramaCameraRender->nativeLoadLogoImage(nullptr, 0);
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
        panoramaCameraRender->nativeLoadLogoImage(imageData, unpackedLen);
        env->ReleaseStringUTFChars(filename, filePath);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeSetOnProgressCallback(JNIEnv * env, jobject thiz, jlong objPtr, jobject jOnProgressCallback){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        jobject onProgress_callback_obj      = env->NewGlobalRef(jOnProgressCallback);
        if(onProgress_callback_obj != nullptr){
            jclass clazz                    = env->GetObjectClass(onProgress_callback_obj);
            jmethodID onProgress            = env->GetMethodID(clazz, "onProgress", "(J)V");
            panoramaCameraRender->setOnProgressCallback(onProgress_callback_obj, onProgress);
        }else{
            panoramaCameraRender->setOnProgressCallback(nullptr, 0);
        }
    }
    // set the playing state for the streaming media player
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeSetPlaying(JNIEnv* env,jclass clazz, jlong objPtr, jboolean isPlaying){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeSetPlaying(isPlaying);
    }
    // shut down the native media system
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeOnDestroy(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        delete panoramaCameraRender;
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeScreenShot(JNIEnv* env, jclass clazz, jlong objPtr, jstring jScreenDir){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        const char *screenDir = env->GetStringUTFChars(jScreenDir, nullptr);
        LOGE("screen Dir %s", screenDir);
        if(strcmp(screenDir,"") == 0){
            return;
        }
        panoramaCameraRender->nativeScreenShot(screenDir);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateLogoAngle(JNIEnv * env, jobject thiz, jlong objPtr, jfloat logoAngle){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateLogoAngle(logoAngle);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateProgress(JNIEnv *env, jclass clazz, jlong objPtr, jfloat progress){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeUpdateProgress(progress);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeRestart(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeRestart();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateScale(JNIEnv * env, jobject thiz, jlong objPtr, jfloat scale){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateScale(scale);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloat xMove, jfloat yMove){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateRotate(xMove, yMove);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateRotateFling(JNIEnv * env, jobject thiz, jlong objPtr, jfloat velocityX, jfloat velocityY){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateRotateFling(velocityX, velocityY);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateViewMode(JNIEnv * env, jobject thiz, jlong objPtr, jint viewMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateViewMode((ViewMode)viewMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateRenderMode(JNIEnv * env, jobject thiz, jlong objPtr, jint renderMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateRenderMode((RenderMode)renderMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateOptionMode(JNIEnv * env, jobject thiz, jlong objPtr, jint optionMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->updateOptionMode((OptionMode)optionMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateSensorRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloatArray sRotateMatArray){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        auto sRotateArr = env->GetFloatArrayElements(sRotateMatArray, nullptr);//得到一个指向原始数据类型内容的指针
        panoramaCameraRender->updateSensorRotate(glm::make_mat4(sRotateArr));
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeUpdateLutFilter(JNIEnv * env, jobject thiz, jlong objPtr, jobject assetMgr, jstring jFilePath, jint filterType){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        const char *filePath = env->GetStringUTFChars(jFilePath, nullptr);
        if(assetMgr != nullptr){
             auto mgr = AAssetManager_fromJava(env, assetMgr);
             panoramaCameraRender->updateLutFilter(mgr, filePath, filterType);
        }
        env->ReleaseStringUTFChars(jFilePath, filePath);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaCameraRender_nativeSetSaveFilter(JNIEnv * env, jobject thiz, jlong objPtr, jint saveFilter){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaCameraRender = LONG_TO_OBJ_PTR(PanoramaCameraRender, objPtr);
        panoramaCameraRender->nativeSetSaveFilter(saveFilter);
    }

}
