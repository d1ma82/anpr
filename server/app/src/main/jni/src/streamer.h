#pragma once

#include "filter.h"

namespace filter {

class RTMP: public Filter {
private:
    std::string server;
    cv::Mat frame_clone;

public:
    int                     bitrate     {500000};
    
    const range<int32_t>&   fps;
    Filter*                 input;

    RTMP(Filter* filter_in, const range<int32_t>& fps_in, const char* ip);

    void apply(int);
    cv::UMat frame() const noexcept { return input->frame(); }
    dims get_viewport() const noexcept { return input->get_viewport(); }
    ~RTMP();
};
}