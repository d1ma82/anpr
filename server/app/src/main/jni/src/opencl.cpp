#include <chrono>
#include <EGL/egl.h>
#include "opencl.h"
#include "opengl.h"
#include "log.h"

static cl_int err {0};
static cl_platform_id   current_platform {nullptr};
static cl_device_id     current_device   {nullptr};
static cl_context       context          {nullptr};
static cl_command_queue queue            {nullptr};

void CLError(cl_int err, int line, const char* file, const char* proc) {

const char* str;
switch(err) {
case 0  : return;
case -1 : str="CL_DEVICE_NOT_FOUND                         ";break;
case -2 : str="CL_DEVICE_NOT_AVAILABLE                     ";break;
case -3 : str="CL_COMPILER_NOT_AVAILABLE                   ";break;
case -4 : str="CL_MEM_OBJECT_ALLOCATION_FAILURE            ";break;
case -5 : str="CL_OUT_OF_RESOURCES                         ";break;
case -6 : str="CL_OUT_OF_HOST_MEMORY                       ";break;
case -7 : str="CL_PROFILING_INFO_NOT_AVAILABLE             ";break;
case -8 : str="CL_MEM_COPY_OVERLAP                         ";break;
case -9 : str="CL_IMAGE_FORMAT_MISMATCH                    ";break;
case -10: str="CL_IMAGE_FORMAT_NOT_SUPPORTED               ";break;
case -11: str="CL_BUILD_PROGRAM_FAILURE                    ";break;
case -12: str="CL_MAP_FAILURE                              ";break;
case -13: str="CL_MISALIGNED_SUB_BUFFER_OFFSET             ";break;
case -14: str="CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";break;
case -15: str="CL_COMPILE_PROGRAM_FAILURE                  ";break;
case -16: str="CL_LINKER_NOT_AVAILABLE                     ";break;
case -17: str="CL_LINK_PROGRAM_FAILURE                     ";break;
case -18: str="CL_DEVICE_PARTITION_FAILED                  ";break;
case -19: str="CL_KERNEL_ARG_INFO_NOT_AVAILABLE            ";break;
case -30: str="CL_INVALID_VALUE                            ";break;
case -31: str="CL_INVALID_DEVICE_TYPE                      ";break;
case -32: str="CL_INVALID_PLATFORM                         ";break;
case -33: str="CL_INVALID_DEVICE                           ";break;
case -34: str="CL_INVALID_CONTEXT                          ";break;
case -35: str="CL_INVALID_QUEUE_PROPERTIES                 ";break;
case -36: str="CL_INVALID_COMMAND_QUEUE                    ";break;
case -37: str="CL_INVALID_HOST_PTR                         ";break;
case -38: str="CL_INVALID_MEM_OBJECT                       ";break;
case -39: str="CL_INVALID_IMAGE_FORMAT_DESCRIPTOR          ";break;
case -40: str="CL_INVALID_IMAGE_SIZE                       ";break;
case -41: str="CL_INVALID_SAMPLER                          ";break;
case -42: str="CL_INVALID_BINARY                           ";break;
case -43: str="CL_INVALID_BUILD_OPTIONS                    ";break;
case -44: str="CL_INVALID_PROGRAM                          ";break;
case -45: str="CL_INVALID_PROGRAM_EXECUTABLE               ";break;
case -46: str="CL_INVALID_KERNEL_NAME                      ";break;
case -47: str="CL_INVALID_KERNEL_DEFINITION                ";break;
case -48: str="CL_INVALID_KERNEL                           ";break;
case -49: str="CL_INVALID_ARG_INDEX                        ";break;
case -50: str="CL_INVALID_ARG_VALUE                        ";break;
case -51: str="CL_INVALID_ARG_SIZE                         ";break;
case -52: str="CL_INVALID_KERNEL_ARGS                      ";break;
case -53: str="CL_INVALID_WORK_DIMENSION                   ";break;
case -54: str="CL_INVALID_WORK_GROUP_SIZE                  ";break;
case -55: str="CL_INVALID_WORK_ITEM_SIZE                   ";break;
case -56: str="CL_INVALID_GLOBAL_OFFSET                    ";break;
case -57: str="CL_INVALID_EVENT_WAIT_LIST                  ";break;
case -58: str="CL_INVALID_EVENT                            ";break;
case -59: str="CL_INVALID_OPERATION                        ";break;
case -60: str="CL_INVALID_GL_OBJECT                        ";break;
case -61: str="CL_INVALID_BUFFER_SIZE                      ";break;
case -62: str="CL_INVALID_MIP_LEVEL                        ";break;
case -63: str="CL_INVALID_GLOBAL_WORK_SIZE                 ";break;
case -64: str="CL_INVALID_PROPERTY                         ";break;
case -65: str="CL_INVALID_IMAGE_DESCRIPTOR                 ";break;
case -66: str="CL_INVALID_COMPILER_OPTIONS                 ";break;
case -67: str="CL_INVALID_LINKER_OPTIONS                   ";break;
case -68: str="CL_INVALID_DEVICE_PARTITION_COUNT           ";break;
default : str="Unknown Error"; 
}
LOGE("[FAIL CL] %s, %d, %s, %s", file, line, proc, str);
}

static std::string device_type(cl_device_type type) {

    switch(type) {
        case  CL_DEVICE_TYPE_CPU: return "CPU";
        case  CL_DEVICE_TYPE_GPU: return "GPU";
        case  CL_DEVICE_TYPE_ACCELERATOR: return "ACCELERATOR";
        default: return "Unknown";
    }
}

static void dump() {

    LOGI("*** OpenCL info ***")
    try{
        std::vector<cv::ocl::PlatformInfo> infos;
        cv::ocl::getPlatfomsInfo(infos);

        for (auto& info: infos){
            
            cv::ocl::Device device;
            int n = info.deviceNumber();
    
            LOGI("OpenCL platform name=%s, ver=%s, ven=%s, devn=%d", 
                info.name().c_str(), info.version().c_str(), info.vendor().c_str(), n)

            for (int i=0; i<n; i++)  {
                
                info.getDevice(device, i);
                LOGI("OpenCL device name=%s, (%s), ext=%s", device.name().c_str(), 
                    device_type(device.type()).c_str(), device.extensions().c_str())
            }
        }
    } catch(const cv::Exception& err) {
        LOGE("OpenCL info: error while gathering OpenCL info: %s (%d)", err.what(), err.code)
    }
}


static void get_platform_dev_by_extension(
    const char* extention, 
    cl_platform_id& ret_platform, 
    cl_device_id& ret_dev
) {
    cl_uint num_platforms = 0;
    CALLCL(clGetPlatformIDs(0, nullptr, &num_platforms))
    std::vector<cl_platform_id> platforms;
    platforms.resize(num_platforms);
    CALLCL(clGetPlatformIDs(num_platforms, platforms.data(), nullptr))

    char* ext {nullptr};
    
    for (auto& platform: platforms) {
        
        cl_uint num_devices=0;
        CALLCL(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices))
        std::vector<cl_device_id> devices;
        devices.resize(num_devices);
        CALLCL(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), nullptr))
        
        for (auto& dev: devices) {

            size_t size=0;
            CALLCL(clGetDeviceInfo(dev, CL_DEVICE_EXTENSIONS, 0, nullptr, &size))
            if (size>0) {
                ext = new char[size+1];
                CALLCL(clGetDeviceInfo(dev, CL_DEVICE_EXTENSIONS, size, ext, nullptr))
                ext[size]=0;

                if (strstr(ext, extention)) {

                    ret_platform    =platform;
                    ret_dev         =dev;
                    delete[] ext;
                    return;
                }
                delete[] ext;
            }
        }
    }
}

static void callback(
    const char* errinfo, 
    const void* private_info, 
    size_t cb, 
    void *user_data
) {
    LOGI("Callback: %s", errinfo)
}

static void create_context() {

    EGLDisplay display = eglGetCurrentDisplay();
    if (display == EGL_NO_DISPLAY) LOGE("EGL_NO_DISPLAY, %x", eglGetError())

    EGLContext egl_context = eglGetCurrentContext();
    if (egl_context==EGL_NO_CONTEXT) LOGE("EGL_NO_CONTEXT, %x", eglGetError())

    cl_context_properties props[] {
        CL_GL_CONTEXT_KHR, (cl_context_properties)  egl_context,
        CL_EGL_DISPLAY_KHR, (cl_context_properties) display,
        CL_CONTEXT_PLATFORM, (cl_context_properties) current_platform,
        0
    };
    cl_device_type type;
    CALLCL(clGetDeviceInfo(current_device, CL_DEVICE_TYPE, sizeof(type), &type, nullptr))
    CALLCL2(context, clCreateContextFromType(props, type, callback, nullptr, &err))
    CALLCL2(queue, clCreateCommandQueueWithProperties(context, current_device, 0, &err))
}

static void platform_name(std::string& name_ret) {

    size_t size  = 0;

    CALLCL(clGetPlatformInfo(current_platform, CL_PLATFORM_NAME, 0, nullptr, &size))
    if (size>0) {
        char* buf = new char[size+1];
        CALLCL(clGetPlatformInfo(current_platform, CL_PLATFORM_NAME, size, buf, nullptr))
        buf[size]   =0;
        name_ret    =buf;
        delete[] buf;
    }
}

void initCL() {

    dump(); 
    get_platform_dev_by_extension("cl_khr_gl_sharing", current_platform, current_device);
    if (!current_platform || !current_device) {LOGE("Not support cl_khr_gl_sharing") return;}
    LOGI("Support cl_khr_gl_sharing.")
    create_context();
    std::string name;
    platform_name(name);
    cv::ocl::attachContext(name, current_platform, context, current_device);
    if (cv::ocl::useOpenCL()) LOGI("OpenCV+OpenCL OK") else LOGI("Cant init OpenCV with OpenCL")
}

void get_frame(uint32_t texture, cv::UMat& result) {

    //auto tick = std::chrono::system_clock::now();
    cl_mem frame;
    CALLCL2(frame, clCreateFromGLTexture(context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, texture, &err))
    std::vector<cl_mem> images(1, frame);
    CALLCL(clEnqueueAcquireGLObjects(queue, images.size(), images.data(), 0, nullptr, nullptr))
    cv::ocl::convertFromImage(frame, result);
    CALLCL(clEnqueueReleaseGLObjects(queue, images.size(), images.data(), 0, nullptr, nullptr))
    CALLCL(clFinish(queue))
    //auto tock = std::chrono::system_clock::now();
    //LOGI("Load texture to UMat costs %u ms", (int32_t) std::chrono::duration_cast<std::chrono::milliseconds>(tock-tick).count())
}

void set_frame(uint32_t texture, const cv::UMat& image) {

    cl_mem frame;
    CALLCL2(frame, clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture, &err))    
    std::vector<cl_mem> images(1, frame);
    CALLCL(clEnqueueAcquireGLObjects(queue, images.size(), images.data(), 0, nullptr, nullptr))
    cl_mem buf = (cl_mem) image.handle(cv::ACCESS_READ);
    //LOGI("Image %dx%d, ch %d", image.rows, image.cols, image.channels())
    size_t origins[3] {0,0,0};
    size_t region [3] {static_cast<size_t>(image.cols), static_cast<size_t>(image.rows), 1};
    CALLCL(clEnqueueCopyBufferToImage(queue, buf, frame, 0, origins, region, 0, nullptr, nullptr))
    CALLCL(clEnqueueReleaseGLObjects(queue, images.size(), images.data(), 0, nullptr, nullptr))
    CALLCL(clFinish(queue))
}

void freeCL() {

    if (queue) clReleaseCommandQueue(queue);
    if (current_device) clReleaseDevice(current_device);
    if (context) clReleaseContext(context);
    
    current_device=nullptr;
    current_platform=nullptr;
    context=nullptr;
    queue=nullptr;
}

// ext=
//          cl_khr_3d_image_writes 
//          cl_img_egl_image 
//          cl_khr_byte_addressable_store 
//          cl_khr_depth_images 
//          cl_khr_egl_event 
//          cl_khr_egl_image 
//          cl_khr_fp16 
//          cl_khr_gl_sharing 
//          cl_khr_global_int32_base_atomics 
//          cl_khr_global_int32_extended_atomics 
//          cl_khr_local_int32_base_atomics 
//          cl_khr_local_int32_extended_atomics 
//          cl_khr_image2d_from_buffer 
//          cl_khr_mipmap_image 
//          cl_khr_srgb_image_writes 
//          cl_khr_subgroups 
//          cl_qcom_create_buffer_from_image 
//          cl_qcom_ext_host_ptr 
//          cl_qcom_ion_host_ptr 
//          cl_qcom_perf_hint 
//          cl_qcom_read_image_2x2 
//          cl_qcom_android_native_buffer_host_ptr 
//          cl_qcom_protected_context 
//          cl_qcom_priority_hint 
//          cl_qcom_compressed_yuv_image_read 
//          cl_qcom_compressed_image