#include "raylib.h"
#include <cmath>
#include <string>

#include "audio.h"
#include "chart.h"
#include "score.h"

int main() {
  const int W = 900;
  const int H = 600;
  InitWindow(W, H, "4K rhythm base (raylib)");
  InitAudioDevice();
  SetTargetFPS(240);

  AudioPlayer audio;
  audio.load_auto(); // song.ogg / song.wav рядом

  Chart chart = make_test_chart();
  HitWindows win;
  ScoreState score;

  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};
  double lastFrame = GetTime();

  while (!WindowShouldClose()) {
    double now = GetTime();
    double dt = now - lastFrame;
    lastFrame = now;

    if (!audio.started) {
      if (IsKeyPressed(KEY_SPACE)) audio.start();
    }

    double songTimeSec = audio.song_time();
    double songBeat = chart.timing.beat_from_song_time(songTimeSec);

    if (audio.started) {
      audio.update();

      score.passive_decay(dt);

        for (int lane = 0; lane < 4; lane++) {
        if (IsKeyPressed(keys[lane])) {
          try_hit_lane(chart, lane, songBeat, win, score);
        }
      }

      update_auto_miss(chart, songBeat, win, score);
      score.update_heat_level();
    }

    // ===== render =====
    BeginDrawing();
    ClearBackground(BLACK);

    int laneW = W / 6;
    int laneStartX = W / 2 - 2 * laneW;
    int hitLineY = (int)(H * 0.80);

    for (int i = 0; i < 4; i++) {
      Rectangle r{(float)(laneStartX + i * laneW), 0.0f, (float)laneW, (float)H};
      DrawRectangleLinesEx(r, 2, DARKGRAY);

      if (IsKeyDown(keys[i])) {
        DrawRectangle((int)r.x, hitLineY, laneW, 12, DARKGRAY);
      }
    }

    DrawLine(laneStartX, hitLineY, laneStartX + 4 * laneW, hitLineY, GRAY);

    double scrollPxPerBeat = 96.0;
    for (auto &n : chart.notes) {
      if (n.hit || n.missed) continue;

      double beatToHit = n.beat - songBeat;
      double y = hitLineY - beatToHit * scrollPxPerBeat;

      if (y < -50 || y > H + 50) continue;

      int x = laneStartX + n.lane * laneW + laneW / 2;
      DrawCircle(x, (int)y, 14, RAYWHITE);
    }

    DrawText(audio.started ? "SPACE started" : "Press SPACE to start", 20, 20, 20, RAYWHITE);

    std::string s1 = "Score: " + std::to_string(score.score);
    std::string s2 = "Combo: " + std::to_string(score.combo) + " (max " + std::to_string(score.maxCombo) + ")";
    std::string s3 = std::string("Judge: ") + judge_name(score.lastJudge) +
                     "  dt=" + std::to_string((int)score.lastDtMs) + "ms";
    std::string s4 = "Heat: " + std::to_string((int)score.heat) +
                     "  Level: " + std::to_string(score.heatLevel);

    DrawText(s1.c_str(), 20, 60, 20, RAYWHITE);
    DrawText(s2.c_str(), 20, 90, 20, RAYWHITE);
    DrawText(s3.c_str(), 20, 120, 20, RAYWHITE);
    DrawText(s4.c_str(), 20, 150, 20, RAYWHITE);

    if (audio.started) {
      int bar = chart.timing.bar_from_beat(songBeat) + 1;
      int beatInBar = (int)std::floor(chart.timing.beat_in_bar(songBeat)) + 1;
      std::string st = "t=" + std::to_string(songTimeSec) +
                       "  beat=" + std::to_string(songBeat) +
                       "  bar=" + std::to_string(bar) + ":" + std::to_string(beatInBar);
      DrawText(st.c_str(), 20, 180, 20, GRAY);
    }

    EndDrawing();
  }

  audio.unload();
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
