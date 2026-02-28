#pragma once
#include <cmath>
#include <vector>

enum class Judge { MISS = 0, BAD = 1, GOOD = 2, GREAT = 3, PERFECT = 4 };

struct TimingMap {
    double bpm = 120.0;
    int beatsPerBar = 4; // e.g. 4 for 4/4
    double offsetSec = 0.0; // chart beat 0 offset from audio start

    double seconds_per_beat() const { return 60.0 / bpm; }

    double beat_from_song_time(double songTimeSec) const {
        return (songTimeSec - offsetSec) / seconds_per_beat();
    }

    double song_time_from_beat(double beat) const {
        return offsetSec + beat * seconds_per_beat();
    }

    int bar_from_beat(double beat) const {
        return (int)std::floor(beat / (double)beatsPerBar);
    }

    double beat_in_bar(double beat) const {
        double local = std::fmod(beat, (double)beatsPerBar);
        if (local < 0.0) local += (double)beatsPerBar;
        return local;
    }
};

struct Note {
    int lane;       // 0..3
    double beat;    // hit time in beats
    bool hit = false;
    bool missed = false;
};

struct HitWindows {
    double perfect = 0.025; // 25ms
    double great = 0.045;   // 45ms
    double good = 0.080;    // 80ms
    double bad = 0.120;     // 120ms
};

struct Chart {
    TimingMap timing;
    std::vector<Note> notes;
    void sort_notes();
};

Judge judge_from_abs_dt(double absDt, const HitWindows &w);
const char *judge_name(Judge j);

// gameplay helpers
bool try_hit_lane(Chart &chart, int lane, double songBeat,
                  const HitWindows &w, struct ScoreState &score);

void update_auto_miss(Chart &chart, double songBeat,
                      const HitWindows &w, struct ScoreState &score);

// optional: make a simple hardcoded chart (как у тебя)
Chart make_test_chart();
