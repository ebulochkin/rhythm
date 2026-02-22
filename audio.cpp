#include "audio.h"

bool AudioPlayer::load_auto(const std::string &ogg, const std::string &wav) {
    hasMusic = false;

    if (FileExists(ogg.c_str())) {
        music = LoadMusicStream(ogg.c_str());
        hasMusic = true;
    } else if (FileExists(wav.c_str())) {
        music = LoadMusicStream(wav.c_str());
        hasMusic = true;
    }
    return hasMusic;
}

void AudioPlayer::start() {
    started = true;
    startTime = GetTime();
    if (hasMusic) PlayMusicStream(music);
}

void AudioPlayer::update() {
    if (started && hasMusic) UpdateMusicStream(music);
}

double AudioPlayer::song_time() const {
    if (!started) return 0.0;
    // базово как было у тебя
    return GetTime() - startTime;
}

void AudioPlayer::unload() {
    if (hasMusic) UnloadMusicStream(music);
    hasMusic = false;
}