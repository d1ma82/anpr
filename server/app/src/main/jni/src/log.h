#pragma once

#include <stdexcept>

#ifdef ANDROID
    #include <android/log.h>

    #define TAG "anpr: "
    #define LOGI(...) (__android_log_print(ANDROID_LOG_INFO, TAG __FILE__, __VA_ARGS__));
    #define LOGE(...) (__android_log_print(ANDROID_LOG_ERROR, TAG __FILE__, __VA_ARGS__));
#else
    #define LOGI(str, ...) printf( str, __VA_ARGS__);
    #define LOGE(format, ...)  	\
    {								\
    	fprintf(stderr, "[ERR] [%s:%s:%d] " format "", \
    	__FILE__, __FUNCTION__ , __LINE__, ##__VA_ARGS__);     \
    }
#endif

