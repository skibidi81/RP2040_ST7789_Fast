// RP2040_ST7789_Fast - high-speed ST7789 driver for RP2040 (arduino-pico)
// Copyright (C) 2026 skbidi81
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// BouncingBalls: hoat anh muot bang double buffer + DMA.
// Mau ve: ve frame moi vao buffer dang ve, roi flip() de DMA day frame vua ve len man.
#include <RP2040_ST7789_Fast.h>

ST7789Fast tft;

const int N = 24;
struct Ball { int x, y, vx, vy, r; uint16_t color; } balls[N];

uint32_t seed = 12345;
static inline uint32_t rnd(uint32_t n) {
  seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
  return ((seed >> 16) * n) >> 16;
}

void setup() {
  Serial.begin(115200);
  if (!tft.begin()) { while (true) delay(1000); }
  Serial.print("Buffers: "); Serial.println(tft.buffers());

  for (int i = 0; i < N; i++) {
    balls[i].r = 6 + rnd(12);
    balls[i].x = balls[i].r + rnd(tft.width()  - 2 * balls[i].r);
    balls[i].y = balls[i].r + rnd(tft.height() - 2 * balls[i].r);
    balls[i].vx = (int)rnd(7) - 3; if (balls[i].vx == 0) balls[i].vx = 2;
    balls[i].vy = (int)rnd(7) - 3; if (balls[i].vy == 0) balls[i].vy = 1;
    balls[i].color = tft.color565(64 + rnd(192), 64 + rnd(192), 64 + rnd(192));
  }
}

void loop() {
  static uint32_t frames = 0, lastMs = millis();
  static char fpsText[24] = "FPS: --";

  tft.clear(ST7789_BLACK);

  for (int i = 0; i < N; i++) {
    Ball &b = balls[i];
    b.x += b.vx; b.y += b.vy;
    if (b.x < b.r || b.x > tft.width()  - b.r) { b.vx = -b.vx; b.x += b.vx; }
    if (b.y < b.r || b.y > tft.height() - b.r) { b.vy = -b.vy; b.y += b.vy; }
    tft.fillCircle(b.x, b.y, b.r, b.color);
  }

  tft.textBg(4, 4, fpsText, ST7789_WHITE, ST7789_BLACK, 2);

  tft.flip();    // day len man bang DMA, tra ve ngay; ve frame ke tiep vao buffer con lai

  frames++;
  uint32_t now = millis();
  if (now - lastMs >= 1000) {
    snprintf(fpsText, sizeof(fpsText), "FPS: %lu", (unsigned long)(frames * 1000UL / (now - lastMs)));
    Serial.println(fpsText);
    frames = 0; lastMs = now;
  }
}
