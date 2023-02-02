#pragma once

#include "filter.h"
#define IMGDEBUG

struct Char {
    cv::UMat img;
    cv::Rect pos;
};

struct Plate {
    Plate() {}
    Plate(cv::UMat img, cv::Rect pos): image{img}, position{pos} {}
    cv::UMat image;
    cv::Rect position;
    std::stringstream text;
    std::vector<Char> chars;
};

namespace filter {
class ANPR: public Filter {
private:
    Filter* input;
    cv::Rect roi;
    void detect_plates(cv::UMat& input,  Plate& plate);
    void detect_chars(Plate& plate);
    void classify_chars(cv::UMat& input, Plate& plate);

public:
   
    ANPR(Filter* in, cv::Rect roi_input);
    void apply(int);
    cv::UMat frame() const  noexcept { return input->frame(); }
    dims get_viewport() const noexcept { return input->get_viewport(); }
    ~ANPR() { }
};
}