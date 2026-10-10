#ifndef AUDIO_H
#define AUDIO_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "raylib.h"

class Audio {
private:
    static constexpr size_t SOUND_POOL_SIZE = 15;

public:
    enum class MusicId { MENU, GAME, NONE };

    enum class SoundId { EXPLOSION, HIT };

private:
    std::unordered_map<Audio::MusicId, Music> music;
    std::unordered_map<Audio::SoundId, Sound> loadedSounds;
    std::unordered_map<Audio::SoundId, std::vector<Sound>> soundPools;

    MusicId activeMusic = MusicId::NONE;

public:
    Audio();
    ~Audio();

    void loadMusic(MusicId id, const char* path, float volume);
    void loadSound(SoundId id, const char* path);

    void playMusic(MusicId id);
    void playSound(SoundId id);

    void stopMusic(MusicId id);

    void update(void);
};

#endif