#include <vector>

#include "camera.h"
#include "basic_filters.h"
#include "ffmpeg_filter.h"
    
static std::vector<filter::Filter*> filters;
static const char* path = "data/shot.png";

static void onCaptureFailed(
    void* context, 
    ACameraCaptureSession* session, 
    ACaptureRequest* request, 
    ACameraCaptureFailure* failure
) {
    LOGI("onCaptureFailed %d", failure->reason)
}
static void onCaptureSequenceCompleted(
    void* context, 
    ACameraCaptureSession* session, 
    int sequenceId, 
    int64_t frameNumber
) {}
static void onCaptureSequenceAborted(
    void* context, 
    ACameraCaptureSession* session, 
    int sequenceId
) {}
static void onCaptureCompleted (
        void* context,
        ACameraCaptureSession* session,
        ACaptureRequest* request, 
        const ACameraMetadata* result
) {}

Camera::Camera(
     
    ACameraDevice*  device_in, 
    dims preview_in,
    int sensor_orientation_in,
    range<int32_t> fps_in,
    const char* ip_in

): device{device_in},  preview{preview_in}, 
    sensor_orientation{sensor_orientation_in}, fps{fps_in}, ip {ip_in},
        _characteristic{0, preview_in.first, preview_in.second}
{
    sessionStateCallbacks.context      =    this;
    sessionStateCallbacks.onActive     =    [] (void* context, ACameraCaptureSession *session) {};
    sessionStateCallbacks.onReady      =    [] (void* context, ACameraCaptureSession *session) {};
    sessionStateCallbacks.onClosed     =    [] (void* context, ACameraCaptureSession *session) {};

    captureCallbacks.context                    =this;
    captureCallbacks.onCaptureStarted           =nullptr;
    captureCallbacks.onCaptureProgressed        =nullptr;
    captureCallbacks.onCaptureCompleted         =onCaptureCompleted;
    captureCallbacks.onCaptureFailed            =onCaptureFailed;
    captureCallbacks.onCaptureSequenceCompleted =onCaptureSequenceCompleted;
    captureCallbacks.onCaptureSequenceAborted   =onCaptureSequenceAborted;
    captureCallbacks.onCaptureBufferLost        =nullptr;

    requests.resize(1);
    memset(requests.data(), 0, requests.size()*sizeof(RequestInfo));
    
    render=new Render(preview, sensor_orientation);
 
    filters.push_back(new filter::Camera(preview, render->get_input())); 
    filters.push_back(new filter::Rect((filter::Input*) filters[0], preview));
    filters.push_back(new filter::RTMP(*filters.rbegin(), preview, fps, ip.c_str()));
    filters.push_back(new filter::Frame(*filters.rbegin(), preview));

    render->attach_filters(filters);
    _characteristic[0] = render->get_input().texture;     // Need to sent camera texture to SurfaceTexture in android part
}

Camera::~Camera() {
    
    close_session();
    requests.resize(0);
    
    render->wait_finish();
    for (auto filter: filters) delete filter;
    filters.resize(0);

    delete render;
    
    LOGI("Camera destroyed")
}

void Camera::close_session() {

    if (!session_created) return;
    if (preview_started) start_preview(false);
    ACameraCaptureSession_abortCaptures(session);
    ACameraCaptureSession_close(session);
    
    session_created=false;
    LOGI("Session closed")    
}

void Camera::start_preview( bool toogle ) {

    if (toogle) {       // ON

        if (preview_started) return;
        ACaptureRequest* request = requests[PREVIEW_IDX].request.get();
        CAMCALL(ACameraCaptureSession_setRepeatingRequest(session, &captureCallbacks, 1, &request, nullptr))
        preview_started=status==ACAMERA_OK;
    } else {

        if (!preview_started) return;
        CAMCALL(ACameraCaptureSession_stopRepeating(session))
        LOGI("Preview stopped %d", status)
        preview_started=false;
    }
}

// Creating preview session
void Camera::onSurfaceCreated( ANativeWindow* window ) {

    if (!window) return;
    if (session_created) return;
    LOGI("Creating session")

    auto container = output_container { [this] () {

            ACaptureSessionOutputContainer* container{};
            CAMCALL(ACaptureSessionOutputContainer_create(&container))
            return container;
        } (), ACaptureSessionOutputContainer_free
    };

    requests[PREVIEW_IDX].req_template  = TEMPLATE_PREVIEW;
    requests[PREVIEW_IDX].window        = {window, ANativeWindow_release};

    for (auto &req: requests) {

        ANativeWindow_acquire(req.window.get());
        req.output = session_output {  [&req, this] () {

                    ACaptureSessionOutput* output {};
                    CAMCALL(ACaptureSessionOutput_create(req.window.get(), &output))
                    return output;
            } (), ACaptureSessionOutput_free
        };
        CAMCALL(ACaptureSessionOutputContainer_add(container.get(), req.output.get()));

        req.target = output_target { [&req, this] () {

                    ACameraOutputTarget* target {};
                    CAMCALL(ACameraOutputTarget_create(req.window.get(), &target))
                    return target;
            }(), ACameraOutputTarget_free
        };

        req.request = capture_request { [this] (ACameraDevice_request_template request_template) {

                ACaptureRequest* request {};
                CAMCALL(ACameraDevice_createCaptureRequest(device, request_template, &request))
                if (request_template == TEMPLATE_PREVIEW) {
                    CAMCALL(ACaptureRequest_setEntry_i32(request, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, &fps.first))
                }
                return request;
            } (req.req_template), ACaptureRequest_free
        };
        CAMCALL(ACaptureRequest_addTarget(req.request.get(), req.target.get()))        
    }
    CAMCALL(ACameraDevice_createCaptureSession(device, container.get(), &sessionStateCallbacks, &session))
    CAMCALL(ACaptureSessionOutputContainer_remove(container.get(), requests[PREVIEW_IDX].output.get()))
    session_created = status==ACAMERA_OK; 
    LOGI("Preview session created, %d", status)    
}