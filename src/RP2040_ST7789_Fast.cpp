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

#include "RP2040_ST7789_Fast.h"

#include <stdlib.h>
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// Ham nong nam trong RAM (khong chay tu flash XIP)
#define FST_RAM __attribute__((section(".time_critical.st7789fast")))

// ---------------------------------------------------------------
// Font 5x7, ASCII 32..126 (moi ky tu 5 cot, bit0 = hang tren cung)
// ---------------------------------------------------------------
static const uint8_t kFont5x7[95][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // ' '
  {0x00,0x00,0x5F,0x00,0x00}, // !
  {0x00,0x07,0x00,0x07,0x00}, // "
  {0x14,0x7F,0x14,0x7F,0x14}, // #
  {0x24,0x2A,0x7F,0x2A,0x12}, // $
  {0x23,0x13,0x08,0x64,0x62}, // %
  {0x36,0x49,0x56,0x20,0x50}, // &
  {0x00,0x05,0x03,0x00,0x00}, // '
  {0x00,0x1C,0x22,0x41,0x00}, // (
  {0x00,0x41,0x22,0x1C,0x00}, // )
  {0x14,0x08,0x3E,0x08,0x14}, // *
  {0x08,0x08,0x3E,0x08,0x08}, // +
  {0x00,0x50,0x30,0x00,0x00}, // ,
  {0x08,0x08,0x08,0x08,0x08}, // -
  {0x00,0x60,0x60,0x00,0x00}, // .
  {0x20,0x10,0x08,0x04,0x02}, // /
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9
  {0x00,0x36,0x36,0x00,0x00}, // :
  {0x00,0x56,0x36,0x00,0x00}, // ;
  {0x08,0x14,0x22,0x41,0x00}, // <
  {0x14,0x14,0x14,0x14,0x14}, // =
  {0x00,0x41,0x22,0x14,0x08}, // >
  {0x02,0x01,0x51,0x09,0x06}, // ?
  {0x32,0x49,0x79,0x41,0x3E}, // @
  {0x7E,0x11,0x11,0x11,0x7E}, // A
  {0x7F,0x49,0x49,0x49,0x36}, // B
  {0x3E,0x41,0x41,0x41,0x22}, // C
  {0x7F,0x41,0x41,0x22,0x1C}, // D
  {0x7F,0x49,0x49,0x49,0x41}, // E
  {0x7F,0x09,0x09,0x09,0x01}, // F
  {0x3E,0x41,0x49,0x49,0x7A}, // G
  {0x7F,0x08,0x08,0x08,0x7F}, // H
  {0x00,0x41,0x7F,0x41,0x00}, // I
  {0x20,0x40,0x41,0x3F,0x01}, // J
  {0x7F,0x08,0x14,0x22,0x41}, // K
  {0x7F,0x40,0x40,0x40,0x40}, // L
  {0x7F,0x02,0x0C,0x02,0x7F}, // M
  {0x7F,0x04,0x08,0x10,0x7F}, // N
  {0x3E,0x41,0x41,0x41,0x3E}, // O
  {0x7F,0x09,0x09,0x09,0x06}, // P
  {0x3E,0x41,0x51,0x21,0x5E}, // Q
  {0x7F,0x09,0x19,0x29,0x46}, // R
  {0x46,0x49,0x49,0x49,0x31}, // S
  {0x01,0x01,0x7F,0x01,0x01}, // T
  {0x3F,0x40,0x40,0x40,0x3F}, // U
  {0x1F,0x20,0x40,0x20,0x1F}, // V
  {0x3F,0x40,0x38,0x40,0x3F}, // W
  {0x63,0x14,0x08,0x14,0x63}, // X
  {0x07,0x08,0x70,0x08,0x07}, // Y
  {0x61,0x51,0x49,0x45,0x43}, // Z
  {0x00,0x7F,0x41,0x41,0x00}, // [
  {0x02,0x04,0x08,0x10,0x20}, // backslash
  {0x00,0x41,0x41,0x7F,0x00}, // ]
  {0x04,0x02,0x01,0x02,0x04}, // ^
  {0x40,0x40,0x40,0x40,0x40}, // _
  {0x00,0x01,0x02,0x04,0x00}, // `
  {0x20,0x54,0x54,0x54,0x78}, // a
  {0x7F,0x48,0x44,0x44,0x38}, // b
  {0x38,0x44,0x44,0x44,0x20}, // c
  {0x38,0x44,0x44,0x48,0x7F}, // d
  {0x38,0x54,0x54,0x54,0x18}, // e
  {0x08,0x7E,0x09,0x01,0x02}, // f
  {0x0C,0x52,0x52,0x52,0x3E}, // g
  {0x7F,0x08,0x04,0x04,0x78}, // h
  {0x00,0x44,0x7D,0x40,0x00}, // i
  {0x20,0x40,0x44,0x3D,0x00}, // j
  {0x7F,0x10,0x28,0x44,0x00}, // k
  {0x00,0x41,0x7F,0x40,0x00}, // l
  {0x7C,0x04,0x18,0x04,0x78}, // m
  {0x7C,0x08,0x04,0x04,0x78}, // n
  {0x38,0x44,0x44,0x44,0x38}, // o
  {0x7C,0x14,0x14,0x14,0x08}, // p
  {0x08,0x14,0x14,0x18,0x7C}, // q
  {0x7C,0x08,0x04,0x04,0x08}, // r
  {0x48,0x54,0x54,0x54,0x20}, // s
  {0x04,0x3F,0x44,0x40,0x20}, // t
  {0x3C,0x40,0x40,0x20,0x7C}, // u
  {0x1C,0x20,0x40,0x20,0x1C}, // v
  {0x3C,0x40,0x30,0x40,0x3C}, // w
  {0x44,0x28,0x10,0x28,0x44}, // x
  {0x0C,0x50,0x50,0x50,0x3C}, // y
  {0x44,0x64,0x54,0x4C,0x44}, // z
  {0x00,0x08,0x36,0x41,0x00}, // {
  {0x00,0x00,0x7F,0x00,0x00}, // |
  {0x00,0x41,0x36,0x08,0x00}, // }
  {0x10,0x08,0x08,0x10,0x08}  // ~
};

// ---------------------------------------------------------------
// Tien ich noi bo
// ---------------------------------------------------------------
static inline void fill16(uint16_t *p, uint16_t c, int n) {
  if (n <= 0) return;
  if ((uintptr_t)p & 2) { *p++ = c; n--; }
  uint32_t v = (uint32_t)c | ((uint32_t)c << 16);
  uint32_t *q = (uint32_t *)p;
  int m = n >> 1;
  while (m >= 8) {
    q[0] = v; q[1] = v; q[2] = v; q[3] = v;
    q[4] = v; q[5] = v; q[6] = v; q[7] = v;
    q += 8; m -= 8;
  }
  while (m--) *q++ = v;
  if (n & 1) *(uint16_t *)q = c;
}

// ---------------------------------------------------------------
// Khoi tao / giai phong
// ---------------------------------------------------------------
ST7789Fast::ST7789Fast() {}

ST7789Fast::~ST7789Fast() { end(); }

void ST7789Fast::cmd(uint8_t c) {
  gpio_put(_cfg.dc, 0);
  gpio_put(_cfg.cs, 0);
  spi_write_blocking(_spi, &c, 1);
  gpio_put(_cfg.cs, 1);
}

void ST7789Fast::data(const uint8_t *d, size_t n) {
  gpio_put(_cfg.dc, 1);
  gpio_put(_cfg.cs, 0);
  spi_write_blocking(_spi, d, n);
  gpio_put(_cfg.cs, 1);
}

void ST7789Fast::cmd1(uint8_t c, uint8_t v) { cmd(c); data(&v, 1); }

void ST7789Fast::window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  x0 += _cfg.xOffset; x1 += _cfg.xOffset;
  y0 += _cfg.yOffset; y1 += _cfg.yOffset;
  uint8_t a[4] = { (uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8), (uint8_t)x1 };
  uint8_t b[4] = { (uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8), (uint8_t)y1 };
  cmd(0x2A); data(a, 4);   // CASET
  cmd(0x2B); data(b, 4);   // RASET
  cmd(0x2C);               // RAMWR
}

bool ST7789Fast::begin(const ST7789FastConfig &cfg) {
  if (_ready) end();
  _cfg = cfg;
  _w = cfg.width;
  _h = cfg.height;
  if (_w == 0 || _h == 0) return false;

  // ---- Framebuffer ----
  size_t bytes = (size_t)_w * _h * 2;
  _buf[0] = (uint16_t *)malloc(bytes);
  if (!_buf[0]) return false;
  memset(_buf[0], 0, bytes);
  _nbuf = 1;
  if (cfg.doubleBuffer && rp2040.getFreeHeap() > bytes + cfg.heapReserve) {
    _buf[1] = (uint16_t *)malloc(bytes);
    if (_buf[1]) { memset(_buf[1], 0, bytes); _nbuf = 2; }
  }
  _cur = 0;
  _fb = _buf[0];

  // ---- GPIO ----
  gpio_init(_cfg.cs);  gpio_set_dir(_cfg.cs, GPIO_OUT);  gpio_put(_cfg.cs, 1);
  gpio_init(_cfg.dc);  gpio_set_dir(_cfg.dc, GPIO_OUT);  gpio_put(_cfg.dc, 1);
  if (_cfg.rst >= 0) { gpio_init(_cfg.rst); gpio_set_dir(_cfg.rst, GPIO_OUT); gpio_put(_cfg.rst, 1); }
  if (_cfg.bl  >= 0) { gpio_init(_cfg.bl);  gpio_set_dir(_cfg.bl,  GPIO_OUT); gpio_put(_cfg.bl, 1); }

  // ---- Clock: clk_peri mac dinh 48 MHz -> SPI toi da 24 MHz ----
  if (_cfg.periFromSys) {
    uint32_t sysHz = clock_get_hz(clk_sys);
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, sysHz, sysHz);
  }

  // ---- SPI (mode 3, 8 bit khi gui lenh) ----
  _spi = ((_cfg.sck >> 3) & 1) ? spi1 : spi0;
  spi_init(_spi, 10 * 1000 * 1000);
  gpio_set_function(_cfg.sck,  GPIO_FUNC_SPI);
  gpio_set_function(_cfg.mosi, GPIO_FUNC_SPI);
  spi_set_format(_spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

  // ---- Reset va khoi tao ST7789 ----
  if (_cfg.rst >= 0) {
    gpio_put(_cfg.rst, 1); sleep_ms(10);
    gpio_put(_cfg.rst, 0); sleep_ms(20);
    gpio_put(_cfg.rst, 1); sleep_ms(150);
  }
  cmd(0x01); sleep_ms(150);            // SWRESET
  cmd(0x11); sleep_ms(120);            // SLPOUT
  cmd1(0x3A, 0x55); sleep_ms(10);      // COLMOD: RGB565
  cmd1(0x36, _cfg.madctl);             // MADCTL
  if (_cfg.invert) cmd(0x21);          // INVON
  sleep_ms(10);
  cmd(0x13); sleep_ms(10);             // NORON
  cmd(0x29); sleep_ms(120);            // DISPON

  // Sau khi khoi tao moi tang toc do SPI
  _baud = spi_set_baudrate(_spi, _cfg.spiHz);

  // ---- DMA: 16 bit -> SPI TX FIFO ----
  _dma = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(_dma);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
  channel_config_set_dreq(&c, spi_get_dreq(_spi, true));
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  dma_channel_configure(_dma, &c, &spi_get_hw(_spi)->dr, nullptr, 0, false);

  _busy = false;
  _ready = true;
  return true;
}

void ST7789Fast::end() {
  if (!_ready) return;
  waitDMA();
  if (_dma >= 0) { dma_channel_unclaim(_dma); _dma = -1; }
  if (_buf[0]) { free(_buf[0]); _buf[0] = nullptr; }
  if (_buf[1]) { free(_buf[1]); _buf[1] = nullptr; }
  _fb = nullptr;
  _nbuf = 0;
  _ready = false;
}

void ST7789Fast::backlight(bool on) {
  if (_cfg.bl >= 0) gpio_put(_cfg.bl, on ? 1 : 0);
}

// ---------------------------------------------------------------
// Hien thi
// ---------------------------------------------------------------
void ST7789Fast::waitDMA() {
  if (!_busy) return;
  dma_channel_wait_for_finish_blocking(_dma);
  while (spi_get_hw(_spi)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
  gpio_put(_cfg.cs, 1);
  spi_set_format(_spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  _busy = false;
}

void ST7789Fast::startPush(const uint16_t *buf) {
  waitDMA();
  window(0, 0, _w - 1, _h - 1);
  gpio_put(_cfg.dc, 1);
  gpio_put(_cfg.cs, 0);
  spi_set_format(_spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  dma_channel_set_read_addr(_dma, buf, false);
  dma_channel_set_trans_count(_dma, (uint32_t)_w * _h, true);
  _busy = true;
}

void ST7789Fast::display() {
  if (!_ready) return;
  startPush(_fb);
  waitDMA();
}

void ST7789Fast::flip() {
  if (!_ready) return;
  if (_nbuf < 2) { display(); return; }
  startPush(_buf[_cur]);   // tu cho DMA truoc xong roi moi bat dau
  _cur ^= 1;
  _fb = _buf[_cur];
}

void ST7789Fast::displayRect(int x, int y, int w, int h) {
  if (!_ready) return;
  int x1 = x + w, y1 = y + h;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x1 > _w) x1 = _w;
  if (y1 > _h) y1 = _h;
  if (x >= x1 || y >= y1) return;
  w = x1 - x;
  waitDMA();
  window(x, y, x1 - 1, y1 - 1);
  gpio_put(_cfg.dc, 1);
  gpio_put(_cfg.cs, 0);
  spi_set_format(_spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  for (int row = y; row < y1; row++) {
    dma_channel_set_read_addr(_dma, _fb + row * _w + x, false);
    dma_channel_set_trans_count(_dma, w, true);
    dma_channel_wait_for_finish_blocking(_dma);
  }
  while (spi_get_hw(_spi)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
  gpio_put(_cfg.cs, 1);
  spi_set_format(_spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
}

// ---------------------------------------------------------------
// Do hoa
// ---------------------------------------------------------------
void FST_RAM ST7789Fast::clear(uint16_t color) { fill16(_fb, color, _w * _h); }

void FST_RAM ST7789Fast::hline(int x, int y, int w, uint16_t c) {
  if ((unsigned)y >= (unsigned)_h || w <= 0) return;
  int x1 = x + w;
  if (x < 0) x = 0;
  if (x1 > _w) x1 = _w;
  if (x >= x1) return;
  fill16(_fb + y * _w + x, c, x1 - x);
}

void FST_RAM ST7789Fast::vline(int x, int y, int h, uint16_t c) {
  if ((unsigned)x >= (unsigned)_w || h <= 0) return;
  int y1 = y + h;
  if (y < 0) y = 0;
  if (y1 > _h) y1 = _h;
  if (y >= y1) return;
  uint16_t *p = _fb + y * _w + x;
  for (; y < y1; y++, p += _w) *p = c;
}

void FST_RAM ST7789Fast::fillRect(int x, int y, int w, int h, uint16_t c) {
  if (w <= 0 || h <= 0) return;
  int x1 = x + w, y1 = y + h;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x1 > _w) x1 = _w;
  if (y1 > _h) y1 = _h;
  if (x >= x1 || y >= y1) return;
  int n = x1 - x;
  uint16_t *p = _fb + y * _w + x;
  for (; y < y1; y++, p += _w) fill16(p, c, n);
}

void FST_RAM ST7789Fast::rect(int x, int y, int w, int h, uint16_t c) {
  hline(x, y, w, c);
  hline(x, y + h - 1, w, c);
  vline(x, y, h, c);
  vline(x + w - 1, y, h, c);
}

void FST_RAM ST7789Fast::line(int x0, int y0, int x1, int y1, uint16_t c) {
  if (y0 == y1) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    hline(x0, y0, x1 - x0 + 1, c);
    return;
  }
  if (x0 == x1) {
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    vline(x0, y0, y1 - y0 + 1, c);
    return;
  }
  int dx = abs(x1 - x0), dy = -abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  bool inside = (unsigned)x0 < (unsigned)_w && (unsigned)x1 < (unsigned)_w &&
                (unsigned)y0 < (unsigned)_h && (unsigned)y1 < (unsigned)_h;
  if (inside) {
    uint16_t *p = _fb + y0 * _w + x0;
    int stepY = sy * _w;
    for (;;) {
      *p = c;
      if (x0 == x1 && y0 == y1) break;
      int e2 = err << 1;
      if (e2 >= dy) { err += dy; x0 += sx; p += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; p += stepY; }
    }
  } else {
    for (;;) {
      pixel(x0, y0, c);
      if (x0 == x1 && y0 == y1) break;
      int e2 = err << 1;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }
}

void FST_RAM ST7789Fast::circle(int cx, int cy, int r, uint16_t c) {
  if (r < 0) return;
  int x = 0, y = r, d = 1 - r;
  while (x <= y) {
    pixel(cx + x, cy + y, c); pixel(cx - x, cy + y, c);
    pixel(cx + x, cy - y, c); pixel(cx - x, cy - y, c);
    pixel(cx + y, cy + x, c); pixel(cx - y, cy + x, c);
    pixel(cx + y, cy - x, c); pixel(cx - y, cy - x, c);
    if (d < 0) d += 2 * x + 3;
    else { d += 2 * (x - y) + 5; y--; }
    x++;
  }
}

void FST_RAM ST7789Fast::fillCircle(int cx, int cy, int r, uint16_t c) {
  if (r < 0) return;
  int x = 0, y = r, d = 1 - r;
  while (x <= y) {
    hline(cx - y, cy + x, 2 * y + 1, c);
    hline(cx - y, cy - x, 2 * y + 1, c);
    hline(cx - x, cy + y, 2 * x + 1, c);
    hline(cx - x, cy - y, 2 * x + 1, c);
    if (d < 0) d += 2 * x + 3;
    else { d += 2 * (x - y) + 5; y--; }
    x++;
  }
}

void ST7789Fast::triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
  line(x0, y0, x1, y1, c);
  line(x1, y1, x2, y2, c);
  line(x2, y2, x0, y0, c);
}

void FST_RAM ST7789Fast::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
  int t;
  if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
  if (y1 > y2) { t = y2; y2 = y1; y1 = t; t = x2; x2 = x1; x1 = t; }
  if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }

  if (y0 == y2) {
    int a = x0, b = x0;
    if (x1 < a) a = x1; else if (x1 > b) b = x1;
    if (x2 < a) a = x2; else if (x2 > b) b = x2;
    hline(a, y0, b - a + 1, c);
    return;
  }

  int dx01 = x1 - x0, dy01 = y1 - y0;
  int dx02 = x2 - x0, dy02 = y2 - y0;
  int dx12 = x2 - x1, dy12 = y2 - y1;
  int sa = 0, sb = 0;
  int last = (y1 == y2) ? y1 : y1 - 1;
  int y;

  for (y = y0; y <= last; y++) {
    int a = x0 + sa / dy01;
    int b = x0 + sb / dy02;
    sa += dx01; sb += dx02;
    if (a > b) { t = a; a = b; b = t; }
    hline(a, y, b - a + 1, c);
  }
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for (; y <= y2; y++) {
    int a = x1 + sa / dy12;
    int b = x0 + sb / dy02;
    sa += dx12; sb += dx02;
    if (a > b) { t = a; a = b; b = t; }
    hline(a, y, b - a + 1, c);
  }
}

// ---------------------------------------------------------------
// Chu
// ---------------------------------------------------------------
void FST_RAM ST7789Fast::drawGlyph(int x, int y, char ch, uint16_t fg, uint16_t bg, bool useBg, uint8_t size) {
  unsigned idx = (unsigned char)ch;
  if (idx < 32 || idx > 126) idx = '?';
  const uint8_t *g = kFont5x7[idx - 32];
  if (size == 0) size = 1;

  if (useBg) fillRect(x, y, 6 * size, 8 * size, bg);

  if (size == 1) {
    if (x >= 0 && x + 5 <= _w && y >= 0 && y + 7 <= _h) {
      uint16_t *p = _fb + y * _w + x;
      for (int col = 0; col < 5; col++, p++) {
        uint8_t bits = g[col];
        uint16_t *q = p;
        for (int row = 0; row < 7; row++, q += _w)
          if (bits & (1 << row)) *q = fg;
      }
    } else {
      for (int col = 0; col < 5; col++)
        for (int row = 0; row < 7; row++)
          if (g[col] & (1 << row)) pixel(x + col, y + row, fg);
    }
  } else {
    for (int col = 0; col < 5; col++)
      for (int row = 0; row < 7; row++)
        if (g[col] & (1 << row)) fillRect(x + col * size, y + row * size, size, size, fg);
  }
}

void ST7789Fast::drawText(int x, int y, const char *s, uint16_t fg, uint16_t bg, bool useBg, uint8_t size) {
  if (size == 0) size = 1;
  int startX = x;
  for (; *s; s++) {
    if (*s == '\n') { x = startX; y += 8 * size; continue; }
    if (*s == '\r') continue;
    drawGlyph(x, y, *s, fg, bg, useBg, size);
    x += 6 * size;
  }
}

void ST7789Fast::text(int x, int y, const char *s, uint16_t fg, uint8_t size) {
  drawText(x, y, s, fg, 0, false, size);
}

void ST7789Fast::textBg(int x, int y, const char *s, uint16_t fg, uint16_t bg, uint8_t size) {
  drawText(x, y, s, fg, bg, true, size);
}

int ST7789Fast::textWidth(const char *s, uint8_t size) const {
  if (size == 0) size = 1;
  int n = 0;
  while (s[n] && s[n] != '\n') n++;
  return n * 6 * size;
}

// ---------------------------------------------------------------
// Blit (src la anh RGB565 rong w, cao h; co cat bien)
// ---------------------------------------------------------------
void FST_RAM ST7789Fast::blit(int x, int y, int w, int h, const uint16_t *src) {
  if (!src || w <= 0 || h <= 0) return;
  const int srcW = w;
  int sx = 0, sy = 0;
  if (x < 0) { sx = -x; w += x; x = 0; }
  if (y < 0) { sy = -y; h += y; y = 0; }
  if (x + w > _w) w = _w - x;
  if (y + h > _h) h = _h - y;
  if (w <= 0 || h <= 0) return;
  const uint16_t *s = src + sy * srcW + sx;
  uint16_t *d = _fb + y * _w + x;
  for (int r = 0; r < h; r++, s += srcW, d += _w) memcpy(d, s, (size_t)w * 2);
}

void FST_RAM ST7789Fast::blitKey(int x, int y, int w, int h, const uint16_t *src, uint16_t keyColor) {
  if (!src || w <= 0 || h <= 0) return;
  const int srcW = w;
  int sx = 0, sy = 0;
  if (x < 0) { sx = -x; w += x; x = 0; }
  if (y < 0) { sy = -y; h += y; y = 0; }
  if (x + w > _w) w = _w - x;
  if (y + h > _h) h = _h - y;
  if (w <= 0 || h <= 0) return;
  const uint16_t *s = src + sy * srcW + sx;
  uint16_t *d = _fb + y * _w + x;
  for (int r = 0; r < h; r++, s += srcW, d += _w) {
    for (int i = 0; i < w; i++) {
      uint16_t v = s[i];
      if (v != keyColor) d[i] = v;
    }
  }
}
