#ifndef APPPLATFORM_NDS_H__
#define APPPLATFORM_NDS_H__

#include <fstream>
#include <string>
#include <png.h>
#include <cstdio>
#include <cstdlib>

#if defined(__NDS__)
#include <nds.h>
#endif

#include "AppPlatform.h"
#include "NinecraftApp.h"
#include "platform/log.h"

// Разрешение NDS
static const int width = 256;
static const int height = 192;

static void png_funcReadFile(png_structp pngPtr, png_bytep data, png_size_t length) {
    ((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

static std::string nds_keyboard_text = "";

class AppPlatform_NDS : public AppPlatform
{
public:
    bool supportsTouchscreen() override { return true; }

    int getScreenWidth() override { return width; }
    int getScreenHeight() override { return height; }

    void buyGame() override {
        LOGI("buyGame() called, stubbed for NDS.\n");
    }

    void showKeyboard() override {
        LOGI("Keyboard requested, but not natively supported on NDS yet.\n");
    }

    void hideKeyboard() override {
    }

    bool isKeyboardVisible() override {
        return false;
    }

    std::string getKeyboardInput() override {
        return nds_keyboard_text;
    }

    bool isPowerVR() override {
        return false;
    }

    std::string defaultUsername() override {
        return "NDS";
    }

    BinaryBlob readAssetFile(const std::string& filename) override {
        std::string fullAssetPath = "fat:/minecraftpe/" + filename;

        FILE* fd = fopen(fullAssetPath.c_str(), "rb");
        if (!fd) {
            fullAssetPath = "nitro:/data/" + filename;
            fd = fopen(fullAssetPath.c_str(), "rb");
            if (!fd) {
                LOGI("failed to open: %s\n", fullAssetPath.c_str());
                return BinaryBlob();
            }
        }

        fseek(fd, 0, SEEK_END);
        size_t size = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        BinaryBlob blob;
        blob.size = size;
        blob.data = new unsigned char[blob.size];

        size_t rd = fread(blob.data, 1, blob.size, fd);
        fclose(fd);

        if (rd != blob.size) {
            LOGI("wrong size read: %s\n", fullAssetPath.c_str());
            return BinaryBlob();
        }

        return blob;
    }

    TextureData loadTexture(const std::string& filename_, bool textureFolder) override {
        TextureData out;
        out.data = nullptr;

        std::string filename = textureFolder ? "nitro:/data/images/" + filename_ : "nitro:/" + filename_;
        FILE* fd = fopen(filename.c_str(), "rb");
        
        if (!fd) {
            fclose(fd);
            filename = "fat:/minecraftpe/images/" + filename_;
            fd = fopen(filename.c_str(), "rb");
            if (!fd) {
                fclose(fd);
                filename = "fat:/minecraftpe/" + filename_;
                fd = fopen(filename.c_str(), "rb");
                if (!fd) {
                    LOGI("failed to open: %s\n", filename.c_str());
                    fclose(fd);
                }
            }
        }
        
        std::ifstream source(filename.c_str(), std::ios::binary);

        if (source) {
            png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

            if (!pngPtr) return out;

            png_infop infoPtr = png_create_info_struct(pngPtr);

            if (!infoPtr) {
                png_destroy_read_struct(&pngPtr, NULL, NULL);
                return out;
            }

            png_set_read_fn(pngPtr, (void*)&source, png_funcReadFile);
            png_read_info(pngPtr, infoPtr);

            out.w = png_get_image_width(pngPtr, infoPtr);
            out.h = png_get_image_height(pngPtr, infoPtr);

            png_bytep* rowPtrs = new png_bytep[out.h];
            out.data = new unsigned char[4 * out.w * out.h];
            out.memoryHandledExternally = false;

            int rowStrideBytes = 4 * out.w;
            for (int i = 0; i < out.h; i++) {
                rowPtrs[i] = (png_bytep)&out.data[i * rowStrideBytes];
            }

            png_read_image(pngPtr, rowPtrs);

            png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
            delete[] (png_bytep)rowPtrs;
            source.close();

            return out;
        } else {
            LOGI("Couldn't find file: %s\n", filename.c_str());
            return out;
        }
    }
};

#endif