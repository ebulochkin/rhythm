#pragma once
#include <vector>

enum class Judge { MISS = 0, BAD = 1, GOOD = 2, GREAT = 3, PERFECT = 4 };

struct Note {
    int lane;       // 0..3
    double timeSec; // hit time in seconds
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
    std::vector<Note> notes;
    void sort_notes();
};

Judge judge_from_abs_dt(double absDt, const HitWindows &w);
const char *judge_name(Judge j);

// gameplay helpers
bool try_hit_lane(Chart &chart, int lane, double songTimeSec,
                  const HitWindows &w, struct ScoreState &score);

void update_auto_miss(Chart &chart, double songTimeSec,
                      const HitWindows &w, struct ScoreState &score);

// optional: make a simple hardcoded chart (как у тебя)
Chart make_test_chart();