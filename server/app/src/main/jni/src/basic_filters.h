#pragma once

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include "filter.h"
#include "assets.h"
#include "opengl.h"
#include "opencl.h"
#include "log.h"

/**
 *  Draw input from camera into a framebuffer
*/
namespace filter {
    
class Camera : public Input {
public:
    const Program&  input;
    const dims&     viewport;
    cv::UMat        frame_pix;
    GLenum          err;
    GLuint          FBO{0}, RBO{0};

    Camera(
        const dims& viewport_in, 
        const Program& prog_in
    ): input{prog_in}, viewport{viewport_in} {
        
        initCL();

        glGenFramebuffers(1, &FBO);
        CALLGL(glBindFramebuffer(GL_FRAMEBUFFER, FBO))
        
        texture = d2_texture(viewport.first, viewport.second, nullptr);
        CALLGL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0));
        CALLGL(glBindTexture(GL_TEXTURE_2D, 0))
        
        CALLGL(glGenRenderbuffers(1, &RBO));
        CALLGL(glBindRenderbuffer(GL_RENDERBUFFER, RBO));
        CALLGL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO));

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) LOGE("Camera framebuffer is not complete!")
        CALLGL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
              
    void apply(int) {

        CALLGL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO))
        CALLGL(glClearColor(0.1f, 0.1f, 0.1f, 0.0f));
        CALLGL(glClear(GL_COLOR_BUFFER_BIT));

        CALLGL(glUseProgram(input.id));
        
        CALLGL(glBindVertexArray(input.VAO));
        CALLGL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        glBindVertexArray(0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glFinish();
            // pixels example
      /*  glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, viewport.first, viewport.second, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glFinish();*/

        CALLGL(glBindTexture(GL_TEXTURE_2D, texture))
        get_frame(texture, frame_pix);
        //LOGI("Input %zu, %dx%d", frame_pix.step1(), frame_pix.cols, frame_pix.rows)
        CALLGL(glBindTexture(GL_TEXTURE_2D, 0))
    }

    ~Camera() { 

        glDeleteTextures(1, &texture);
        glDeleteRenderbuffers(1, &RBO);
        glDeleteFramebuffers(1, &FBO);

        freeCL();
    }

    cv::UMat frame() noexcept { return frame_pix; }
};

// When use this filter need to define IMAGE
class Image: public Input {
public:
    const char*     img_path;
    const dims&     viewport;
    cv::UMat        frame_pix;
    cv::Mat         img;

    Image(
        const char* path,
        const dims& viewport_in 
    ):img_path{path},viewport{viewport_in} {

        if (!asset::manager->open(path)) return;

        const char* buf = asset::manager->read();
        std::vector<uchar> vec(&buf[0], &buf[asset::manager->size()]);
        
        img = cv::imdecode(vec, cv::IMREAD_UNCHANGED); 
        texture=d2_texture(viewport.first, viewport.second, img.data);

        asset::manager->close();
    }

    ~Image() {glDeleteTextures(1, &texture);}

    void apply(int) {}
    cv::UMat frame() const noexcept { return frame_pix;}
};

/**
 *  Draw a rectangle(ROI) in a center of picture
*/
class Rect : public Filter {
public:
    cv::Rect        roi;
    Filter*         input;
    const dims&     viewport;
    static const int DIV=4;
    int x1, x2, y1, y2, cx, cy;

    Rect(Input* filter_in, const dims& viewport_in): 
        input{filter_in}, viewport{viewport_in}  {
        
        bool wide = ((float)viewport.first/viewport.second) > (16.0f/9-0.00001f);
        cx=viewport.first/2;
        cy=viewport.second/(wide? 2: 3);                   // Мы используем канву, а она может быть больше экрана
        x1=(viewport.first/DIV);
        y1=(viewport.second/DIV);
        x2=cx+(cx-x1);
        y2=cy+(cy-y1);
        roi     =   cv::Rect(cv::Point2i(x1, y1), cv::Point2i(x2, y2));
        LOGI("%d, %d, %d, %d, %s", viewport.first, viewport.second, roi.width, roi.height, wide? "true": "false")
    }

    void apply(int) {

        cv::rectangle(input->frame(), roi, cv::Scalar(0,255,0));
    }

    ~Rect() {}
    cv::UMat frame() noexcept { return input->frame();}
};

class Frame: public Output {
public:
    Filter*         input;
    const dims&     viewport;
    GLenum          err;

    Frame(
        Filter* filter_in, 
        const dims& viewport_in
    ):input{filter_in}, viewport{viewport_in} {

        texture = d2_texture(viewport.first, viewport.second, nullptr);
    } 

    void apply(int) {

        CALLGL(glBindTexture(GL_TEXTURE_2D, texture))
        set_frame(texture, input->frame());
        CALLGL(glBindTexture(GL_TEXTURE_2D, 0))
        
        //  pixels example
       // CALLGL(glBindTexture(GL_TEXTURE_2D, texture))
        //CALLGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport.first, viewport.second, 0, GL_RGBA, GL_UNSIGNED_BYTE, input->data()))
    }

    ~Frame() { glDeleteTextures(1, &texture); }
    cv::UMat frame() noexcept { return input->frame(); }
};

/**
 *  Make a shot and save in data folder
*/
class Shot: public Filter {
public:
    const dims&     viewport;
    Filter*         input;
    const char*     shot_path;  
    cv::UMat        frame_pix;

    Shot(
        Filter* filter_input, 
        const char* path,
        const dims&     viewport_in): 
            input{filter_input},
            viewport{viewport_in},
            shot_path{path} {
    }

    void apply(int orientation) {

        std::string file_name = shot_path+std::string("/shot.png");
        LOGI("Write file %s", file_name.c_str())

       // cv::Mat file(viewport.second, viewport.first, CV_8UC4, input->data());
      //  if (orientation>200) cv::flip(file, file, 0);
       // else if (orientation>0 && orientation<200) cv::flip(file, file, 1);
        //cv::imwrite(file_name, file);
    }

    cv::UMat frame() noexcept { return frame_pix;}
    ~Shot() { } 
};
}