#ifndef APPPLATFORM_VITA_H__
#define APPPLATFORM_VITA_H__

#include <png.h>

#include "AppPlatform.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/appmgr.h>
#include <psp2/libime.h>
#include <psp2/registrymgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/rtc.h>
#include <psp2/np/mgr.h>

#include <cstdlib>

#include "NinecraftApp.h"


static int64_t vbTimeStart = 0;
static int64_t vbTimeElapsed = 0;

static void VibrateController(uint64_t ms) {
	if(ms != 0) {
		vbTimeStart = sceKernelGetProcessTimeWide();
		vbTimeElapsed = ms * 1000.0;
	}

	if(vbTimeElapsed > 0 ) {
		SceCtrlActuator actuatorSetting = {0};
		actuatorSetting.small = 0x7F;
		actuatorSetting.large = 0x7F;
		sceCtrlSetActuator(1, &actuatorSetting);

		vbTimeElapsed -= (sceKernelGetProcessTimeWide() - vbTimeStart);
	}

}

static void Utf16ToUtf8(const uint16_t *src, uint8_t *dst)
{
	int i;
	for (i = 0; src[i]; i++) {
		if (!(src[i] & 0xFF80)) {
			*(dst++) = src[i] & 0xFF;
		} else if (!(src[i] & 0xF800)) {
			*(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		} else if ((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
			*(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
			*(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
			*(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
			*(dst++) = (src[i + 1] & 0x3F) | 0x80;
			i += 1;
		} else {
			*(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
			*(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		}
	}

	*dst = '\0';
}

static void Utf8ToUtf16(const uint8_t *src, size_t src_size, uint16_t *dst)
{
	for (size_t i = 0; i < src_size && src[i];) {
		if ((src[i] & 0xE0) == 0xE0) {
			if (i + 2 >= src_size) {
				break;
			}
			*(dst++) = ((src[i] & 0x0F) << 12) | ((src[i + 1] & 0x3F) << 6) | (src[i + 2] & 0x3F);
			i += 3;
		} else if ((src[i] & 0xC0) == 0xC0) {
			if (i + 1 >= src_size) {
				break;
			}
			*(dst++) = ((src[i] & 0x1F) << 6) | (src[i + 1] & 0x3F);
			i += 2;
		} else {
			*(dst++) = src[i];
			i += 1;
		}
	}

	*dst = '\0';
}



static SceWChar16 ime_out[SCE_IME_MAX_PREEDIT_LENGTH + SCE_IME_MAX_TEXT_LENGTH + 1] = {0};
static SceUInt8 ime_out_utf8[sizeof(ime_out)] = {0};
static SceWChar16 ime_inital[SCE_IME_MAX_TEXT_LENGTH] = { 0 };

static bool ime_is_open = false;
static int ime_carret_pos = -1;
static int ime_pos_x = -1;
static int ime_pos_y = -1;

static void ImeEventHandler(void *arg, const SceImeEventData *e)
{

	LOGI("e->id: %x\n", e->id);

	LOGI("e->param.rect.x: %i\n", e->param.rect.x);
	LOGI("e->param.rect.y: %i\n", e->param.rect.y);
	LOGI("e->param.rect.width: %i\n", e->param.rect.width);
	LOGI("e->param.rect.height: %i\n", e->param.rect.height);

	LOGI("e->param.text.preeditIndex: %i\n", e->param.text.preeditIndex);
	LOGI("e->param.text.preeditLength: %i\n", e->param.text.preeditLength);
	LOGI("e->param.text.caretIndex: %i\n", e->param.text.caretIndex);
	LOGI("e->param.text.str: %p\n", e->param.text.str);
	LOGI("e->param.text.editIndex: %i\n", e->param.text.editIndex);
	LOGI("e->param.text.editLengthChange: %i\n", e->param.text.editLengthChange);

	LOGI("e->param.caretIndex: %i\n", e->param.caretIndex);


	switch (e->id) {
		case SCE_IME_EVENT_OPEN:
			ime_pos_x = e->param.rect.x;
			ime_pos_y = e->param.rect.y;
			ime_is_open = true;
			break;
		case SCE_IME_EVENT_UPDATE_TEXT:
			ime_carret_pos = e->param.text.caretIndex;
			Utf16ToUtf8((SceWChar16 *)ime_out, (uint8_t*)ime_out_utf8);
			break;
		case SCE_IME_EVENT_UPDATE_CARET:
			ime_carret_pos = e->param.caretIndex;
			break;
		case SCE_IME_EVENT_CHANGE_SIZE:
			ime_pos_x = e->param.rect.x;
			ime_pos_y = e->param.rect.y;
			break;

		case SCE_IME_EVENT_PRESS_ENTER:
			sceImeClose();

			// reset ime
			ime_is_open = false;
			ime_carret_pos =  -1;
			ime_pos_y = -1;
			ime_pos_x = -1;
			break;
		case SCE_IME_EVENT_PRESS_CLOSE:
			sceImeClose();

			// reset ime
			ime_is_open = false;
			ime_carret_pos =  -1;
			ime_pos_y = -1;
			ime_pos_x = -1;
			break;
	}
}



class AppPlatform_Vita : public AppPlatform
{
public:

	int getScreenWidth() override { return width; }
	int getScreenHeight() override { return height; }

	bool supportsTouchscreen() override { return !isVitaTv; }

	void buyGame() override {

		int lang = 0;
		SceNpCountryCode code = { 0 };
		sceNpManagerGetAccountRegion(&code, &lang);

		std::string region = std::string((char*)code.data, sizeof(code.data));

		if (region == "jp") {
			sceAppMgrLaunchAppByUri(0x60000, "psts:browse?product=JP0127-PCSG00302_00-MINECRAFTVIT0000");
		} else if (region == "us" || region == "ca") {
			sceAppMgrLaunchAppByUri(0x60000, "psts:browse?product=UP4433-PCSE00491_00-MINECRAFTVIT0000");
		} else {
			sceAppMgrLaunchAppByUri(0x60000, "psts:browse?product=EP4433-PCSB00560_00-MINECRAFTVIT0000");
		}
	}

	void showKeyboard(std::string defaultText, int maxLength) override {
		if(maxLength < 0)
			maxLength = SCE_IME_MAX_TEXT_LENGTH;

		if(maxLength > SCE_IME_MAX_TEXT_LENGTH)
			maxLength = SCE_IME_MAX_TEXT_LENGTH;

		memset(ime_inital, 0x00, sizeof(ime_inital));
		Utf8ToUtf16((const uint8_t*)defaultText.c_str(), defaultText.size(), ime_inital);

		static SceUInt32 ime_workram[SCE_IME_WORK_BUFFER_SIZE / sizeof(SceInt32)] = {0};
	
		SceImeParam param;
		sceImeParamInit(&param);

		// fixed utf8 encode function so .. 
		// add support for all languages; 
	
		param.supportedLanguages = (SCE_IME_LANGUAGE_DANISH | SCE_IME_LANGUAGE_GERMAN | SCE_IME_LANGUAGE_ENGLISH | SCE_IME_LANGUAGE_SPANISH | SCE_IME_LANGUAGE_FRENCH | SCE_IME_LANGUAGE_ITALIAN | SCE_IME_LANGUAGE_DUTCH | SCE_IME_LANGUAGE_NORWEGIAN | SCE_IME_LANGUAGE_POLISH | SCE_IME_LANGUAGE_PORTUGUESE | SCE_IME_LANGUAGE_RUSSIAN | SCE_IME_LANGUAGE_FINNISH | SCE_IME_LANGUAGE_SWEDISH | SCE_IME_LANGUAGE_JAPANESE | SCE_IME_LANGUAGE_KOREAN | SCE_IME_LANGUAGE_SIMPLIFIED_CHINESE | SCE_IME_LANGUAGE_TRADITIONAL_CHINESE | SCE_IME_LANGUAGE_PORTUGUESE_BR | SCE_IME_LANGUAGE_ENGLISH_GB | SCE_IME_LANGUAGE_TURKISH);
		param.languagesForced = SCE_FALSE;
		
		param.type = SCE_IME_TYPE_DEFAULT;
		param.option = 0;

		param.inputTextBuffer = ime_out;
		param.maxTextLength = maxLength;
		param.enterLabel = SCE_IME_ENTER_LABEL_DEFAULT;
		param.handler = ImeEventHandler;
		param.filter = NULL;
		param.initialText = ime_inital;
		param.arg = NULL;
		param.work = ime_workram;

		sceImeOpen(&param);
		ime_is_open = true;
	}

	void hideKeyboard() override {
		sceImeClose();
		ime_is_open = false;
	}

	bool isKeyboardVisible() override {
		return ime_is_open;
	}

	std::string getKeyboardInput() override {
		Utf16ToUtf8(ime_out, ime_out_utf8);
		std::string ime_txt = std::string((char*)ime_out_utf8);
		return ime_txt;
	}

	int getKeyboardCarret() override {
		return ime_carret_pos;
	};

	int getKeyboardX() override {
		return ime_pos_x;
	};

	int getKeyboardY() override {
		return ime_pos_y;
	};

	void _tick() override {
		sceImeUpdate();
		VibrateController(0);
	}


	void vibrate(int milliSeconds) override {
		VibrateController(milliSeconds);
	}

	bool isPowerVR() override {
		return true;
	}

	std::string defaultUsername() override {
		SceNpId npid;
		int ret = sceNpManagerGetNpId(&npid);

		if(ret >= 0) {
			return std::string((char*)npid.onlineId.data, sizeof(npid.onlineId.data));
		}
		else {
			LOGI("Failed to read npid: %x\n", ret);

			// read from registry as a fallback
			char rd_username[0x100] = {0};
			ret = sceRegMgrGetKeyStr("/CONFIG/SYSTEM", "username", rd_username, sizeof(rd_username));
			if(ret >= 0) {
				return std::string(rd_username);
			}

		}

		return "Vita";
	}

	BinaryBlob readAssetFile(const std::string& filename) override {
		std::string fullAssetPath = ("app0:data/" + filename);

		LOGI("fullAssetPath: %s\n", fullAssetPath.c_str());
		SceIoStat stat;
		int ret = sceIoGetstat(fullAssetPath.c_str(), &stat);
		if(ret < 0) {
			LOGI("failed to stat: %x %s\n", ret,fullAssetPath.c_str());
			return BinaryBlob();
		}

		SceUID fd = sceIoOpen(fullAssetPath.c_str(), SCE_O_RDONLY, 0777);

		if(fd < 0) {
			LOGI("failed to open: %x %s\n", fd, fullAssetPath.c_str());
			return BinaryBlob();
		}


		BinaryBlob blob;
		blob.size = stat.st_size;
		blob.data = new unsigned char[blob.size];

		int rd = sceIoRead(fd, blob.data, blob.size);
		sceIoClose(fd);

		if((size_t)rd != blob.size) {
			LOGI("wrong size: %x %s\n", rd, fullAssetPath.c_str());

			return BinaryBlob();
		}

		LOGI("read %x bytes from %s\n", rd, fullAssetPath.c_str());

		return blob;
	}
	std::string getPlatformStringVar (int stringId) override {
		if(isVitaTv) return "PlayStation TV";
		else return "PlayStation Vita";
	}

	TextureData loadTexture(const std::string& filename_, bool textureFolder) override {
		TextureData out;

		std::string filename = textureFolder ? "data/images/" + filename_ : filename_;
		
		FILE* file = fopen(filename.c_str(), "rb");
		if(!file) {
			LOGI("Couldn't find file: %s\n", filename.c_str());
			return out;
		}
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
		out.width = png_get_image_width(pngPtr, infoPtr);
		out.height = png_get_image_height(pngPtr, infoPtr);

		if(out.height > 1000000) out.height = 1000000; // make gcc happy
		png_bytep* rowPtrs = new png_bytep[out.height];
		out.data = new unsigned char[4 * out.width * out.height];
		out.memoryHandledExternally = false;

		size_t rowStrideBytes = 4 * out.width;
		for (size_t i = 0; i < out.height; i++) {
			rowPtrs[i] = (png_bytep)&out.data[i*rowStrideBytes];
		}
		png_read_image(pngPtr, rowPtrs);

		// Teardown and return
		png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
		delete[] (png_bytep)rowPtrs;
		fclose(file);
		return out;
	}

private:
	int width = 960;
	int height = 544;
	bool isVitaTv = sceKernelIsPSVitaTV();
};

#endif
