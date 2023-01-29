#pragma once
#include <opencv2/core.hpp>

namespace filter {

//using Frame = std::shared_ptr<cv::UMat>

class Filter {
protected:
    Filter() {}
public:
    
    virtual cv::UMat frame() noexcept =0;
    virtual void apply(int orientation) =0;
    virtual ~Filter()=default;
};


class Input : public Filter {
protected:
    uint32_t texture{0};
    Input() {}
public:
    virtual ~Input()=default;
};

class Output: public Filter {
protected:
    Output() {};
public:
    uint32_t texture{0};
    virtual ~Output() {}
};
}