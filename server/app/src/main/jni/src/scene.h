#pragma once

#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>

#include "types.h"

#define ASSERT(cond, fmt, ...)                                \
  if (!(cond)) __android_log_assert(#cond, "ASSERT", fmt, ##__VA_ARGS__); 
  
#define CAMCALL(func) status = func; \
        ASSERT(status == 0, "%d error code returned by %s", status, #func);


class Scene {
    protected:
        Scene() {}

    public:
        virtual ~Scene() {}
        virtual void take_shot(const char* path)=0;
        virtual void start_preview(bool toogle)=0;
        virtual const int32_t* characteristic(int& size) const =0;
        virtual void onSurfaceCreated(ANativeWindow* window)=0;
        virtual void onSurfaceChanged(const dims&)=0;
        virtual void onDraw(int)=0;
};