#include <algorithm>
#include <numeric>
#include <map>
#include <vector>

#include "manager.h"
#include "camera.h"
#include "log.h"

using   meta_ptr    =   std::unique_ptr<ACameraMetadata, std::function<void(ACameraMetadata*)>>;
using   device_ptr  =   std::unique_ptr<ACameraDevice, std::function<void(ACameraDevice*)>>;

static std::map<long, Scene*> cameras;
static std::map<long, meta_ptr> metadata;
static std::map<long, device_ptr> devices;

CameraManager::CameraManager() {
    
    cameras.clear();
    metadata.clear();
    devices.clear();

    manager=ACameraManager_create();
    cameraMgrListener.context               = this;
    cameraMgrListener.onCameraAvailable     = [] (void* context, const char* id) {};
    cameraMgrListener.onCameraUnavailable   = [] (void* context, const char* id) {};

    cameraDeviceCallbacks.context           = this;
    cameraDeviceCallbacks.onDisconnected    = [] (void* context, ACameraDevice* device) {};
    cameraDeviceCallbacks.onError           = [] (void* context, ACameraDevice* device, int error) {};

    ACameraManager_registerAvailabilityCallback(manager, &cameraMgrListener);
    ACameraManager_getCameraIdList(manager, &cam_list);
}

CameraManager::~CameraManager() {
    
    for (auto [key, val]: cameras) if (val) delete val;

    ACameraManager_deleteCameraIdList(cam_list);
    ACameraManager_delete(manager);
}

int CameraManager::camera_count() const { return cam_list->numCameras; }

long CameraManager::camera_handle(
    const char* camera_id,
    const dims& screen,
    const char* ip
) {
    std::pair<const char*, ACameraMetadata*> info;
    std::pair<int, int>                      preview;            
    Scene*                                   camera   {nullptr};
    ACameraDevice*                           device   {nullptr};

    if      (strncmp(camera_id, "back", 4)==0)  {
        
        info    =   get(ACAMERA_LENS_FACING_BACK);
        preview =   compatible(AIMAGE_FORMAT_YUV_420_888, info.second, {screen.first, screen.second});
        CAMCALL(ACameraManager_openCamera(manager, info.first, &cameraDeviceCallbacks, &device));
        camera  =   new Camera(device, preview, sensor_orientation(info.second), framerate(info.second), ip);
    }
    else if (strncmp(camera_id, "front", 5)==0) {
        
        info    =   get(ACAMERA_LENS_FACING_FRONT);
        preview =   compatible(AIMAGE_FORMAT_YUV_420_888, info.second, {screen.first, screen.second});
        CAMCALL(ACameraManager_openCamera(manager, info.first, &cameraDeviceCallbacks, &device));
        camera  =   new Camera(device, preview, sensor_orientation(info.second), framerate(info.second), ip);
    }
    else return 0;

    long key = reinterpret_cast<long>(camera);

    cameras[key]=camera;
    if (!set_current(key)) LOGE("Coud not set current camera\n");

    metadata[key]   = {info.second, ACameraMetadata_free};
    devices[key]    = {device, ACameraDevice_close};

    LOGI("Create %s camera, id %d, internal id %s\n", camera_id, (uint32_t)key, info.first);
    return key;
}

bool CameraManager::set_current(long camera) {

    if (cameras.empty()) return false;
    if (cameras.find(camera) == cameras.end()) return false;
    current = cameras[camera];
    return true; 
}

std::pair<const char*, ACameraMetadata*> CameraManager::get(
    acamera_metadata_enum_acamera_lens_facing facing
) {
    const char*         id       {nullptr};      
    ACameraMetadata*    metadata {nullptr};
    for (int i=0; i<cam_list->numCameras; i++) {

        if (metadata) ACameraMetadata_free(metadata);
        CAMCALL(ACameraManager_getCameraCharacteristics(manager, cam_list->cameraIds[i], &metadata));
        ACameraMetadata_const_entry lens {0};
        CAMCALL(ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &lens));
        auto entry_facing = (acamera_metadata_enum_acamera_lens_facing) lens.data.u8[0];
        if ((entry_facing==facing) && (status==ACAMERA_OK)) id=cam_list->cameraIds[i]; break;
    }
    return {id, metadata};
}

range<int32_t> CameraManager::framerate(ACameraMetadata* metadata) {
  
    std::vector<range<int32_t>> fps;
    std::vector<range<int32_t>>::iterator result;
    ACameraMetadata_const_entry entry {0};
    CAMCALL(ACameraMetadata_getConstEntry(metadata, ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, &entry))
    fps.resize(entry.count/2);

    for(int i=0; i<entry.count; i+=2) {
        
        fps.push_back({entry.data.i32[i], entry.data.i32[i+1]});
        LOGI("FPS %d-%d", entry.data.i32[i], entry.data.i32[i+1])
    }
    result = std::max_element(fps.begin(), fps.end(), [] (const auto& p1, const auto& p2) { return p1.first<p2.first; });
    LOGI("Selected %d-%d", result->first, result->second)

    return *result;
}

dims CameraManager::compatible(
    AIMAGE_FORMATS format,
    ACameraMetadata* metadata,
    dims screen
) {
    std::vector<dims> sizes;
    ACameraMetadata_const_entry entry {0};
    CAMCALL(ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry));

    for (int i=0; i<entry.count; i+=4) {

        int32_t input = entry.data.i32[i+3];
        if (input) continue;

        if (entry.data.i32[i] == format) {

            sizes.push_back({entry.data.i32[i+1], entry.data.i32[i+2]});
            LOGI("AVALABLE CONFIGURATION, %d x %d", entry.data.i32[i+1], entry.data.i32[i+2])
        }
    }
        // Sorting by width from low to high
    std::sort(sizes.begin(), sizes.end(), 
        [] (const auto& w1, const auto& w2) { return w1.first<w2.first; });
    
    auto medium=std::lower_bound(sizes.crbegin(), sizes.crend(), screen.first,
        [] (const auto& size, const int& val) { return val<size.first; });

    LOGI("MEDIUM= %dx%d", medium->first, medium->second)
    return *medium;
}

int CameraManager::sensor_orientation(ACameraMetadata* metadata) {

    ACameraMetadata_const_entry entry = {0};
    CAMCALL(ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &entry))
    LOGI("ACAMERA_SENSOR_ORIENTATION, %d", entry.data.i32[0])
    return entry.data.i32[0];
}