#pragma once

#ifndef _SRC_PLATFORM_AUDIO_SOUNDSYSTEMSYMBIAN_H_DFAA47F5_BF5A_5D48_85DD_C7ED719CCC05
#define _SRC_PLATFORM_AUDIO_SOUNDSYSTEMSYMBIAN_H_DFAA47F5_BF5A_5D48_85DD_C7ED719CCC05

#include "SoundSystem.h"

#include "../../util/Mth.h"

struct SoundHandlerSymbian;

struct SoundSystemSymbian : SoundSystem {
	SoundSystemSymbian();
	~SoundSystemSymbian() override;

	void enable(bool status) override;

	void playAt(const SoundDesc &sound, float x, float y, float z, float volume, float pitch) override;

	inline bool isAvailable() override { return true; }

	inline void setListenerPos(float x, float y, float z) override { iListenerPos = Vec3(x, y, z); }
	inline void setListenerAngle(float deg) override { iListenerRot = deg; }

private:
	float iListenerRot;
	Vec3 iListenerPos;

	SoundHandlerSymbian *iHandler;
};

#endif
