#ifndef APPPLATFORM_LINUX_H__
#define APPPLATFORM_LINUX_H__

#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <png.h>
#include <GLFW/glfw3.h>

#include "AppPlatform.h"
#include "NinecraftApp.h"

static void png_funcReadFile(png_structp pngPtr, png_bytep data, png_size_t length) {
	FILE* file = (FILE*)png_get_io_ptr(pngPtr);
	fread(data, length, 1, file);
}

class AppPlatform_Linux : public AppPlatform
{
private:
	GLFWwindow* _window;
public:
	AppPlatform_Linux(GLFWwindow* window) : _window(window) {}

	bool supportsTouchscreen() override {
		return false;
	}

	int getScreenWidth() override {
		int width, height;
		glfwGetWindowSize(_window, &width, &height);
		return width;
	}
	int getScreenHeight() override {
		int width, height;
		glfwGetWindowSize(_window, &width, &height);
		return height;
	}

	bool isPowerVR() override { return true; }

	std::string defaultUsername() override {
		return "Linux";
	}

	BinaryBlob readAssetFile(const std::string& filename) override {
		std::string fullAssetPath = ("data/" + filename);

		LOGI("fullAssetPath: %s\n", fullAssetPath.c_str());

		struct stat file_stat;
		int ret = stat(fullAssetPath.c_str(), &file_stat);
		if(ret < 0) {
			LOGI("failed to stat: %x %s\n", ret, fullAssetPath.c_str());
			return BinaryBlob();
		}

		FILE* file = fopen(fullAssetPath.c_str(), "rb");
		if(file == 0) {
			LOGI("failed to open: %s\n", fullAssetPath.c_str());
			return BinaryBlob();
		}


		BinaryBlob blob;
		blob.size = file_stat.st_size;
		blob.data = new unsigned char[blob.size];

		size_t rd = fread(blob.data, blob.size, 1, file);
		if(rd != blob.size) {
			LOGI("wrong size: %lx %s\n", rd, fullAssetPath.c_str());

			return BinaryBlob();
		}

		LOGI("read %lx bytes from %s\n", rd, fullAssetPath.c_str());

		return blob;
	}

	TextureData loadTexture(const std::string& filename_, bool textureFolder) override {
		TextureData out;

		std::string filename = textureFolder ? "data/images/" + filename_ : filename_;

		FILE* file = fopen(filename.c_str(), "rb");
		if(file) {
			png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if (!pngPtr) {
				fclose(file);
				return out;
			}
			png_init_io(pngPtr, file);

			png_infop infoPtr = png_create_info_struct(pngPtr);
			if (!infoPtr) {
				png_destroy_read_struct(&pngPtr, NULL, NULL);
				fclose(file);
				return out;
			}
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
			fclose(file);

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
