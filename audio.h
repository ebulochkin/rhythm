#pragma once
#include "raylib.h"
#include <string>

struct AudioPlayer {
    Music music{};
    bool hasMusic = false;

    bool started = false;
    double startTime = 0.0; // GetTime() at start

    bool load_auto(const std::string &ogg = "song.ogg",
                   const std::string &wav = "song.wav");

    void start();      // starts timer + plays if loaded
    void update();     // UpdateMusicStream
    void unload();

    double song_time() const; // seconds since start
};