# RP2040_ST7789_Fast

A high-speed **ST7789** display library for the **Raspberry Pi Pico (RP2040)**, built on the **arduino-pico** core (Earle Philhower).

The library does not depend on Adafruit GFX or TFT_eSPI. The driver is written directly on top of the pico-sdk:

- **16-bit SPI frames + DMA**: no byte swapping needed, and the CPU is free while the screen is being pushed.
- **RGB565 framebuffer in RAM**, with **double buffering** (when enough heap is available) so a new frame can be drawn while DMA pushes the previous one.
- **Custom graphics**: pixel, line, rectangle, circle, triangle, 5x7 text, image blit. Hot functions run from RAM, not from XIP flash.
- **`clk_peri` connected to `clk_sys`**, so SPI reaches 50 MHz (CPU at 200 MHz) or 62.5 MHz (CPU at 250 MHz).

> The library is written for an **ST7789 240x240** display connected over SPI. Other sizes (for example 240x320) can be set in the configuration but have not been tested.

---

## 1. Requirements

- An RP2040 board (Raspberry Pi Pico, Pico W, ...).
- The **arduino-pico** core by Earle Philhower (Boards Manager: *Raspberry Pi Pico/RP2040*).
- An ST7789 display with an SPI interface.

No other libraries are required.

## 2. Installation

**Option 1 - ZIP file:** Arduino IDE, *Sketch > Include Library > Add .ZIP Library...*, then select `RP2040_ST7789_Fast.zip`.

**Option 2 - copy the folder:** copy the whole `RP2040_ST7789_Fast` folder into `Documents/Arduino/libraries/`, then restart the Arduino IDE.

After installing, the examples are under *File > Examples > RP2040_ST7789_Fast*.

## 3. Wiring (default)

| Display | Pico |
|---------|------|
| SCK / SCL | GP18 |
| MOSI / SDA | GP19 |
| CS | GP17 |
| DC | GP16 |
| RST | GP20 |
| BL (backlight) | 3V3 (or a GPIO pin, see `cfg.bl`) |
| VCC / GND | 3V3 / GND |

The pins can be changed in `ST7789FastConfig`. `sck` and `mosi` must be a hardware SPI pin pair of the RP2040 (SPI0: GP16-19, or GP2-3, GP6-7, GP18-19, GP22-23; SPI1: GP10-11, GP14-15, GP26-27).

## 4. Arduino IDE settings (important for speed)

In the *Tools* menu:

- **CPU Speed**: `250 MHz (Overclock)` for SPI to reach 62.5 MHz. If you use `200 MHz`, set `cfg.spiHz = 50000000`.
- **Optimize**: `Fastest (-O3)`.

> 250 MHz is an overclock. If the display shows noise at 62.5 MHz, lower `spiHz` to 50 MHz.

## 5. Quick start

```cpp
#include <RP2040_ST7789_Fast.h>

ST7789Fast tft;

void setup() {
  if (!tft.begin()) {            // default config: 240x240, pins 18/19/17/16/20
    while (true) delay(1000);    // could not allocate RAM
  }

  tft.clear(ST7789_BLACK);
  tft.fillCircle(120, 120, 50, ST7789_RED);
  tft.text(20, 20, "Hello RP2040", ST7789_WHITE, 2);
  tft.display();                 // push the buffer to the screen (blocks until done)
}

void loop() {}
```

### Smooth animation with double buffering

```cpp
void loop() {
  tft.clear(ST7789_BLACK);
  // ... draw the new frame ...
  tft.flip();   // push the frame just drawn via DMA, then switch to the other buffer
}
```

`flip()` returns almost immediately if the DMA transfer of the previous frame has already finished; it only waits when the previous frame has not been fully pushed yet. If only **one** buffer could be allocated (`tft.buffers() == 1`), `flip()` automatically behaves like `display()` (it waits until the push is complete before returning).

## 6. Configuration (`ST7789FastConfig`)

```cpp
ST7789FastConfig cfg;
cfg.spiHz = 50000000;     // for example when the CPU runs at 200 MHz
cfg.madctl = 0xC0;        // for example to change orientation
cfg.yOffset = 80;
tft.begin(cfg);
```

| Field | Default | Meaning |
|-------|---------|---------|
| `sck`, `mosi` | 18, 19 | Hardware SPI pins |
| `cs`, `dc` | 17, 16 | Chip select, data/command |
| `rst` | 20 | Reset pin, `-1` if not connected |
| `bl` | -1 | Backlight pin (driven high in `begin`), `-1` if unused |
| `width`, `height` | 240, 240 | Framebuffer / display size |
| `spiHz` | 62500000 | Requested SPI speed (see `tft.spiHz()` for the actual value) |
| `madctl` | 0x00 | ST7789 orientation/color register |
| `xOffset`, `yOffset` | 0, 0 | Display window offset |
| `invert` | true | Enable INVON (most 240x240 modules need it) |
| `periFromSys` | true | Switch `clk_peri` to `clk_sys` (see the Notes section) |
| `doubleBuffer` | true | Try to allocate a second buffer if there is enough RAM |
| `heapReserve` | 24000 | Minimum heap that must remain after allocating the second buffer |

### Fixing a wrong image

| Symptom | Fix |
|---------|-----|
| Image is shifted/cut by about 80 lines, or upside down | `cfg.madctl = 0xC0; cfg.yOffset = 80;` |
| Red and blue are swapped | Add the BGR bit: `cfg.madctl \|= 0x08;` |
| Colors are inverted (black appears white) | `cfg.invert = false;` |
| Display shows noise at high speed | Lower `cfg.spiHz` (50 MHz, then 40 MHz, ...) |
| Display stays completely black | Check wiring, backlight, `cs`/`dc`/`rst` |

## 7. API

### Initialization and information

| Function | Description |
|----------|-------------|
| `bool begin(cfg = ST7789FastConfig())` | Initializes the display and allocates the framebuffer. Returns `false` if out of RAM. |
| `void end()` | Releases the buffers and the DMA channel. |
| `int width()`, `int height()` | Dimensions. |
| `int buffers()` | Number of buffers allocated (1 or 2). |
| `uint32_t spiHz()` | Actual SPI speed (Hz). |
| `uint16_t *buffer()` | Pointer to the buffer currently being drawn to (RGB565). |
| `static uint16_t color565(r, g, b)` | Converts 8-bit-per-channel color to RGB565. |
| `void backlight(bool on)` | Turns the backlight on/off (requires `cfg.bl`). |

Color constants: `ST7789_BLACK`, `ST7789_WHITE`, `ST7789_RED`, `ST7789_GREEN`, `ST7789_BLUE`, `ST7789_YELLOW`, `ST7789_CYAN`, `ST7789_MAGENTA`.

### Drawing (into the framebuffer, not yet shown on screen)

All functions clip to the screen bounds automatically, so you can draw at coordinates outside the screen.

| Function | Description |
|----------|-------------|
| `clear(color = 0)` / `fillScreen(color)` | Fills the whole buffer. |
| `pixel(x, y, c)` | A single pixel. |
| `hline(x, y, w, c)`, `vline(x, y, h, c)` | Horizontal / vertical line. |
| `line(x0, y0, x1, y1, c)` | Straight line (Bresenham). |
| `rect(x, y, w, h, c)`, `fillRect(...)` | Outlined / filled rectangle. |
| `circle(cx, cy, r, c)`, `fillCircle(...)` | Outlined / filled circle. |
| `triangle(x0, y0, x1, y1, x2, y2, c)`, `fillTriangle(...)` | Outlined / filled triangle. |
| `text(x, y, str, fg, size = 1)` | 5x7 text (ASCII 32-126), transparent background. `\n` starts a new line. |
| `textBg(x, y, str, fg, bg, size = 1)` | Text with a background color. |
| `textWidth(str, size = 1)` | Width of the first line of the string (pixels). |
| `blit(x, y, w, h, src)` | Copies a `w*h` RGB565 image into the buffer. |
| `blitKey(x, y, w, h, src, key)` | Like `blit`, but skips pixels of color `key` (transparent background for sprites). |

### Display

| Function | Description |
|----------|-------------|
| `display()` | Pushes the current drawing buffer to the screen and **waits until done**. |
| `flip()` | Pushes the buffer via **DMA**, then switches to the other buffer (if there are 2 buffers). |
| `displayRect(x, y, w, h)` | Pushes only a rectangular region (waits until done). Useful when only part of the screen changes. |
| `waitDMA()` | Waits for the current DMA transfer to finish. |
| `bool dmaBusy()` | Whether DMA is currently running. |

Image buffers used with `blit` must be RGB565 in the RP2040's native byte order (little-endian, the same as a C `uint16_t`); no byte swapping is needed.

## 8. Examples

| Example | Content |
|---------|---------|
| `Basic` | Initialization, drawing shapes and text, `display()`. |
| `BouncingBalls` | Animation with many balls using `flip()`, showing FPS. |
| `Cube3D` | Rotating 3D cube (fixed-point, hidden-face removal). |
| `Benchmark` | Measures 2D, 3D, screen push and pipeline speed; results are printed to Serial. |

## 9. Performance tips

- Use `flip()` with double buffering instead of `display()`; this lets drawing and screen pushing run in parallel.
- Redraw only the parts that change and use `displayRect()` if most of the screen is static.
- Avoid `float` in hot loops (the RP2040 has no FPU). For example, `Cube3D` uses fixed-point for the vertex transform.
- Use `fillRect`/`hline` instead of many `pixel` calls.
- Use `Optimize: Fastest (-O3)` and a 250 MHz CPU.

## 10. Notes

- **RAM**: each 240x240 framebuffer takes 115,200 bytes. The RP2040 has 264 KB, so a double buffer can only be allocated when enough heap remains. Check with `tft.buffers()`. Larger resolutions (for example 240x320) leave room for only one buffer.
- **`periFromSys`**: switching `clk_peri` to `clk_sys` affects **every** device that uses `clk_peri` (hardware UART and other SPI buses). USB Serial is not affected, but if you use `Serial1`/`Serial2` or another SPI bus, set `cfg.periFromSys = false` (SPI speed is then limited to 24 MHz) or initialize those devices after `tft.begin()`.
- Text supports only the 5x7 ASCII font (32-126).
- The library is for **RP2040 / arduino-pico** only.
- Every time a frame is sent, the library resets the display window (CASET/RASET/RAMWR), so `display()`, `flip()` and `displayRect()` can be mixed freely.

## 11. License

The library is released under the **GNU General Public License v3.0 or later** (`GPL-3.0-or-later`). See the [`LICENSE`](LICENSE) file.

This means you are free to use, modify and distribute the library, but when you distribute firmware (or a sketch) that links against this library, the full source code of that firmware must also be released under GPL-3.0-or-later. For personal use there are no additional restrictions.

## 12. Contributing and roadmap

- Currently supported: **ST7789 240x240** on RP2040 (arduino-pico).
- Planned: support for more displays (same architecture: 16-bit SPI + DMA + framebuffer).
- Bug reports or suggestions: open an *Issue* on GitHub, including the Serial log and the `ST7789FastConfig` you are using.
- Pull requests are always welcome; contributed code will be released under the same GPL-3.0-or-later license.
