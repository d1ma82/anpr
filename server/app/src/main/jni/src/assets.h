#pragma once

#ifdef ANDROID
#include <android/asset_manager.h>
#endif

#include "log.h"

class Asset;

namespace asset {
    inline Asset* manager {nullptr};
}

#ifdef ANDROID
class Asset {
    private:
        off_t           count   {0};
        AAsset*         file    {nullptr};
        char*           content {nullptr};
        AAssetManager*  manager;
    public:
               // copy-move disabled
        Asset(const Asset&) = delete;
        Asset(Asset&&) = delete;
        Asset& operator=(const Asset&) = delete;
        Asset& operator=(Asset&&) = delete;

        Asset(AAssetManager* manager_input): manager{manager_input} {}

        bool open(const char* path) {

            LOGI("Asset create %s", path)
            file  = AAssetManager_open(manager, path, AASSET_MODE_BUFFER);

            if (file==nullptr) {

                LOGE("Could not open asset, %s", path)
                return false;
            } else {
                
                count   =   AAsset_getLength(file);
                content =   new char[count+1];
                LOGI("Asset opened OK %d bytes", (int)count)
                return true;
            }
        }

        ~Asset() {}

        const char* read() {
            
            if (AAsset_read(file, content, count)<=0) LOGE("Asset::get Could not read asset");
            content[count]='\0';
            return content;
        }

        void close() { delete[] content; count=0; AAsset_close(file); }
        off_t size() const {return count;}
};
#endif