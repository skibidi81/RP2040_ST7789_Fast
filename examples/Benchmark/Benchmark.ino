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

// Benchmark: do toc do do hoa 2D, 3D, day man va pipeline bang thu vien.
// Ket qua in ra Serial Monitor (115200). Tools: CPU 250 MHz (hoac 200), Optimize -O3.
#include <RP2040_ST7789_Fast.h>
#include <math.h>

ST7789Fast tft;
#define W 240
#define H 240

uint32_t seed = 1234567;
static inline uint32_t randomFast() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }
static inline uint32_t rnd(uint32_t n) { return ((randomFast() >> 16) * n) >> 16; }
static inline uint16_t randColor() { return (uint16_t)(randomFast() >> 8); }

void report(const char *name, uint32_t elapsed, uint32_t ops) {
  if (elapsed == 0) elapsed = 1;
  Serial.print(name); Serial.print(" | "); Serial.print(elapsed);
  Serial.print(" us | "); Serial.print((float)ops * 1000000.0f / (float)elapsed, 1);
  Serial.println(" ops/s");
}

// ---- 3D (fixed-point Q12) ----
const int8_t cubeV[8][3] = {
  {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1}, {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
};
const uint8_t edges[12][2] = {
  {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
};
const uint8_t quads[6][4] = {
  {0,1,2,3},{4,5,6,7},{0,1,5,4},{3,2,6,7},{0,3,7,4},{1,2,6,5}
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
    int32_t rx = x * cy - z * sy, rz = x * sy + z * cy;
    int32_t ry = y * cx - ((rz * sx) >> 12), rz2 = y * sx + ((rz * cx) >> 12);
    int32_t camera = rz2 + (4 << 12);
    pr[i].x = 120 + (rx * 150) / camera;
    pr[i].y = 120 - (ry * 150) / camera;
  }
}
bool faceVisible(const uint8_t *q) {
  int32_t ax = pr[q[1]].x - pr[q[0]].x, ay = pr[q[1]].y - pr[q[0]].y;
  int32_t bx = pr[q[2]].x - pr[q[0]].x, by = pr[q[2]].y - pr[q[0]].y;
  return (ax * by - ay * bx) < 0;
}
void drawCube(int frame) {
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
}

void bench2D() {
  Serial.println("\n===== 2D =====");
  uint32_t t;

  tft.clear(0); t = micros();
  for (int i = 0; i < 12000; i++) tft.pixel(rnd(W), rnd(H), randColor());
  report("Pixel", micros() - t, 12000); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 3000; i++) tft.line(rnd(W), rnd(H), rnd(W), rnd(H), randColor());
  report("Line", micros() - t, 3000); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 1500; i++) tft.rect(rnd(200), rnd(200), 10 + rnd(30), 10 + rnd(30), randColor());
  report("Rectangle", micros() - t, 1500); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 1000; i++) tft.fillRect(rnd(200), rnd(200), 10 + rnd(30), 10 + rnd(30), randColor());
  report("Filled rectangle", micros() - t, 1000); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 1000; i++) tft.circle(rnd(W), rnd(H), 2 + rnd(40), randColor());
  report("Circle", micros() - t, 1000); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 300; i++) tft.fillCircle(rnd(W), rnd(H), 2 + rnd(25), randColor());
  report("Filled circle", micros() - t, 300); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 1500; i++)
    tft.fillTriangle(rnd(W), rnd(H), rnd(W), rnd(H), rnd(W), rnd(H), randColor());
  report("Filled triangle", micros() - t, 1500); tft.display();

  tft.clear(0); t = micros();
  for (int i = 0; i < 1000; i++) tft.text(rnd(170), rnd(230), "RP2040 PICO", randColor());
  report("Text", micros() - t, 1000); tft.display();

  t = micros();
  for (int i = 0; i < 200; i++) tft.clear(randColor());
  report("clear", micros() - t, 200);
}

void bench3D() {
  Serial.println("\n===== 3D =====");
  uint32_t t = micros();
  for (int f = 0; f < 180; f++) drawCube(f);
  report("3D solid cube + cull (frames)", micros() - t, 180);
  tft.display();
}

void benchPush() {
  Serial.println("\n===== PUSH =====");
  uint16_t *p = tft.buffer();
  for (uint32_t i = 0; i < (uint32_t)W * H; i++)
    p[i] = ((i & 31) << 11) | (((i / W) & 63) << 5) | ((i / 7) & 31);
  uint32_t t = micros();
  for (int f = 0; f < 30; f++) tft.display();
  uint32_t avg = (micros() - t) / 30;
  Serial.print("SPI thuc: "); Serial.print(tft.spiHz() / 1000000.0f, 1); Serial.println(" MHz");
  Serial.print("Push 1 frame: "); Serial.print(avg); Serial.print(" us | Max FPS: ");
  Serial.println(1000000.0f / (float)avg, 1);
}

void benchPipeline() {
  Serial.println("\n===== PIPELINE (ve cube + day man) =====");
  const int FRAMES = 120;
  uint32_t t = micros();
  for (int f = 0; f < FRAMES; f++) { drawCube(f); tft.display(); }
  Serial.print("A) display() tuan tu: ");
  Serial.print(FRAMES * 1000000.0f / (float)(micros() - t), 1); Serial.println(" FPS");

  if (tft.buffers() == 2) {
    t = micros();
    for (int f = 0; f < FRAMES; f++) { drawCube(f); tft.flip(); }
    tft.waitDMA();
    Serial.print("B) flip() double buffer + DMA: ");
    Serial.print(FRAMES * 1000000.0f / (float)(micros() - t), 1); Serial.println(" FPS");
  } else {
    Serial.println("B) bo qua: chi co 1 buffer");
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);

  if (!tft.begin()) { Serial.println("Khoi tao that bai"); while (true) delay(1000); }
  Serial.print("CPU: "); Serial.print(rp2040.f_cpu() / 1000000); Serial.println(" MHz");
  Serial.print("Buffers: "); Serial.println(tft.buffers());

  bench2D();
  bench3D();
  benchPush();
  benchPipeline();
  Serial.println("\nBENCHMARK COMPLETE");
}

void loop() {
}
