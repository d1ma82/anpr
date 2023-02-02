#include <thread>

#include "streamer.h"
#include "rtmp/server.h"

using PicturePtr = std::unique_ptr<Picture>;
using ServerPtr  = std::unique_ptr<rtmp::Server>;

static bool thread_start {false};
static bool frame_available {false};
static int  orientation {0};

static ServerPtr    srv;
static Scaler       scaler;
static PicturePtr   pic;
static cv::Mat*     img     {nullptr};

static void init(
    dims src, 
    range<int32_t> fps, 
    uint32_t bitrate, 
    dims dst, 
    const char* url
) {
    LOGI("Start server %s; src= %dx%d fps= %d; dst= %dx%d\n",url, src.first, src.second, fps.first, dst.first, dst.second);
    av_log_set_level(AV_LOG_INFO);

    srv = std::make_unique<rtmp::Server>(url);

    scaler = Scaler{[] (dims src, enum AVPixelFormat srcFormat,
                        dims dst, enum AVPixelFormat dstFormat, int flags
    )   {
            SwsContext* ctx  = sws_getContext(src.first, src.second, srcFormat, dst.first, dst.second, dstFormat, flags, nullptr, nullptr, nullptr);
            if (!ctx) LOGE("Could not init sample scaler\n")
            return ctx;

        } (src, AV_PIX_FMT_RGBA, dst, srv->out_pixel_fmt, SWS_BILINEAR), sws_freeContext
    };

    pic = std::make_unique<Picture>(AV_PIX_FMT_YUV420P, dst.first, dst.second);

    LOGI("Listen incomings\n")
    if (!srv->listen()) {

        thread_start=false;
        LOGI("Connection timeout\n")
        return;
    }

    LOGI("Begin stream\n")
    srv->begin_stream(dst.first, dst.second, fps.first, bitrate, orientation);
    srv->start_time=srv->clk.now();
}

static void loop(dims src) {

    while (thread_start) {
        // stream frames
        if (!frame_available) continue;
        
        const int stride[] {static_cast<int>(img->step1())};
        int ret=sws_scale(scaler.get(), &img->data, stride, 0, src.second, (*pic)->data, (*pic)->linesize);
        if (!srv->encode_and_write_frame(*pic)) break;
        // proccess frames
        frame_available = false;
    }
    LOGI("Stream total: %f sec\n", (*pic)->pts*av_q2d(srv->out_codec_ctx->time_base))
    thread_start=false;
}

static void clear() {

    scaler.reset(nullptr);
    pic.reset(nullptr);
    srv.reset(nullptr);
    img = nullptr;
}

static void run(
    dims src, 
    range<int32_t> fps, 
    int bitrate,  
    dims dst, 
    const char* url
) {
    try {
        init(src, fps, bitrate, dst, url);
        loop(src);
        clear();
    } catch (std::exception& e) {
        LOGI("%s", e.what())
    }
}

filter::RTMP::RTMP(
        Filter* filter_in, 
        const range<int32_t>& fps_in,
        const char* ip
): input{filter_in}, fps{fps_in} {
        
    thread_start=true;
    server  = "rtmp://"+std::string(ip)+"/live/mystream/";
    img     = &frame_clone;

    if (srv==nullptr){
        std::thread rtmp_server_handler(run, input->get_viewport(), fps, bitrate, dims{1280, 720}, server.c_str());
        rtmp_server_handler.detach();
    }
}

void filter::RTMP::apply(int new_orientation) {

    if (!frame_available) {

        frame().getMat(cv::ACCESS_READ).copyTo(frame_clone);
        orientation = new_orientation;
        frame_available = true;
    }
}

filter::RTMP::~RTMP() {

    thread_start=false;
}