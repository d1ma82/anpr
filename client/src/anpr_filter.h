#pragma once

#include "filter.h"
#include "types.h"

static const std::string debug = "/storage/emulated/0/Android/data/com.home.anpr/files/";

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
    void detect_plates(cv::UMat& input,  Plate& plate);
    void detect_chars(Plate& plate);
    void classify_chars(Plate& plate);

public:
    const dims&     viewport;
    Filter*         input;
    cv::Rect        ROI_rect;
    cv::UMat        ROI;
   
    ANPR(Filter* filter_in, const dims& viewport_in, cv::Rect roi);
    void apply(int);
    cv::UMat& frame() noexcept { return input->frame(); }
    ~ANPR() { }
};
}