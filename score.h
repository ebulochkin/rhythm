#pragma once
#include "chart.h"

typedef long long int lli;

struct ScoreState {
    lli score = 0;
    lli combo = 0;
    lli maxCombo = 0;

    double heat = 0.0; // 0..100
    int heatLevel = 0; // 0..3

    Judge lastJudge = Judge::MISS;
    double lastDtMs = 0.0;

    void apply_hit(Judge j, double dtSec) {
        lastJudge = j;
        lastDtMs = dtSec * 1000.0;

        if (j == Judge::MISS) {
            combo = 0;
            heat *= 0.5;
            return;
        }

        combo += 1;
        if (combo > maxCombo) maxCombo = combo;

        if (j == Judge::PERFECT) score += 1000;
        if (j == Judge::GREAT)   score += 700;
        if (j == Judge::GOOD)    score += 400;
        if (j == Judge::BAD)     score += 150;

        if (j == Judge::PERFECT) heat += 6.0;
        if (j == Judge::GREAT)   heat += 4.0;
        if (j == Judge::GOOD)    heat += 2.0;
        if (j == Judge::BAD)     heat += 1.0;

        if (heat > 100.0) heat = 100.0;
    }

    void apply_miss_auto() {
        lastJudge = Judge::MISS;
        lastDtMs = 999.0;
        combo = 0;
        heat *= 0.5;
    }

    void update_heat_level() {
        if (heat < 25.0) heatLevel = 0;
        else if (heat < 50.0) heatLevel = 1;
        else if (heat < 75.0) heatLevel = 2;
        else heatLevel = 3;
    }

    void passive_decay(double dt) {
        heat -= dt * 2.0;
        if (heat < 0.0) heat = 0.0;
    }
};