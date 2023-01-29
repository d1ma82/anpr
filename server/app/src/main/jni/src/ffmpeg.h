#pragma once

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

}

#include "log.h"

using Format    = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;
using Codec     = std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*)>>;
using Scaler    = std::unique_ptr<SwsContext, std::function<void(SwsContext*)>>;

inline void on_error(int, int, const char*, const char*);

#define CALL(proc) \
              err = proc;\
              on_error(err, __LINE__, __FILE__, #proc);

class Picture {
private:
    int    err       {0};
    const  int ALIGN {32};
    AVFrame* frame   {nullptr};
    uint8_t* data    {nullptr};

public:
    Picture(): frame{av_frame_alloc()} {}

    Picture(enum AVPixelFormat pix_fmt, int width, int height): Picture() {
        
        frame->format   = pix_fmt;
        frame->width    = width;
        frame->height   = height;
        frame->duration = 0;
        frame->pts      = 0;
        int sz  =  av_image_get_buffer_size(pix_fmt, width, height, ALIGN);
        data    =  (uint8_t*) av_malloc(sz);
        CALL(av_image_fill_arrays(frame->data, frame->linesize, data, pix_fmt, width, height, ALIGN))
       // int sz = av_image_alloc(frame->data, frame->linesize, width, height, pix_fmt, 32);
        LOGI("Pic size: %d, strides: %d, %d, %d, %d, %d, %d, %d, %d\n", sz, frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3], frame->linesize[4], frame->linesize[5], frame->linesize[6], frame->linesize[7])
    }

    AVFrame* operator->() const { return frame; }

    ~Picture() {

        if (data) av_free(data);
        data=nullptr;
        if (frame) av_frame_free(&frame);
    }
};

void on_error(int err, int line, const char* file, const char* func) {

    if (err > 0) return; else err=AVERROR(err);
    const char* str;
    switch(err) {
        case 0: return;
        case 1:    str= "EPERM            "; break;
        case 2  :  str= " ENOENT          "; break;
        case 3  :  str= " ESRCH           "; break;
        case 4  :  str= " EINTR           "; break;
        case 5  :  str= " EIO             "; break;
        case 6  :  str= " ENXIO           "; break;
        case 7  :  str= " E2BIG           "; break;
        case 8  :  str= " ENOEXEC         "; break;
        case 9  :  str= " EBADF           "; break;
        case 10 :   str= " ECHILD          "; break;
        case 12 :   str= " ENOMEM          "; break;
        case 13 :   str= " EACCES          "; break;
        case 14 :   str= " EFAULT          "; break;
        case 16 :   str= " EBUSY           "; break;
        case 17 :   str= " EEXIST          "; break;
        case 18 :   str= " EXDEV           "; break;
        case 19 :   str= " ENODEV          "; break;
        case 20 :   str= " ENOTDIR         "; break;
        case 21 :   str= " EISDIR          "; break;
        case 23 :   str= " ENFILE          "; break;
        case 24 :   str= " EMFILE          "; break;
        case 25 :   str= " ENOTTY          "; break;
        case 27 :   str= " EFBIG           "; break;
        case 28 :   str= " ENOSPC          "; break;
        case 29 :   str= " ESPIPE          "; break;
        case 30 :   str= " EROFS           "; break;
        case 31 :   str= " EMLINK          "; break;
        case 32 :   str= " EPIPE           "; break;
        case 33 :   str= " EDOM            "; break;
        case 36 :   str= " EDEADLK         "; break;
        case 38 :   str= " ENAMETOOLONG    "; break;
        case 39 :   str= " ENOLCK          "; break;
        case 40 :   str= " ENOSYS          "; break;
        case 41 :   str= " ENOTEMPTY       "; break;
        case 22 :   str= " EINVAL          "; break;
        case 34 :   str= " ERANGE          "; break;
        case 42 :   str= " EILSEQ          "; break;
        case 80 :   str= " STRUNCATE       "; break;
        case 100:    str= " EADDRINUSE      "; break;
        case 101:    str= " EADDRNOTAVAIL   "; break;
        case 102:    str= " EAFNOSUPPORT    "; break;
        case 103:    str= " EALREADY        "; break;
        case 104:    str= " EBADMSG         "; break;
        case 105:    str= " ECANCELED       "; break;
        case 106:    str= " ECONNABORTED    "; break;
        case 107:    str= " ECONNREFUSED    "; break;
        case 108:    str= " ECONNRESET      "; break;
        case 109:    str= " EDESTADDRREQ    "; break;
        case 110:    str= " EHOSTUNREACH    "; break;
        case 111:    str= " EIDRM           "; break;
        case 112:    str= " EINPROGRESS     "; break;
        case 113:    str= " EISCONN         "; break;
        case 114:    str= " ELOOP           "; break;
        case 115:    str= " EMSGSIZE        "; break;
        case 116:    str= " ENETDOWN        "; break;
        case 117:    str= " ENETRESET       "; break;
        case 118:    str= " ENETUNREACH     "; break;
        case 119:    str= " ENOBUFS         "; break;
        case 120:    str= " ENODATA         "; break;
        case 121:    str= " ENOLINK         "; break;
        case 122:    str= " ENOMSG          "; break;
        case 123:    str= " ENOPROTOOPT     "; break;
        case 124:    str= " ENOSR           "; break;
        case 125:    str= " ENOSTR          "; break;
        case 126:    str= " ENOTCONN        "; break;
        case 127:    str= " ENOTRECOVERABLE "; break;
        case 128:    str= " ENOTSOCK        "; break;
        case 129:    str= " ENOTSUP         "; break;
        case 130:    str= " EOPNOTSUPP      "; break;
        case 131:    str= " EOTHER          "; break;
        case 132:    str= " EOVERFLOW       "; break;
        case 133:    str= " EOWNERDEAD      "; break;
        case 134:    str= " EPROTO          "; break;
        case 135:    str= " EPROTONOSUPPORT "; break;
        case 136:    str= " EPROTOTYPE      "; break;
        case 137:    str= " ETIME           "; break;
        case 138:    str= " ETIMEDOUT       "; break;
        case 139:    str= " ETXTBSY         "; break;
        case 140:    str= " EWOULDBLOCK     "; break;
        case 11: return;
        case AVERROR_EOF: return;
    }
    LOGE("[FAIL] %s, %d, %s, %s\n", file, line, func, str);
}
