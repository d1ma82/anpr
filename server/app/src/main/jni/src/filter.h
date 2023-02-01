#pragma once
#include <opencv2/core.hpp>
#include "types.h"

namespace filter {

class Filter {
protected:
    Filter() {}
public:
    
    virtual cv::UMat frame() const noexcept =0;
    virtual void apply(int orientation) =0;
    virtual dims get_viewport() const noexcept=0;
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