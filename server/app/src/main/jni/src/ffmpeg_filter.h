#pragma once

#include "filter.h"
#include "types.h"

namespace filter {

class RTMP: public Filter {
private:
    std::string server;
    cv::Mat frame_clone;
public:
    int                     bitrate     {500000};
    const dims&             viewport;
    const range<int32_t>&   fps;
    Filter*                 input;

    RTMP(Filter* filter_in, const dims& viewport_in, const range<int32_t>& fps_in, const char* ip);

    void apply(int);
    cv::UMat frame() noexcept { return input->frame(); }
    ~RTMP();
};
}