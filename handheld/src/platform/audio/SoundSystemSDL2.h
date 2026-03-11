#ifndef SoundSystemSDL_H__
#define SoundSystemSDL_H__

#include "SoundSystem.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

class SoundSystemSDL : public SoundSystem
{
public:
    SoundSystemSDL();
    virtual ~SoundSystemSDL();

    virtual void init();
    virtual void destroy();

    virtual void setListenerPos(float x, float y, float z);
    virtual void setListenerAngle(float deg);

    virtual void load(const std::string& name) {}
    virtual void play(const std::string& name) {}
    virtual void pause(const std::string& name) {}
    virtual void stop(const std::string& name) {}
    virtual void playAt(const SoundDesc& sound, float x, float y, float z, float volume, float pitch);

private:
    static const int NUM_VOICES = 16;

    Mix_Chunk* _chunks[NUM_VOICES];
    int _voiceIndex;

    bool available;
};

#endif /*SoundSystemSDL_H__*/