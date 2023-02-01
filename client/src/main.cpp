#include <iostream>
#include <opencv2/highgui.hpp>

#include "basic_filters.h"
#include "player.h"
#include "anpr.h"
#include "log.h"

const char* server = "rtmp://192.168.1.100/live/mystream/";
static std::vector<filter::Filter*> filters;

static void init() {

    av_log_set_level(AV_LOG_INFO);
    
    filters.push_back(new filter::Player(server));
    filters.push_back(new filter::ROI((filter::Input*) filters[0]));
    filter::ROI* roi = dynamic_cast<filter::ROI*>(*filters.rbegin());
    filters.push_back(new filter::ANPR(*filters.rbegin(), roi->roi));
}

static void loop() {

    while (true) {

        for(auto el: filters) el->apply(0);

        cv::imshow("VID", (*filters.rbegin())->frame() );
        if (cv::waitKey(5)==27) break;
    }
}

static void clear() {

    for (auto el: filters) delete el;
    filters.resize(0);
    cv::destroyWindow("VID");
}

int main (int argc, char** argv) {

    int result=0;
    try{
        init();
        loop();
        clear();
    } catch (std::exception& e) {
        LOGI("%s", e.what())
        result = 1;
    }
    LOGI("End")
    
    return result;
}
