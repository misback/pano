#include "CameraVideoRender.h"
#include<sys/types.h>
#include "JniHelper.h"
extern "C" {
    JNIEXPORT jlong JNICALL Java_com_uni_vr_CameraVideoRender_nativeOnCreate(JNIEnv* env, jclass clazz, jstring mediaDir, jstring tDetectModelPath, jstring tAlignModelPath, jint sphereVertexNum, jfloat sphereRadius, jobject context, jobject assetMgr){
        auto ctDetectModelPath = env->GetStringUTFChars(tDetectModelPath, JNI_FALSE);
        auto ctAlignModelPath = env->GetStringUTFChars(tAlignModelPath, JNI_FALSE);
        auto cameraVideoRender = new CameraVideoRender(ctDetectModelPath, ctAlignModelPath);
        JniHelper::setClassLoaderFrom(context);
        auto cMediaDir = env->GetStringUTFChars(mediaDir, JNI_FALSE);
        auto aAssetManager = AAssetManager_fromJava(env, assetMgr);
        cameraVideoRender->nativeOnCreate(cMediaDir, sphereVertexNum, sphereRadius, aAssetManager);
        env->ReleaseStringUTFChars(mediaDir, cMediaDir);
        env->ReleaseStringUTFChars(tDetectModelPath, ctDetectModelPath);
        env->ReleaseStringUTFChars(tAlignModelPath, ctAlignModelPath);
        return ((jlong)cameraVideoRender);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeInitGL(JNIEnv *env, jclass clazz, jlong objPtr, jint renderWidth, jint renderHeight, jint takePhotoWidth, jint takePhotoHeight, jobject jRenderCallback, jint nTimeScale, jint nFrameRate){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        jobject render_callback_obj         = env->NewGlobalRef(jRenderCallback);
        jmethodID callbackTakePhotoMethod   = env->GetMethodID(clazz, "callbackTakePhoto", "(ILjava/lang/String;)V");
        cameraVideoRender->nativeInitGL( renderWidth, renderHeight, takePhotoWidth, takePhotoHeight, render_callback_obj, callbackTakePhotoMethod, nTimeScale, nFrameRate);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeSetSurface(JNIEnv* env, jclass clazz, jlong objPtr, jobject surface){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        cameraVideoRender->nativeSetSurface(window);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeOnSurfaceChanged(JNIEnv* env, jclass clazz, jlong objPtr, jint pNewWidth, jint pNewHeight){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeOnSurfaceChanged(pNewWidth, pNewHeight);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeDrawFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeDrawFrame();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeDrawPanoramaFrame(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeDrawPanoramaFrame();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeOnResume(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeOnResume();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeOnPause(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeOnPause();
    }
    JNIEXPORT jint Java_com_uni_vr_CameraVideoRender_nativeGetTextureId(JNIEnv *env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return 0L;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        return cameraVideoRender->nativeGetTextureId();
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeCallBackFaceDetector(JNIEnv* env, jclass clazz, jlong objPtr, jint width, jint height, jfloat x, jfloat y, jfloat eyesDistance){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeCallBackFaceDetector(width, height, x, y, eyesDistance);
    }
    JNIEXPORT jint JNICALL Java_com_uni_vr_CameraVideoRender_nativeConnect(JNIEnv * env, jobject thiz, jlong objPtr, jint venderId, jint productId, jint fileDescriptor, jstring usbFs, jobject surface){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return 0L;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        auto window = surface == nullptr? nullptr:ANativeWindow_fromSurface(env, surface);
        const char *cusbFs        = env->GetStringUTFChars(usbFs, JNI_FALSE);
        int result  =    (int)cameraVideoRender->nativeConnect(venderId, productId, fileDescriptor, cusbFs, window);
        env->ReleaseStringUTFChars(usbFs, cusbFs);
        return result;
    }
    JNIEXPORT jint JNICALL Java_com_uni_vr_CameraVideoRender_nativeDisConnect(JNIEnv * env, jobject thiz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return 0L;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        return (int)cameraVideoRender->nativeDisConnect();
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeLoadLogoImage(JNIEnv* env, jclass clazz, jlong objPtr, jobject assetMgr, jstring filename){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        const char *filePath = env->GetStringUTFChars(filename, nullptr);
        LOGE("opening %s", filePath);
        if(strcmp(filePath,"") == 0){
            cameraVideoRender->nativeLoadLogoImage(nullptr, 0);
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
        cameraVideoRender->nativeLoadLogoImage(imageData, unpackedLen);
        env->ReleaseStringUTFChars(filename, filePath);
    }
    // shut down the native media system
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeOnDestroy(JNIEnv* env, jclass clazz, jlong objPtr){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        delete cameraVideoRender;
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeTakePhoto(JNIEnv * env, jobject thiz, jlong objPtr, jint tickCount){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->nativeTakePhoto(tickCount);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateLogoAngle(JNIEnv * env, jobject thiz, jlong objPtr, jfloat logoAngle){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->updateLogoAngle(logoAngle);
    }

    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateScale(JNIEnv * env, jobject thiz, jlong objPtr, jfloat scale){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->updateScale(scale);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateRotate(JNIEnv * env, jobject thiz, jlong objPtr, jfloat xMove, jfloat yMove){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->updateRotate(xMove, yMove);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateRotateFling(JNIEnv * env, jobject thiz, jlong objPtr, jfloat velocityX, jfloat velocityY){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->updateRotateFling(velocityX, velocityY);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateViewMode(JNIEnv * env, jobject thiz, jlong objPtr, jint viewMode){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        cameraVideoRender->updateViewMode((ViewMode)viewMode);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateLutFilter(JNIEnv * env, jobject thiz, jlong objPtr, jobject assetMgr, jstring jFilePath, jint filterType, jint useFaceDetector){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        const char *filePath = env->GetStringUTFChars(jFilePath, nullptr);
        if(assetMgr != nullptr){
             AAssetManager* mgr = AAssetManager_fromJava(env, assetMgr);
             cameraVideoRender->updateLutFilter(mgr, filePath, filterType, useFaceDetector);
        }
        env->ReleaseStringUTFChars(jFilePath, filePath);
    }
    JNIEXPORT void JNICALL Java_com_uni_vr_CameraVideoRender_nativeUpdateEffect(JNIEnv * env, jobject thiz, jlong objPtr, jstring effectFileName, jint useFaceDetector){
        if(objPtr == 0){
            LOGE("error objPtr = 0");
            return;
        }
        auto cameraVideoRender = LONG_TO_OBJ_PTR(CameraVideoRender, objPtr);
        const char *effectFilePath = env->GetStringUTFChars(effectFileName, nullptr);
        if(effectFilePath != nullptr){
             cameraVideoRender->updateEffect(effectFilePath, useFaceDetector);
             env->ReleaseStringUTFChars(effectFileName, effectFilePath);
        }
    }
}
