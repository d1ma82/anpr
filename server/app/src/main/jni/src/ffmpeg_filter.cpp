#include <thread>

#include "ffmpeg_filter.h"
#include "rtmp.h"

static bool thread_start {false};
static bool frame_available {false};

static void run(
    dims src, 
    range<int32_t> fps, 
    int bitrate, 
    const cv::Mat* const input, 
    dims dst, 
    const char* server
) {
    LOGI("Start server %s; src= %dx%d fps= %d; dst= %dx%d\n",server, src.first, src.second, fps.first, dst.first, dst.second);

    av_log_set_level(AV_LOG_TRACE);
    RTMP rtmp(server);
    Scaler scaler = {[] (int src_width, 
                         int src_height, 
                         enum AVPixelFormat srcFormat,
                         int dst_width, 
                         int dst_height, 
                         enum AVPixelFormat dstFormat, 
                         int flags
    )   {
            SwsContext* ctx  = sws_getContext(src_width, src_height, srcFormat, dst_width, dst_height, dstFormat, flags, nullptr, nullptr, nullptr);
            if (!ctx) LOGE("Could not init sample scaler\n")
            return ctx;

        } (src.first, src.second, AV_PIX_FMT_RGBA, dst.first, dst.second, rtmp.out_pixel_fmt, SWS_BILINEAR), sws_freeContext
    };
    
    Picture pic(AV_PIX_FMT_YUV420P, dst.first, dst.second);
    LOGI("Listen incomings\n")
    if (!rtmp.listen()) return;
    LOGI("Begin stream\n")
    rtmp.begin_stream(dst.first, dst.second, fps.first, bitrate);
    rtmp.start_time=rtmp.clk.now();

    while (thread_start) {
        // stream frames
        if (!frame_available) continue;
        
        const int stride[] {static_cast<int>(input->step1())};
        int ret=sws_scale(scaler.get(), &input->data, stride, 0, src.second, pic->data, pic->linesize);
        if (!rtmp.encode_and_write_frame(pic)) break;
        // proccess frames
        frame_available = false;
    }
    thread_start=false;
    LOGI("Stream total: %f sec\n", pic->pts*av_q2d(rtmp.out_codec_ctx->time_base))
}

filter::RTMP::RTMP(
        Filter* filter_in, 
        const dims& viewport_in, 
        const range<int32_t>& fps_in,
        const char* ip
): input{filter_in}, viewport{viewport_in}, fps{fps_in} {
        
    thread_start=true;
    server = "rtmp://"+std::string(ip)+"/live/mystream/";
    std::thread rtmp_server_handler(run, viewport, fps, bitrate, &frame_clone, dims{1280, 720}, server.c_str());
    rtmp_server_handler.detach();
}

void filter::RTMP::apply(int) {

    if (!frame_available) {

        frame().getMat(cv::ACCESS_READ).copyTo(frame_clone);
        frame_available = true;
    }
}

filter::RTMP::~RTMP() {

    thread_start=false;
}