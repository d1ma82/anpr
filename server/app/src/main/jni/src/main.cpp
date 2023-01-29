///                 JNI interface between Java and C++

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <string>

#include "manager.h"
#include "assets.h"

static jobject _assets_manager{nullptr};

/// @brief                      Display that lib connected
/// @param env 
/// @param instance 
/// @param input_name           greet someone 
/// @return                     greeting string
extern "C" JNIEXPORT jstring JNICALL Java_com_home_anpr_Wrapper_greeting(
    JNIEnv* env,
    jobject instance,
    jstring input_name
) {
    const char* name = env->GetStringUTFChars(input_name, nullptr);
    std::string complete = "Hello " + (std::string) name + '!';
    env->ReleaseStringUTFChars(input_name, name);

    return env->NewStringUTF(complete.c_str());
}

/// @brief                          Creates a new camera
/// @param env 
/// @param instance 
/// @param asset_mng                Assets manager
/// @param width                    available width
/// @param height                   available height
/// @param camera_id_input          camera id can be, back or front
/// @return                         camera handle
extern "C" JNIEXPORT jlong JNICALL Java_com_home_anpr_Wrapper_createCamera(
    JNIEnv* env,
    jobject instance,
    jobject asset_mng,   
    jint width,
    jint height,
    jstring camera_id_input,
    jstring ip_input
) {
    asset_mng=env->NewGlobalRef(asset_mng);
    _assets_manager=asset_mng;
    asset::manager = new Asset(AAssetManager_fromJava(env, asset_mng));

    const char* id = env->GetStringUTFChars(camera_id_input, nullptr);
    std::string camera_id(id);
    env->ReleaseStringUTFChars(camera_id_input, id);

    const char* ip = env->GetStringUTFChars(ip_input, nullptr);
    std::string cam_ip(ip);
    env->ReleaseStringUTFChars(ip_input, ip);

    if (!camera::manager) camera::manager = new CameraManager();
    return camera::manager->camera_handle(camera_id.c_str(), {width, height}, cam_ip.c_str());
}

extern "C" JNIEXPORT jintArray JNICALL Java_com_home_anpr_Wrapper_characteristic(
    JNIEnv* env,
    jobject instance 
) {
    int size=0;
    const int32_t* arr= (*camera::manager)->characteristic(size);
    jintArray ret = env->NewIntArray(size);
    env->SetIntArrayRegion(ret, 0, size, arr);
    return ret;
}

extern "C" JNIEXPORT void JNICALL Java_com_home_anpr_Wrapper_shot(
    JNIEnv* env,
    jobject instance,
    jstring _path
) {
    const char* path = env->GetStringUTFChars(_path, nullptr);
    (*camera::manager)->take_shot(path);
    env->ReleaseStringUTFChars(_path, path);
}

extern "C" JNIEXPORT void JNICALL Java_com_home_anpr_Wrapper_onPreviewSurfaceCreated(
    JNIEnv* env,
    jobject instance,
    jobject surface      
) {
    (*camera::manager)->onSurfaceCreated(ANativeWindow_fromSurface(env, surface));
    (*camera::manager)->start_preview(true);
}

extern "C" JNIEXPORT void JNICALL Java_com_home_anpr_Wrapper_onPreviewSurfaceChanged(
    JNIEnv* env,
    jobject instance,
    jint width,
    jint height
) {
    (*camera::manager)->onSurfaceChanged({width, height});
}

extern "C" JNIEXPORT void JNICALL Java_com_home_anpr_Wrapper_onDraw(
    JNIEnv* env,
    jobject instance,
    jint orientation
) {
    (*camera::manager)->onDraw(orientation);
}

extern "C" JNIEXPORT void JNICALL Java_com_home_anpr_Wrapper_destroy(
    JNIEnv* env,
    jobject instance    
) {
    if(asset::manager) delete asset::manager;
    env->DeleteGlobalRef(_assets_manager);
    _assets_manager=nullptr;
    asset::manager=nullptr;

    if (camera::manager) delete camera::manager;
    camera::manager=nullptr;
}