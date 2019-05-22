#include "PanoramaVideoRender.h"
#include<sys/types.h>
#include "JniHelper.h"
extern "C" {
    JNIEXPORT jlong JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeOnCreate(JNIEnv* env, jclass clazz, jobject context, jstring mediaDir, jobject jPanoramaCallback){
        JniHelper::setClassLoaderFrom(context);
        auto cMediaDir = env->GetStringUTFChars(mediaDir, JNI_FALSE);
        jobject panorama_callback_obj       = env->NewGlobalRef(jPanoramaCallback);
        jmethodID callbackScreenShotMethod  = env->GetMethodID(clazz, "callback", "(Ljava/lang/String;I)V");
        jmethodID callbackSaveFilterMethod  = env->GetMethodID(clazz, "callbackAudioBuffer", "(Ljava/nio/ByteBuffer;I)V");
        auto panoramaVideoRender = new PanoramaVideoRender(cMediaDir, panorama_callback_obj, callbackScreenShotMethod, callbackSaveFilterMethod);
        env->ReleaseStringUTFChars(mediaDir, cMediaDir);
        return ((jlong)panoramaVideoRender);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeInitGL(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeInitGL();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeOnSurfaceChanged(JNIEnv* env, jclass clazz, jlong objPtr, jint pNewWidth, jint pNewHeight){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeOnSurfaceChanged(pNewWidth, pNewHeight);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeDrawFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeDrawFrame();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeDrawFilterFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeDrawFilterFrame();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeOnResume(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeOnResume();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeOnPause(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeOnPause();
    }
    JNIEXPORT jint Java_com_uni_vr_PanoramaVideoRender_nativeGetTextureId(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return 0L;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        return panoramaVideoRender->nativeGetTextureId();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeLoadVideo(JNIEnv* env, jclass clazz, jlong objPtr, jobject surface, jobject assetMgr, jstring filename, jint jwidth, jint jheight){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
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
            panoramaVideoRender->nativeLoadAssets(fd, outStart, outLen, window, jwidth, jheight);
        }else{
            panoramaVideoRender->nativeLoadSource(filePath, window, jwidth, jheight);
        }
        env->ReleaseStringUTFChars(filename, filePath);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeLoadLogoImage(JNIEnv* env, jclass clazz, jlong objPtr, jobject assetMgr, jstring filename){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        // convert Java string to UTF-8
        const char *filePath = env->GetStringUTFChars(filename, nullptr);
        LOGE("opening %s", filePath);
        if(strcmp(filePath,"") == 0){
            panoramaVideoRender->nativeLoadLogoImage(nullptr, 0);
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
        panoramaVideoRender->nativeLoadLogoImage(imageData, unpackedLen);
        env->ReleaseStringUTFChars(filename, filePath);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeSetOnProgressCallback(JNIEnv * env, jobject thiz, jlong objPtr, jobject jOnProgressCallback){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        jobject onProgress_callback_obj      = env->NewGlobalRef(jOnProgressCallback);
        if(onProgress_callback_obj != nullptr){
            jclass clazz                    = env->GetObjectClass(onProgress_callback_obj);
            jmethodID onProgress            = env->GetMethodID(clazz, "onProgress", "(J)V");
            panoramaVideoRender->setOnProgressCallback(onProgress_callback_obj, onProgress);
        }else{
            panoramaVideoRender->setOnProgressCallback(nullptr, 0);
        }
    }
    // set the playing state for the streaming media player
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeSetPlaying(JNIEnv* env,jclass clazz, jlong objPtr, jboolean isPlaying){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeSetPlaying(isPlaying);
    }
    // shut down the native media system
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeOnDestroy(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        delete panoramaVideoRender;
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeScreenShot(JNIEnv* env, jclass clazz, jlong objPtr, jstring jScreenDir){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        const char *screenDir = env->GetStringUTFChars(jScreenDir, nullptr);
        LOGE("screen Dir %s", screenDir);
        if(strcmp(screenDir,"") == 0){
            return;
        }
        panoramaVideoRender->nativeScreenShot(screenDir);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateLogoAngle(JNIEnv * env, jobject thiz, jlong objPtr, jfloat logoAngle){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateLogoAngle(logoAngle);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateProgress(JNIEnv *env, jclass clazz, jlong objPtr, jfloat progress){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeUpdateProgress(progress);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeRestart(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeRestart();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateScale(JNIEnv * env, jobject thiz, jlong objPtr, jfloat scale){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateScale(scale);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloat xMove, jfloat yMove){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateRotate(xMove, yMove);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateRotateFling(JNIEnv * env, jobject thiz, jlong objPtr, jfloat velocityX, jfloat velocityY){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateRotateFling(velocityX, velocityY);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateViewMode(JNIEnv * env, jobject thiz, jlong objPtr, jint viewMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateViewMode((ViewMode)viewMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateRenderMode(JNIEnv * env, jobject thiz, jlong objPtr, jint renderMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateRenderMode((RenderMode)renderMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateOptionMode(JNIEnv * env, jobject thiz, jlong objPtr, jint optionMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->updateOptionMode((OptionMode)optionMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateSensorRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloatArray sRotateMatArray){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        auto sRotateArr = env->GetFloatArrayElements(sRotateMatArray, nullptr);//得到一个指向原始数据类型内容的指针
        panoramaVideoRender->updateSensorRotate(glm::make_mat4(sRotateArr));
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeUpdateLutFilter(JNIEnv * env, jobject thiz, jlong objPtr, jobject assetMgr, jstring jFilePath, jint filterType){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        const char *filePath = env->GetStringUTFChars(jFilePath, nullptr);
        if(assetMgr != nullptr){
             auto mgr = AAssetManager_fromJava(env, assetMgr);
             panoramaVideoRender->updateLutFilter(mgr, filePath, filterType);
        }
        env->ReleaseStringUTFChars(jFilePath, filePath);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_PanoramaVideoRender_nativeSetSaveFilter(JNIEnv * env, jobject thiz, jlong objPtr, jint saveFilter){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto panoramaVideoRender = LONG_TO_OBJ_PTR(PanoramaVideoRender, objPtr);
        panoramaVideoRender->nativeSetSaveFilter(saveFilter);
    }

}
