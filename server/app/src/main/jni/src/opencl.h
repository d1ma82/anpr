#pragma once
#include <CL/opencl.h>
#include <opencv2/core/ocl.hpp>

#define CALLCL2(result, proc) \
                result=proc;\
                CLError(err, __LINE__, __FILE__, #proc);
        
#define CALLCL(proc) \
                err = proc;\
                CLError(err, __LINE__, __FILE__, #proc);


void CLError(cl_int, int, const char*, const char*);
void initCL(); 
void get_frame(uint32_t texture, cv::UMat& result);
void set_frame(uint32_t texture, const cv::UMat& image);
void freeCL();
