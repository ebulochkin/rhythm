#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

typedef long long int lli;

static double NowSec() { return GetTime(); }

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

static Judge judge_from_abs_dt(double absDt, const HitWindows &w) {
  if (absDt <= w.perfect)
    return Judge::PERFECT;
  if (absDt <= w.great)
    return Judge::GREAT;
  if (absDt <= w.good)
    return Judge::GOOD;
  if (absDt <= w.bad)
    return Judge::BAD;
  return Judge::MISS;
}

struct ScoreState {
  lli score = 0;
  lli combo = 0;
  lli maxCombo = 0;

  // heat
  double heat = 0.0; // 0..100
  int heatLevel = 0; // 0..3

  // last judgement text
  Judge lastJudge = Judge::MISS;
  double lastDtMs = 0.0;

  void apply_hit(Judge j, double dtSec) {
    lastJudge = j;
    lastDtMs = dtSec * 1000.0;

    if (j == Judge::MISS) {
      combo = 0;
      heat *= 0.5; // мягкий сброс
      return;
    }

    combo += 1;
    if (combo > maxCombo)
      maxCombo = combo;

    // scoring (примерно)
    if (j == Judge::PERFECT)
      score += 1000;
    if (j == Judge::GREAT)
      score += 700;
    if (j == Judge::GOOD)
      score += 400;
    if (j == Judge::BAD)
      score += 150;

    // heat gain
    if (j == Judge::PERFECT)
      heat += 6.0;
    if (j == Judge::GREAT)
      heat += 4.0;
    if (j == Judge::GOOD)
      heat += 2.0;
    if (j == Judge::BAD)
      heat += 1.0;

    if (heat > 100.0)
      heat = 100.0;
  }

  void apply_miss_auto() {
    lastJudge = Judge::MISS;
    lastDtMs = 999.0;
    combo = 0;
    heat *= 0.5;
  }

  void update_heat_level() {
    if (heat < 25.0)
      heatLevel = 0;
    else if (heat < 50.0)
      heatLevel = 1;
    else if (heat < 75.0)
      heatLevel = 2;
    else
      heatLevel = 3;
  }

  void passive_decay(double dt) {
    // лёгкое затухание, чтобы heat не висел вечно
    // можно выключить если не надо
    heat -= dt * 2.0; // 2 units per second
    if (heat < 0.0)
      heat = 0.0;
  }
};

struct Chart {
  std::vector<Note> notes;

  // notes отсортированы по timeSec
  void sort_notes() {
    std::sort(notes.begin(), notes.end(), [](const Note &a, const Note &b) {
      if (a.timeSec != b.timeSec)
        return a.timeSec < b.timeSec;
      return a.lane < b.lane;
    });
  }
};

// Находим ближайшую (самую раннюю не hit/missed) ноту в lane, и пытаемся её
// захитить.
static bool try_hit_lane(Chart &chart, int lane, double songTimeSec,
                         const HitWindows &w, ScoreState &score) {
  int idx = -1;
  for (int i = 0; i < (int)chart.notes.size(); i++) {
    auto &n = chart.notes[i];
    if (n.lane != lane)
      continue;
    if (n.hit || n.missed)
      continue;
    idx = i;
    break;
  }
  if (idx == -1)
    return false;

  auto &n = chart.notes[idx];
  double dt = songTimeSec - n.timeSec; // >0 значит поздно, <0 рано
  double absDt = std::abs(dt);

  Judge j = judge_from_abs_dt(absDt, w);
  if (j == Judge::MISS) {
    // В osu/fnf обычно на нажатие далеко от ноты — просто "ничего" или "bad
    // hit". Я сделаю "ничего", чтобы не штрафовать спамом:
    return false;
  }

  n.hit = true;
  score.apply_hit(j, dt);
  return true;
}

// Авто-miss: если songTime ушло дальше, чем bad window (или отдельный miss
// window)
static void update_auto_miss(Chart &chart, double songTimeSec,
                             const HitWindows &w, ScoreState &score) {
  for (auto &n : chart.notes) {
    if (n.hit || n.missed)
      continue;
    if (songTimeSec - n.timeSec > w.bad) {
      n.missed = true;
      score.apply_miss_auto();
    } else {
      // notes отсортированы; можно break, но lanes перемешаны, поэтому оставим
      // так
    }
  }
}

static const char *judge_name(Judge j) {
  if (j == Judge::PERFECT)
    return "PERFECT";
  if (j == Judge::GREAT)
    return "GREAT";
  if (j == Judge::GOOD)
    return "GOOD";
  if (j == Judge::BAD)
    return "BAD";
  return "MISS";
}

int main() {
  const int W = 900;
  const int H = 600;
  InitWindow(W, H, "4K rhythm base (raylib)");
  InitAudioDevice();
  SetTargetFPS(240);

  // === загрузка музыки (опционально) ===
  // Положи файл рядом: song.ogg или song.wav
  Music music = {};
  bool hasMusic = false;
  if (FileExists("song.ogg")) {
    music = LoadMusicStream("song.ogg");
    hasMusic = true;
  } else if (FileExists("song.wav")) {
    music = LoadMusicStream("song.wav");
    hasMusic = true;
  }

  // === чарт (пока хардкод) ===
  // В секундах, можно сделать парсер позже.
  Chart chart;
  double t = 1.0;
  for (int i = 0; i < 64; i++) {
    chart.notes.push_back({i % 4, t, false, false});
    t += 0.35; // интервал нот
  }
  chart.sort_notes();

  HitWindows win;
  ScoreState score;

  // клавиши под 4k
  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};

  bool started = false;
  double startTime = 0.0; // reference for song time
  double lastFrame = NowSec();

  while (!WindowShouldClose()) {
    double now = NowSec();
    double dt = now - lastFrame;
    lastFrame = now;

    if (!started) {
      if (IsKeyPressed(KEY_SPACE)) {
        started = true;
        startTime = NowSec();
        if (hasMusic) {
          PlayMusicStream(music);
        }
      }
    }

    double songTime = 0.0;
    if (started) {
      if (hasMusic)
        UpdateMusicStream(music);

      // главный таймер: системное время с момента старта.
      // (в идеале можно слегка подправлять по GetMusicTimePlayed)
      songTime = NowSec() - startTime;

      // пассивный decay heat
      score.passive_decay(dt);

      // input hits
      for (int lane = 0; lane < 4; lane++) {
        if (IsKeyPressed(keys[lane])) {
          try_hit_lane(chart, lane, songTime, win, score);
        }
      }

      // auto misses
      update_auto_miss(chart, songTime, win, score);

      score.update_heat_level();
    }

    // === render ===
    BeginDrawing();
    ClearBackground(BLACK);

    // lanes
    int laneW = W / 6;
    int laneStartX = W / 2 - 2 * laneW;
    int hitLineY = (int)(H * 0.80);

    for (int i = 0; i < 4; i++) {
      Rectangle r{(float)(laneStartX + i * laneW), 0.0f, (float)laneW,
                  (float)H};
      DrawRectangleLinesEx(r, 2, DARKGRAY);

      // key state
      bool down = IsKeyDown(keys[i]);
      if (down) {
        DrawRectangle((int)r.x, hitLineY, laneW, 12, DARKGRAY);
      }
    }

    // hit line
    DrawLine(laneStartX, hitLineY, laneStartX + 4 * laneW, hitLineY, GRAY);

    // notes draw (simple: scroll speed)
    double scroll = 320.0; // px per second
    for (auto &n : chart.notes) {
      if (n.hit || n.missed)
        continue;

      double dtToHit = n.timeSec - songTime; // >0 upcoming
      double y = hitLineY - dtToHit * scroll;

      // cull
      if (y < -50 || y > H + 50)
        continue;

      int x = laneStartX + n.lane * laneW + laneW / 2;
      DrawCircle(x, (int)y, 14, RAYWHITE);
    }

    // UI
    DrawText(started ? "SPACE started" : "Press SPACE to start", 20, 20, 20,
             RAYWHITE);

    std::string s1 = "Score: " + std::to_string(score.score);
    std::string s2 = "Combo: " + std::to_string(score.combo) + " (max " +
                     std::to_string(score.maxCombo) + ")";
    std::string s3 = std::string("Judge: ") + judge_name(score.lastJudge) +
                     "  dt=" + std::to_string((int)score.lastDtMs) + "ms";
    std::string s4 = "Heat: " + std::to_string((int)score.heat) +
                     "  Level: " + std::to_string(score.heatLevel);

    DrawText(s1.c_str(), 20, 60, 20, RAYWHITE);
    DrawText(s2.c_str(), 20, 90, 20, RAYWHITE);
    DrawText(s3.c_str(), 20, 120, 20, RAYWHITE);
    DrawText(s4.c_str(), 20, 150, 20, RAYWHITE);

    // song time
    if (started) {
      std::string st = "t=" + std::to_string(songTime);
      DrawText(st.c_str(), 20, 180, 20, GRAY);
    }

    EndDrawing();
  }

  if (hasMusic)
    UnloadMusicStream(music);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
