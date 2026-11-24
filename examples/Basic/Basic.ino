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

// Basic: khoi tao man hinh, ve vai hinh va chu, roi day len man.
// Day noi: SCK->GP18  MOSI->GP19  CS->GP17  DC->GP16  RST->GP20
// Tools: CPU Speed 250 MHz (hoac 200 MHz va doi spiHz = 50000000)
#include <RP2040_ST7789_Fast.h>

ST7789Fast tft;

void setup() {
  Serial.begin(115200);

  ST7789FastConfig cfg;          // mac dinh: 240x240, chan 18/19/17/16/20, SPI 62.5 MHz
  // cfg.spiHz = 50000000;       // dung khi CPU 200 MHz
  // cfg.madctl = 0xC0; cfg.yOffset = 80;   // neu hinh bi lech/nguoc
  if (!tft.begin(cfg)) {
    Serial.println("Khoi tao that bai (het RAM?)");
    while (true) delay(1000);
  }
  Serial.print("Buffers: "); Serial.println(tft.buffers());
  Serial.print("SPI thuc: "); Serial.print(tft.spiHz() / 1000000.0f, 1); Serial.println(" MHz");

  tft.clear(ST7789_BLACK);

  tft.fillRect(10, 10, 100, 60, ST7789_RED);
  tft.rect(10, 10, 100, 60, ST7789_WHITE);
  tft.fillCircle(170, 40, 28, ST7789_GREEN);
  tft.circle(170, 40, 30, ST7789_WHITE);
  tft.fillTriangle(30, 200, 110, 90, 190, 200, ST7789_BLUE);
  tft.triangle(30, 200, 110, 90, 190, 200, ST7789_YELLOW);
  tft.line(0, 239, 239, 100, ST7789_CYAN);

  tft.text(10, 215, "RP2040 ST7789 FAST", ST7789_WHITE, 2);
  tft.text(10, 80, "Hello!", ST7789_YELLOW, 3);

  tft.display();   // day buffer len man hinh
}

void loop() {
}
