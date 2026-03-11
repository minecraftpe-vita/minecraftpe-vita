#include "SoundSystemSDL2.h"
#include "../../util/Mth.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/phys/Vec3.h"
#include "../../client/sound/Sound.h"
#include "../log.h"
#include <cstring>

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};
#pragma pack(pop)

SoundSystemSDL::SoundSystemSDL()
:   available(false),
    _voiceIndex(0)
{
    for (int i = 0; i < NUM_VOICES; i++) {
        _chunks[i] = nullptr;
    }
    init();
}

SoundSystemSDL::~SoundSystemSDL()
{
    destroy();
}

void SoundSystemSDL::init()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        LOGI("SDL Audio Init failed: %s\n", SDL_GetError());
        return;
    }

    if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        LOGI("SDL_mixer OpenAudio failed: %s\n", Mix_GetError());
        return;
    }

    Mix_AllocateChannels(NUM_VOICES);

    available = true;
    LOGI("SoundSystemSDL initialized successfully!\n");
}

void SoundSystemSDL::destroy()
{
    if (!available) return;

    for (int i = 0; i < NUM_VOICES; i++) {
        Mix_HaltChannel(i);
        if (_chunks[i] != nullptr) {
            Mix_FreeChunk(_chunks[i]);
            _chunks[i] = nullptr;
        }
    }

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    available = false;
}

void SoundSystemSDL::setListenerPos(float x, float y, float z) {}
void SoundSystemSDL::setListenerAngle(float deg) {}

void SoundSystemSDL::playAt(const SoundDesc& sound, float x, float y, float z, float volume, float pitch)
{
    if (!available) return;
    if (pitch < 0.01f) pitch = 1.0f;

    int channel = _voiceIndex;
    _voiceIndex++;
    if (_voiceIndex >= NUM_VOICES) {
        _voiceIndex = 0;
    }

    Mix_HaltChannel(channel);
    if (_chunks[channel] != nullptr) {
        Mix_FreeChunk(_chunks[channel]);
        _chunks[channel] = nullptr;
    }

    WAVHeader header;
    header.numChannels = sound.channels;
    header.sampleRate = (uint32_t)(sound.frameRate * pitch);
    header.bitsPerSample = sound.byteWidth * 8;
    header.byteRate = header.sampleRate * header.numChannels * sound.byteWidth;
    header.blockAlign = header.numChannels * sound.byteWidth;
    header.dataSize = sound.size;
    header.fileSize = 36 + header.dataSize;

    size_t totalMemSize = sizeof(WAVHeader) + sound.size;
    uint8_t* wavBuffer = (uint8_t*)malloc(totalMemSize);

    memcpy(wavBuffer, &header, sizeof(WAVHeader));
    memcpy(wavBuffer + sizeof(WAVHeader), sound.frames, sound.size);

    // Загружаем в SDL_mixer
    SDL_RWops* rw = SDL_RWFromMem(wavBuffer, totalMemSize);
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);
    free(wavBuffer);

    if (!chunk) return;

    _chunks[channel] = chunk;

    Mix_SetPosition(channel, 0, 0); // Сбрасываем эффекты на всякий случай
    Mix_Volume(channel, (int)(volume * MIX_MAX_VOLUME));

    Mix_PlayChannel(channel, chunk, 0);
}