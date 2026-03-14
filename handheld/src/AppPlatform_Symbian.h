#pragma once

#ifndef _SRC_APPPLATFORM_SYMBIAN_H_A2B650F7_2078_5E8F_B2DE_0544F8A73562
#define _SRC_APPPLATFORM_SYMBIAN_H_A2B650F7_2078_5E8F_B2DE_0544F8A73562

#include <e32cmn.h>
#include <hwrmvibra.h>

#include <lodepng.h>

#include "AppPlatform.h"

#include "NinecraftApp.h"

#include <algorithm>
#include <vector>
#include <cstdint>

struct AppPlatform_Symbian : AppPlatform {
	AppPlatform_Symbian() : iImeIsOpen(false) {
		TRAPD(err, (iVibra = CHWRMVibra::NewL()));
		if (err != KErrNone) {
			fprintf(stderr, "Vibra open failed: %d\n", err);
			iVibra = NULL;
		}
	}

	bool supportsTouchscreen() override { return true; }

	int getScreenWidth() override;
	int getScreenHeight() override;

	void showKeyboard() override { iImeIsOpen = true; }
	void hideKeyboard() override { iImeIsOpen = false; }

	bool isKeyboardVisible() override { return iImeIsOpen; }

	std::string getKeyboardInput() override {
		// TODO:XXX:
		return "";
	}

	bool isPowerVR() override { return false; }

	std::string defaultUsername() override { return "Symbian"; }

	BinaryBlob readAssetFile(const std::string &filename) override {
		BinaryBlob blob;

		const auto path = "/private/e0000666/data/" + filename;
		fprintf(stdout, "Try read asset: %s\n", path.c_str());

		auto fh = fopen(path.c_str(), "r");
		fprintf(stdout, "fh = %p\n", fh);

		if (!fh) {
			fprintf(stdout, "Does not exist?: %s\n", path.c_str());
			return blob;
		}
		fseek(fh, 0, SEEK_END);
		const auto sz = ftell(fh);
		fseek(fh, 0, SEEK_SET);

		auto buf = new uint8_t[sz];
		if (1 != fread(buf, sz, 1, fh)) {
			fprintf(stdout, "Read failed: %s\n", path.c_str());
			fclose(fh);
			delete[] buf;
			return blob;
		}

		fclose(fh);

		blob.size = sz;
		blob.data = buf;

		fprintf(stdout, "RdOK: %s (sz = %d)\n", path.c_str(), sz);
		return blob;
	}

	TextureData loadTexture(const std::string &filename_, bool textureFolder) override {
		const std::string filename = textureFolder ? "/private/e0000666/data/images/" + filename_ : filename_;
		fprintf(stdout, "Try load texfile: %s\n", filename.c_str());

		TextureData out;
		unsigned err;
		std::vector<uint8_t> pmap;
		if ((err = lodepng::decode(pmap, reinterpret_cast<unsigned int &>(out.w), reinterpret_cast<unsigned int &>(out.h), filename))) {
			fprintf(stdout, "Error %d loading texfile %s\n", err, filename.c_str());
			return out;
		}

		out.memoryHandledExternally = false;

		out.data = new uint8_t[pmap.size()];
		std::copy(pmap.begin(), pmap.end(), out.data);

		fprintf(stdout, "TRdOK: %s\n", filename.c_str());
		return out;
	}

	void vibrate(int millis) override {
		if (iVibra) {
			TRAPD(err, iVibra->StartVibraL(millis));
			(void)err;
		}
	}

private:
	bool iImeIsOpen;
	CHWRMVibra *iVibra;
};

#endif

