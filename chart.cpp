#include "chart.h"
#include "score.h"
#include <algorithm>
#include <cmath>

void Chart::sort_notes() {
    std::sort(notes.begin(), notes.end(), [](const Note &a, const Note &b) {
      if (a.timeSec != b.timeSec) return a.timeSec < b.timeSec;
      return a.lane < b.lane;
    });
}

Judge judge_from_abs_dt(double absDt, const HitWindows &w) {
    if (absDt <= w.perfect) return Judge::PERFECT;
    if (absDt <= w.great)   return Judge::GREAT;
    if (absDt <= w.good)    return Judge::GOOD;
    if (absDt <= w.bad)     return Judge::BAD;
    return Judge::MISS;
}

const char *judge_name(Judge j) {
    if (j == Judge::PERFECT) return "PERFECT";
    if (j == Judge::GREAT)   return "GREAT";
    if (j == Judge::GOOD)    return "GOOD";
    if (j == Judge::BAD)     return "BAD";
    return "MISS";
}

bool try_hit_lane(Chart &chart, int lane, double songTimeSec,
                  const HitWindows &w, ScoreState &score) {
    int idx = -1;
    for (int i = 0; i < (int)chart.notes.size(); i++) {
        auto &n = chart.notes[i];
        if (n.lane != lane) continue;
        if (n.hit || n.missed) continue;
        idx = i;
        break;
    }
    if (idx == -1) return false;

    auto &n = chart.notes[idx];
    double dt = songTimeSec - n.timeSec; // >0 поздно, <0 рано
    double absDt = std::abs(dt);

    Judge j = judge_from_abs_dt(absDt, w);
    if (j == Judge::MISS) {
        // не штрафуем за спам далеко от ноты
        return false;
    }

    n.hit = true;
    score.apply_hit(j, dt);
    return true;
}

void update_auto_miss(Chart &chart, double songTimeSec,
                      const HitWindows &w, ScoreState &score) {
    for (auto &n : chart.notes) {
        if (n.hit || n.missed) continue;
        if (songTimeSec - n.timeSec > w.bad) {
            n.missed = true;
            score.apply_miss_auto();
        }
    }
}

Chart make_test_chart() {
    Chart chart;
    double t = 1.0;
    for (int i = 0; i < 64; i++) {
        chart.notes.push_back({i % 4, t, false, false});
        t += 0.35;
    }
    chart.sort_notes();
    return chart;
}