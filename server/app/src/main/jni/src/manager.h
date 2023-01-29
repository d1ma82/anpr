#pragma once

#include "scene.h"

class CameraManager;

namespace camera {
    inline CameraManager* manager {nullptr};
}

class CameraManager {
    private:
        ACameraManager*      manager  {nullptr};
        ACameraIdList*       cam_list {nullptr};
        camera_status_t      status   {ACAMERA_OK};
        Scene*               current  {nullptr};

        ACameraManager_AvailabilityCallbacks cameraMgrListener;
        ACameraDevice_stateCallbacks cameraDeviceCallbacks;
        
        std::pair<const char*, ACameraMetadata*> get(acamera_metadata_enum_acamera_lens_facing);
        dims compatible(AIMAGE_FORMATS, ACameraMetadata*, dims screen);
        range<int32_t> framerate(ACameraMetadata*); 
        int sensor_orientation(ACameraMetadata*);
    public:
        CameraManager();
        ~CameraManager();

        int camera_count() const;
        long camera_handle(const char* camera_id, const dims&, const char* ip);
        bool set_current(long camera);
        
           // copy-move disabled
        CameraManager(const CameraManager&) = delete;
        CameraManager(CameraManager&&) = delete;
        CameraManager& operator=(const CameraManager&) = delete;
        CameraManager& operator=(CameraManager&&) = delete;

        Scene* operator->() const { return current; }  // TODO: Create default scene
};