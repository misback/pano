#include "Common.h"
#include "JniHelper.h"
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    JniHelper::setJavaVM(vm);
    env = JniHelper::getEnv();
 /*   jclass theClass = env->FindClass("com/uni/pano/MainApplication");
    if(!theClass){
        LOGE(LOG_TAG, "=============[x] failed to get android/app/Application");
    }else{
        LOGE(LOG_TAG, "=============[x] got android/app/Application = %p", theClass);
    }

    jmethodID method = env->GetStaticMethodID(theClass, "getInstance", "()com.uni.pano.MainApplication;");
    if(!method){LOGE(LOG_TAG, "============[x] failed to get constructor");}
    else{LOGE(LOG_TAG, "===============[x] got constructor = %ld",(long) method);}*/

  /*  jobject application = env->CallStaticObjectMethod(theClass, method);
    if(!application){
        LOGE(LOG_TAG, "=========[x] failed to get application");
    }else{
        LOGD(LOG_TAG, "=========[x] got application object");
    }

    method = env->GetMethodID(theClass,"getPackageName","()Ljava/lang/String;");
    if(!method){
        LOGE(LOG_TAG, "[x] failed to get getApplicationContext");
    }else{
        LOGD(LOG_TAG, "[x] got getApplicationContext = %ld",(long) method);
    }

    jstring pkgName = (jstring)env->CallObjectMethod(application, method);
    auto cpkgName = env->GetStringUTFChars(pkgName, JNI_FALSE);
    if(strcmp(cpkgName, "com.uni.pano") != 0){
        //exit(0);
        int * test= nullptr;
        delete test;
    }else{
        env->ReleaseStringUTFChars(pkgName, cpkgName);
    }*/
    return JNI_VERSION_1_6;
}