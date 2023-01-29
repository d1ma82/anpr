#pragma once

#include <functional>

#include "scene.h"
#include "render.h"

using output_container =    std::unique_ptr<ACaptureSessionOutputContainer, std::function<void(ACaptureSessionOutputContainer*)>>;
using output_target    =    std::unique_ptr<ACameraOutputTarget, std::function<void(ACameraOutputTarget*)>>;
using capture_request  =    std::unique_ptr<ACaptureRequest, std::function<void(ACaptureRequest*)>>;
using native_window    =    std::unique_ptr<ANativeWindow, std::function<void(ANativeWindow*)>>;
using session_output   =    std::unique_ptr<ACaptureSessionOutput, std::function<void(ACaptureSessionOutput*)>>;

typedef struct {

    native_window window    {nullptr};
    session_output output   {nullptr};
    output_target target    {nullptr};
    capture_request request {nullptr};
    ACameraDevice_request_template req_template;
    int sequence;

} RequestInfo;

enum RequestIndex {PREVIEW_IDX, STILL_CAPTURE_IDX, REQUESTS_COUNT};

class Camera final: public Scene {
    public:
        Camera(ACameraDevice*, dims, int, range<int32_t>, const char* ip);
        ~Camera();
   // copy-move disabled
        Camera(const Camera&) = delete;
        Camera(Camera&&) = delete;
        Camera& operator=(const Camera&) = delete;
        Camera& operator=(Camera&&) = delete;


        const int32_t* characteristic(int& size) const { size=CHARACTERISTIC_COUNT; return _characteristic; }
        void take_shot(const char* path) { if (render) render->take_shot(path); }
        void start_preview(bool toogle);
        void onSurfaceCreated(ANativeWindow* window);
        void onSurfaceChanged(const dims& dim) { if (render) render->surface_chanded(dim); }
        void onDraw(int orientation) { if (render) render->render(orientation); }
    private:
        static const int CHARACTERISTIC_COUNT=3;

        int32_t                     _characteristic[CHARACTERISTIC_COUNT];
        int                         sensor_orientation;
        dims                        preview;
        range<int32_t>              fps;
        bool                        session_created {false};
        bool                        preview_started {false};
        camera_status_t             status          {ACAMERA_OK};
        ACameraDevice*              device          {nullptr};
        ACameraCaptureSession*      session         {nullptr};
        std::vector<RequestInfo>    requests;
        Render*                     render          {nullptr};
        std::string                 ip;

        ACameraCaptureSession_stateCallbacks sessionStateCallbacks;
        ACameraCaptureSession_captureCallbacks captureCallbacks;

        void close_session();
};  