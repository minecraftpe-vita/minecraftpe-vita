#ifndef APPPLATFORM_VITA_H__
#define APPPLATFORM_VITA_H__

#include <fstream>
#include <png.h>

#include "AppPlatform.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include "NinecraftApp.h"

static const int width = 960;
static const int height = 544;

static void png_funcReadFile(png_structp pngPtr, png_bytep data, png_size_t length) {
	((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}


class AppPlatform_Vita : public AppPlatform
{
public:
	bool supportsTouchscreen() override { return true; }

	int getScreenWidth() override { return width; }
	int getScreenHeight() override { return height; }

	bool isPowerVR() override { return true; }

	BinaryBlob readAssetFile(const std::string& filename) override {
		std::string fullAssetPath = ("app0:data/" + filename);

		sceClibPrintf("fullAssetPath: %s\n", fullAssetPath.c_str());
		SceIoStat stat;
		int ret = sceIoGetstat(fullAssetPath.c_str(), &stat);
		if(ret < 0) {
			sceClibPrintf("failed to stat: %x %s\n", ret,fullAssetPath.c_str());
			return BinaryBlob();
		}

		SceUID fd = sceIoOpen(fullAssetPath.c_str(), SCE_O_RDONLY, 0777);

		if(fd < 0) {
			sceClibPrintf("failed to open: %x %s\n", fd, fullAssetPath.c_str());
			return BinaryBlob();
		}


		BinaryBlob blob;
		blob.size = stat.st_size;
		blob.data = new unsigned char[blob.size];

		int rd = sceIoRead(fd, blob.data, blob.size);
		sceIoClose(fd);

		if(rd != blob.size) {
			sceClibPrintf("wrong size: %x %s\n", rd, fullAssetPath.c_str());

			return BinaryBlob();
		}

		sceClibPrintf("read %x bytes from %s\n", rd, fullAssetPath.c_str());

		return blob;
	}

	TextureData loadTexture(const std::string& filename_, bool textureFolder) override {
		TextureData out;

		std::string filename = textureFolder ? "data/images/" + filename_
		: filename_;
		std::ifstream source(filename.c_str(), std::ios::binary);

		if (source) {
			png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

			if (!pngPtr)
				return out;

			png_infop infoPtr = png_create_info_struct(pngPtr);

			if (!infoPtr) {
				png_destroy_read_struct(&pngPtr, NULL, NULL);
				return out;
			}

			// Hack to get around the broken libpng for windows
			png_set_read_fn(pngPtr,(void*)&source, png_funcReadFile);

			png_read_info(pngPtr, infoPtr);

			// Set up the texdata properties
			out.w = png_get_image_width(pngPtr, infoPtr);
			out.h = png_get_image_height(pngPtr, infoPtr);

			png_bytep* rowPtrs = new png_bytep[out.h];
			out.data = new unsigned char[4 * out.w * out.h];
			out.memoryHandledExternally = false;

			int rowStrideBytes = 4 * out.w;
			for (int i = 0; i < out.h; i++) {
				rowPtrs[i] = (png_bytep)&out.data[i*rowStrideBytes];
			}
			png_read_image(pngPtr, rowPtrs);

			// Teardown and return
			png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
			delete[] (png_bytep)rowPtrs;
			source.close();

			return out;
		}
		else
		{
			LOGI("Couldn't find file: %s\n", filename.c_str());
			return out;
		}
	}
};

#endif
