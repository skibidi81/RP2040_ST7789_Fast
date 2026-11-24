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

// =====================================================
// RP2040_ST7789_Fast
// Driver ST7789 toc do cao cho RP2040 (core arduino-pico).
// SPI khung 16 bit + DMA, framebuffer RGB565 trong RAM, double buffer.
// =====================================================
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include "hardware/spi.h"

// ---- Mau RGB565 ----
static const uint16_t ST7789_BLACK   = 0x0000;
static const uint16_t ST7789_WHITE   = 0xFFFF;
static const uint16_t ST7789_RED     = 0xF800;
static const uint16_t ST7789_GREEN   = 0x07E0;
static const uint16_t ST7789_BLUE    = 0x001F;
static const uint16_t ST7789_YELLOW  = 0xFFE0;
static const uint16_t ST7789_CYAN    = 0x07FF;
static const uint16_t ST7789_MAGENTA = 0xF81F;

// ---- Cau hinh ----
struct ST7789FastConfig {
  // Chan (SCK/MOSI phai la cap chan cung SPI0 hoac SPI1)
  uint8_t sck  = 18;
  uint8_t mosi = 19;
  uint8_t cs   = 17;
  uint8_t dc   = 16;
  int8_t  rst  = 20;     // -1 neu khong noi chan RST
  int8_t  bl   = -1;     // chan den nen (keo len cao khi begin), -1 neu khong dung

  // Kich thuoc framebuffer / man hinh
  uint16_t width  = 240;
  uint16_t height = 240;

  // SPI: 62500000 khi CPU 250 MHz, 50000000 khi CPU 200 MHz
  uint32_t spiHz = 62500000;

  // Cau hinh panel
  uint8_t  madctl  = 0x00;   // huong/mau: 0x00, hoac 0xC0 (+ yOffset 80), bit 0x08 = BGR
  uint16_t xOffset = 0;
  uint16_t yOffset = 0;
  bool     invert  = true;   // INVON (phan lon module 240x240 can)

  // He thong
  bool     periFromSys   = true;    // chuyen clk_peri sang clk_sys (SPI nhanh hon)
  bool     doubleBuffer  = true;    // thu cap phat buffer thu 2 neu du RAM
  uint32_t heapReserve   = 24000;   // RAM heap con lai toi thieu sau khi cap phat buffer 2
};

class ST7789Fast {
 public:
  ST7789Fast();
  ~ST7789Fast();

  // Khoi tao. Tra ve false neu khong cap phat duoc framebuffer.
  bool begin(const ST7789FastConfig &cfg = ST7789FastConfig());
  void end();

  // ---- Thong tin ----
  int       width()   const { return _w; }
  int       height()  const { return _h; }
  int       buffers() const { return _nbuf; }       // 1 hoac 2
  uint32_t  spiHz()   const { return _baud; }       // toc do SPI thuc te
  uint16_t *buffer()  const { return _fb; }         // buffer dang ve (RGB565 native)

  // ---- Mau ----
  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }

  // ---- Ve (vao framebuffer RAM, chua hien len man) ----
  void clear(uint16_t color = 0);
  void fillScreen(uint16_t color) { clear(color); }

  inline void pixel(int x, int y, uint16_t c) {
    if ((unsigned)x < (unsigned)_w && (unsigned)y < (unsigned)_h) _fb[y * _w + x] = c;
  }
  void hline(int x, int y, int w, uint16_t c);
  void vline(int x, int y, int h, uint16_t c);
  void line(int x0, int y0, int x1, int y1, uint16_t c);
  void rect(int x, int y, int w, int h, uint16_t c);
  void fillRect(int x, int y, int w, int h, uint16_t c);
  void circle(int cx, int cy, int r, uint16_t c);
  void fillCircle(int cx, int cy, int r, uint16_t c);
  void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c);
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c);

  // Chu font 5x7 (ASCII 32..126), size = he so phong to. '\n' xuong dong.
  void text(int x, int y, const char *s, uint16_t fg, uint8_t size = 1);
  void textBg(int x, int y, const char *s, uint16_t fg, uint16_t bg, uint8_t size = 1);
  int  textWidth(const char *s, uint8_t size = 1) const;   // do rong dong dau tien (pixel)

  // Chep anh RGB565 (w*h pixel, native byte order) vao framebuffer, co cat bien.
  void blit(int x, int y, int w, int h, const uint16_t *src);
  void blitKey(int x, int y, int w, int h, const uint16_t *src, uint16_t keyColor);

  // ---- Hien thi ----
  void display();                                   // day buffer dang ve len man, cho xong (blocking)
  void displayRect(int x, int y, int w, int h);     // day mot vung chu nhat (blocking)
  void flip();                                      // day bang DMA roi doi buffer (neu co 2 buffer)
  void waitDMA();                                   // cho DMA hien tai xong
  bool dmaBusy() const { return _busy; }
  void backlight(bool on);

 private:
  ST7789FastConfig _cfg;
  spi_inst_t *_spi = nullptr;
  int  _dma = -1;
  uint16_t *_buf[2] = {nullptr, nullptr};
  uint16_t *_fb = nullptr;
  uint8_t  _cur = 0;
  uint8_t  _nbuf = 0;
  uint16_t _w = 0, _h = 0;
  uint32_t _baud = 0;
  volatile bool _busy = false;
  bool _ready = false;

  void cmd(uint8_t c);
  void data(const uint8_t *d, size_t n);
  void cmd1(uint8_t c, uint8_t v);
  void window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void startPush(const uint16_t *buf);
  void drawGlyph(int x, int y, char ch, uint16_t fg, uint16_t bg, bool useBg, uint8_t size);
  void drawText(int x, int y, const char *s, uint16_t fg, uint16_t bg, bool useBg, uint8_t size);
};
