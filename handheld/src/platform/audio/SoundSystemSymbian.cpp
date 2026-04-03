#include "SoundSystemSymbian.h"

#include "../../client/sound/Sound.h"

#include <e32cmn.h>
#include <e32hal.h>
#include <e32event.h>

#include <mdaaudiosampleeditor.h>
#include <mdaaudiooutputstream.h>
#include <mda/common/audio.h>
#include <mmf/common/mmfutilities.h>

#include <cmixer.h>

#include <container.h>

#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>

#define BUFFER_SIZE (512ul)

struct SoundHandlerSymbian : MMdaAudioOutputStreamCallback {
	SoundHandlerSymbian(SoundSystemSymbian *aParent) :
			iParent(aParent),
			iOutputStream(NULL),
			iBufferPtr(reinterpret_cast<TUint8 *>(iBuffer), sizeof iBuffer) {
		cm_init(44100);

		iSettings.iChannels = TMdaAudioDataSettings::EChannelsStereo;
		iSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate44100Hz;
		iSettings.iCaps = TMdaAudioDataSettings::ERealTime | TMdaAudioDataSettings::ESampleRateFixed;

		TRAPD(err, iOutputStream = CMdaAudioOutputStream::NewL(*this));
		if (err) {
			_LIT(KOutputStreamFailed, "Audio out failed");
			User::Panic(KOutputStreamFailed, 0);
		}

		iOutputStream->Open(&iSettings);
	}

	virtual ~SoundHandlerSymbian();

	void MaoscOpenComplete(TInt aError) override;

	void MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer) override;

	void MaoscPlayComplete(TInt aError) override;

	void PruneSources(bool all = false);

	void RequestSound();

	SoundSystemSymbian *iParent;

	TMdaAudioDataSettings iSettings;
	CMdaAudioOutputStream *iOutputStream;
	std::vector<cm_Source *> iSources;

	cm_Int16 iBuffer[BUFFER_SIZE];
	TPtrC8 iBufferPtr;
};

SoundSystemSymbian::SoundSystemSymbian() :
		iListenerRot(.0f),
		iListenerPos(.0f, .0f, .0f) {
	iHandler = new SoundHandlerSymbian(this);
}

SoundSystemSymbian::~SoundSystemSymbian() { delete iHandler; }

SoundHandlerSymbian::~SoundHandlerSymbian() {
	auto pCont = CMcpeContainer::instance();

	if (iOutputStream) {
		if (pCont->iOutputStatus != CMcpeContainer::ENotReady) {
			iOutputStream->Stop();
		}
		delete iOutputStream;
	}
}

void SoundHandlerSymbian::PruneSources(bool all) {
	for (auto &s : iSources) {
		if (s && (all || (cm_get_state(s) == CM_STATE_STOPPED))) {
			cm_destroy_source(s);
			s = NULL;
		}
	}

	decltype(iSources.end()) it;
	while (iSources.end() != (it = std::find_if(
			iSources.begin(), iSources.end(),
			[](cm_Source *src) { return src == NULL; }))) {
		iSources.erase(it);
	}
}

void SoundSystemSymbian::enable(bool status) {
	if (!status) { iHandler->PruneSources(true); }
}

namespace {

struct TSndBufRq {
	TSndBufRq() : iOffset(0) {}
	~TSndBufRq() = default;

	TInt32 iOffset;
	std::vector<cm_Int16> iBuffer;
};
}

void SoundSystemSymbian::playAt(const SoundDesc &sound, float x, float y, float z, float volume, float pitch) {
	//fprintf(stderr, "add sound: %d frames @ %dHz; vol = %.3f, pitch = %.3f; at %.3f,%.3f,%.3f\n", sound.size, sound.frameRate, volume, pitch, x, y, z);
	const auto length = sound.size / sound.channels / 2;

	auto rq = new TSndBufRq;
	const auto framesInt16 = reinterpret_cast<const cm_Int16 *>(sound.frames);

	if (sound.channels == 2) {
		std::copy(framesInt16, framesInt16 + (length * sound.channels), std::back_inserter(rq->iBuffer));
	} else if (sound.channels == 1) {
		for (unsigned i = 0; i < length; ++i) {
			rq->iBuffer.push_back(framesInt16[i]);
			rq->iBuffer.push_back(framesInt16[i]);
		}
	} else {
		return;
	}

	cm_SourceInfo info{
		.handler = +[](cm_Event *ev) {
			auto rq = reinterpret_cast<TSndBufRq *>(ev->udata);
			switch (ev->type) {
			case CM_EVENT_REWIND: {
				rq->iOffset = 0;
			} break;
			case CM_EVENT_SAMPLES: {
				const auto pBegin = rq->iBuffer.begin() + rq->iOffset;
				if (pBegin >= rq->iBuffer.end()) { break; }

				auto pEnd = pBegin + ev->length;
				if (pEnd > rq->iBuffer.end()) {
					pEnd = rq->iBuffer.end();
				}

				std::copy(pBegin, pEnd, ev->buffer);
				const auto diff = ev->length - std::distance(pBegin, pEnd);
				if (diff > 0) {
					auto p = ev->buffer + ev->length;
					std::fill(p - diff, p, 0);
				}

				rq->iOffset = std::distance(rq->iBuffer.begin(), pEnd);
			} break;
			case CM_EVENT_DESTROY: {
				delete rq;
			} break;
			}
		},
		.udata = rq,
		.samplerate = sound.frameRate,
		.length = length,
	};

	auto src = cm_new_source(&info);
	if (src) {
		if (x || y || z) {
			const Vec3 sound_pos(x, y, z);
			const Vec3 dist_v = sound_pos - iListenerPos;
			const float dist = std::sqrt(dist_v.x*dist_v.x + dist_v.y*dist_v.y + dist_v.z*dist_v.z);
			const float rad = (iListenerRot + 90.0f) * Mth::DEGRAD;
			const float lx = -Mth::sin(rad);
			const float lz = Mth::cos(rad);
			const float nx = (dist > 0.001f) ? dist_v.x / dist : 0.0f;
			const float nz = (dist > 0.001f) ? dist_v.z / dist : 0.0f;
			const float pan = nx * lx + nz * lz;

			cm_set_pan(src, pan);
		}
		cm_set_gain(src, volume);
		if (pitch >= .01f) { cm_set_pitch(src, pitch); }

		cm_play(src);
		iHandler->iSources.push_back(src);
	}
}

void SoundHandlerSymbian::RequestSound() {
	auto pCont = CMcpeContainer::instance();

	if (pCont->iOutputStatus == CMcpeContainer::ESetVolume) {
		pCont->iOutputStatus = CMcpeContainer::EOpen;
		if (pCont->iVolume < 0)
			pCont->iVolume = 0;
		if (pCont->iVolume > iOutputStream->MaxVolume())
			pCont->iVolume = iOutputStream->MaxVolume();

		iOutputStream->SetVolume(pCont->iVolume);
	}

	//memset(reinterpret_cast<void *>(iBuffer), 0, sizeof iBuffer);
	cm_process(iBuffer, BUFFER_SIZE);

	iOutputStream->WriteL(iBufferPtr);
}

void SoundHandlerSymbian::MaoscOpenComplete(TInt aError) {
	auto pCont = CMcpeContainer::instance();

	if (aError == KErrNone) {
		//pCont->iVolume = iOutputStream->MaxVolume() / 2;
		pCont->iVolume = iOutputStream->MaxVolume();
		pCont->iVolumeStep = iOutputStream->MaxVolume() / 8;

		if (pCont->iVolumeStep == 0) {
			pCont->iVolumeStep = 1;
		}

		iOutputStream->SetVolume(pCont->iVolume);
		iOutputStream->SetPriority(EPriorityNormal, EMdaPriorityPreferenceTime);

		pCont->iOutputStatus = CMcpeContainer::EOpen;

		RequestSound();
	} else {
		pCont->iOutputStatus = CMcpeContainer::ENotReady;
	}
}

void SoundHandlerSymbian::MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer) {
	auto pCont = CMcpeContainer::instance();

	PruneSources();

	if (aError == KErrAbort) {
		pCont->iOutputStatus = CMcpeContainer::ENotReady;
	} else if (pCont->iOutputStatus != CMcpeContainer::ENotReady) {
		RequestSound();
	}
}

void SoundHandlerSymbian::MaoscPlayComplete(TInt aError) {
	CMcpeContainer::instance()->iOutputStatus = CMcpeContainer::ENotReady;
}
