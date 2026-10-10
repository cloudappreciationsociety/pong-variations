#include "audio.h"

Audio::Audio() {
    InitAudioDevice();
}

Audio::~Audio() {
    for (auto& pair : this->music) {
        UnloadMusicStream(pair.second);
    }

    for (auto& pair : this->soundPools) {
        for (Sound& alias : pair.second) {
            UnloadSoundAlias(alias);
        }
    }

    for (auto& pair : this->loadedSounds) {
        UnloadSound(pair.second);
    }

    CloseAudioDevice();
}

void Audio::loadMusic(MusicId id, const char* path, float volume) {
    this->music.emplace(id, LoadMusicStream(path));
    SetMusicVolume(this->music[id], volume);
}

void Audio::loadSound(SoundId id, const char* path) {
    this->loadedSounds.emplace(id, LoadSound(path));

    for (size_t i = 0; i < Audio::SOUND_POOL_SIZE; ++i) {
        this->soundPools[id].push_back(LoadSoundAlias(this->loadedSounds[id]));
    }
}

void Audio::playMusic(MusicId id) {
    if (this->activeMusic == id) {
        return;
    }

    if (this->activeMusic != MusicId::NONE) {
        StopMusicStream(this->music[this->activeMusic]);
    }

    this->activeMusic = id;
    PlayMusicStream(this->music[id]);
}

void Audio::stopMusic(MusicId id) {
    if (this->activeMusic != id) {
        return;
    }

    StopMusicStream(this->music[id]);
    this->activeMusic = MusicId::NONE;
}

void Audio::playSound(SoundId id) {
    for (size_t i = 0; i < this->soundPools[id].size(); ++i) {
        if (!IsSoundPlaying(this->soundPools[id][i])) {
            PlaySound(this->soundPools[id][i]);
            break;
        }
    }
}

void Audio::update(void) {
    if (this->activeMusic != MusicId::NONE) {
        UpdateMusicStream(this->music[this->activeMusic]);
    }
}