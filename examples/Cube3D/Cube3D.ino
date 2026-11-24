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

// Cube3D: khoi lap phuong 3D xoay, fixed-point (khong dung float cho tung dinh),
// loai mat khuat, double buffer + DMA.
#include <RP2040_ST7789_Fast.h>
#include <math.h>

ST7789Fast tft;

const int8_t cubeV[8][3] = {
  {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
  {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
};
const uint8_t edges[12][2] = {
  {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
};
const uint8_t quads[6][4] = {
  {0,1,2,3}, {4,5,6,7}, {0,1,5,4}, {3,2,6,7}, {0,3,7,4}, {1,2,6,5}
};
const uint16_t quadColors[6] = {
  ST7789_RED, ST7789_GREEN, ST7789_BLUE, ST7789_CYAN, ST7789_MAGENTA, ST7789_YELLOW
};

struct P2 { int16_t x, y; } pr[8];

void transformCube(float ax, float ay) {
  int32_t sx = (int32_t)(sinf(ax) * 4096.0f), cx = (int32_t)(cosf(ax) * 4096.0f);
  int32_t sy = (int32_t)(sinf(ay) * 4096.0f), cy = (int32_t)(cosf(ay) * 4096.0f);
  for (int i = 0; i < 8; i++) {
    int32_t x = cubeV[i][0], y = cubeV[i][1], z = cubeV[i][2];
    int32_t rx  = x * cy - z * sy;
    int32_t rz  = x * sy + z * cy;
    int32_t ry  = y * cx - ((rz * sx) >> 12);
    int32_t rz2 = y * sx + ((rz * cx) >> 12);
    int32_t camera = rz2 + (4 << 12);
    pr[i].x = tft.width()  / 2 + (rx * 150) / camera;
    pr[i].y = tft.height() / 2 - (ry * 150) / camera;
  }
}

bool faceVisible(const uint8_t *q) {
  int32_t ax = pr[q[1]].x - pr[q[0]].x, ay = pr[q[1]].y - pr[q[0]].y;
  int32_t bx = pr[q[2]].x - pr[q[0]].x, by = pr[q[2]].y - pr[q[0]].y;
  return (ax * by - ay * bx) < 0;   // neu cube bi "rong" mat, doi thanh '>'
}

void setup() {
  Serial.begin(115200);
  if (!tft.begin()) { while (true) delay(1000); }
}

void loop() {
  static int frame = 0;
  static uint32_t frames = 0, lastMs = millis();
  static char fpsText[24] = "FPS: --";

  tft.clear(ST7789_BLACK);
  float a = frame * 0.035f;
  transformCube(a, a * 0.8f);

  for (int f = 0; f < 6; f++) {
    const uint8_t *q = quads[f];
    if (!faceVisible(q)) continue;
    tft.fillTriangle(pr[q[0]].x, pr[q[0]].y, pr[q[1]].x, pr[q[1]].y, pr[q[2]].x, pr[q[2]].y, quadColors[f]);
    tft.fillTriangle(pr[q[0]].x, pr[q[0]].y, pr[q[2]].x, pr[q[2]].y, pr[q[3]].x, pr[q[3]].y, quadColors[f]);
  }
  for (int i = 0; i < 12; i++)
    tft.line(pr[edges[i][0]].x, pr[edges[i][0]].y, pr[edges[i][1]].x, pr[edges[i][1]].y, ST7789_WHITE);

  tft.textBg(4, 4, fpsText, ST7789_WHITE, ST7789_BLACK, 2);
  tft.flip();
  frame++;

  frames++;
  uint32_t now = millis();
  if (now - lastMs >= 1000) {
    snprintf(fpsText, sizeof(fpsText), "FPS: %lu", (unsigned long)(frames * 1000UL / (now - lastMs)));
    Serial.println(fpsText);
    frames = 0; lastMs = now;
  }
}
