# RP2040_ST7789_Fast

Thư viện điều khiển màn hình **ST7789** tốc độ cao cho **Raspberry Pi Pico (RP2040)**, dùng core **arduino-pico** (Earle Philhower).

Thư viện không dựa vào Adafruit GFX hay TFT_eSPI. Driver được viết trực tiếp bằng pico-sdk:

- **SPI khung 16 bit + DMA**: không cần đổi byte, CPU rảnh trong lúc đẩy màn.
- **Framebuffer RGB565 trong RAM**, kèm **double buffer** (nếu đủ heap) để vẽ frame mới trong khi DMA đẩy frame cũ.
- **Đồ họa tự viết**: pixel, đường, hình chữ nhật, hình tròn, tam giác, chữ 5x7, blit ảnh. Các hàm nóng chạy từ RAM, không chạy từ flash XIP.
- **`clk_peri` nối vào `clk_sys`**, SPI đạt 50 MHz (CPU 200 MHz) hoặc 62,5 MHz (CPU 250 MHz).

> Thư viện được viết cho màn **ST7789 240x240** nối qua SPI. Kích thước khác (ví dụ 240x320) có thể đặt trong cấu hình nhưng chưa được kiểm tra.

---

## 1. Yêu cầu

- Board RP2040 (Raspberry Pi Pico, Pico W, ...).
- Core **arduino-pico** của Earle Philhower (Boards Manager: *Raspberry Pi Pico/RP2040*).
- Màn hình ST7789 giao tiếp SPI.

Không cần cài thư viện nào khác.

## 2. Cài đặt

**Cách 1 - file ZIP:** Arduino IDE, *Sketch > Include Library > Add .ZIP Library...*, chọn `RP2040_ST7789_Fast.zip`.

**Cách 2 - chép thư mục:** chép cả thư mục `RP2040_ST7789_Fast` vào `Documents/Arduino/libraries/`, rồi mở lại Arduino IDE.

Sau khi cài, các ví dụ nằm ở *File > Examples > RP2040_ST7789_Fast*.

## 3. Đấu dây (mặc định)

| Màn hình | Pico |
|----------|------|
| SCK / SCL | GP18 |
| MOSI / SDA | GP19 |
| CS | GP17 |
| DC | GP16 |
| RST | GP20 |
| BL (đèn nền) | 3V3 (hoặc một chân GPIO, xem `cfg.bl`) |
| VCC / GND | 3V3 / GND |

Có thể đổi chân trong `ST7789FastConfig`. `sck` và `mosi` phải là một cặp chân SPI cứng của RP2040 (SPI0: GP16-19 hoặc GP2-3, GP6-7, GP18-19, GP22-23; SPI1: GP10-11, GP14-15, GP26-27).

## 4. Cài đặt trong Arduino IDE (quan trọng cho tốc độ)

Trong menu *Tools*:

- **CPU Speed**: `250 MHz (Overclock)` để SPI đạt 62,5 MHz. Nếu dùng `200 MHz`, đặt `cfg.spiHz = 50000000`.
- **Optimize**: `Fastest (-O3)`.

> Xung nhịp 250 MHz là ép xung. Nếu màn nhiễu ở 62,5 MHz, hạ `spiHz` xuống 50 MHz.

## 5. Bắt đầu nhanh

```cpp
#include <RP2040_ST7789_Fast.h>

ST7789Fast tft;

void setup() {
  if (!tft.begin()) {            // cấu hình mặc định: 240x240, chân 18/19/17/16/20
    while (true) delay(1000);    // không cấp phát được RAM
  }

  tft.clear(ST7789_BLACK);
  tft.fillCircle(120, 120, 50, ST7789_RED);
  tft.text(20, 20, "Hello RP2040", ST7789_WHITE, 2);
  tft.display();                 // đẩy buffer lên màn (chờ xong)
}

void loop() {}
```

### Hoạt ảnh mượt với double buffer

```cpp
void loop() {
  tft.clear(ST7789_BLACK);
  // ... vẽ frame mới ...
  tft.flip();   // đẩy frame vừa vẽ bằng DMA rồi chuyển sang buffer còn lại
}
```

`flip()` trả về gần như ngay lập tức nếu DMA của frame trước đã xong; nó chỉ chờ khi frame trước chưa đẩy hết. Nếu chỉ cấp phát được **một** buffer (`tft.buffers() == 1`), `flip()` tự chuyển thành `display()` (chờ đẩy xong mới trả về).

## 6. Cấu hình (`ST7789FastConfig`)

```cpp
ST7789FastConfig cfg;
cfg.spiHz = 50000000;     // ví dụ khi CPU 200 MHz
cfg.madctl = 0xC0;        // ví dụ đổi hướng
cfg.yOffset = 80;
tft.begin(cfg);
```

| Trường | Mặc định | Ý nghĩa |
|--------|----------|---------|
| `sck`, `mosi` | 18, 19 | Chân SPI cứng |
| `cs`, `dc` | 17, 16 | Chip select, data/command |
| `rst` | 20 | Chân reset, `-1` nếu không nối |
| `bl` | -1 | Chân đèn nền (kéo lên cao khi `begin`), `-1` nếu không dùng |
| `width`, `height` | 240, 240 | Kích thước framebuffer / màn hình |
| `spiHz` | 62500000 | Tốc độ SPI yêu cầu (giá trị thực xem `tft.spiHz()`) |
| `madctl` | 0x00 | Thanh ghi hướng/màu của ST7789 |
| `xOffset`, `yOffset` | 0, 0 | Độ lệch cửa sổ hiển thị |
| `invert` | true | Bật INVON (hầu hết module 240x240 cần) |
| `periFromSys` | true | Chuyển `clk_peri` sang `clk_sys` (xem mục Lưu ý) |
| `doubleBuffer` | true | Thử cấp phát buffer thứ hai nếu đủ RAM |
| `heapReserve` | 24000 | RAM heap tối thiểu còn lại sau khi cấp phát buffer thứ hai |

### Chỉnh khi hình ảnh bị sai

| Hiện tượng | Cách xử lý |
|-----------|-----------|
| Hình bị lệch/cắt khoảng 80 dòng, hoặc bị ngược | `cfg.madctl = 0xC0; cfg.yOffset = 80;` |
| Đỏ và xanh dương bị đảo | Thêm bit BGR: `cfg.madctl \|= 0x08;` |
| Màu bị đảo (đen thành trắng) | `cfg.invert = false;` |
| Màn nhiễu ở tốc độ cao | Hạ `cfg.spiHz` (50 MHz, rồi 40 MHz, ...) |
| Màn đen hoàn toàn | Kiểm tra dây, đèn nền, `cs`/`dc`/`rst` |

## 7. API

### Khởi tạo và thông tin

| Hàm | Mô tả |
|-----|-------|
| `bool begin(cfg = ST7789FastConfig())` | Khởi tạo màn hình và cấp phát framebuffer. Trả về `false` nếu hết RAM. |
| `void end()` | Giải phóng buffer và kênh DMA. |
| `int width()`, `int height()` | Kích thước. |
| `int buffers()` | Số buffer đã cấp phát (1 hoặc 2). |
| `uint32_t spiHz()` | Tốc độ SPI thực tế (Hz). |
| `uint16_t *buffer()` | Con trỏ tới buffer đang vẽ (RGB565). |
| `static uint16_t color565(r, g, b)` | Đổi màu 8 bit mỗi kênh sang RGB565. |
| `void backlight(bool on)` | Bật/tắt đèn nền (cần `cfg.bl`). |

Hằng số màu: `ST7789_BLACK`, `ST7789_WHITE`, `ST7789_RED`, `ST7789_GREEN`, `ST7789_BLUE`, `ST7789_YELLOW`, `ST7789_CYAN`, `ST7789_MAGENTA`.

### Vẽ (vào framebuffer, chưa hiện lên màn)

Mọi hàm đều tự cắt theo biên màn hình, nên có thể vẽ tọa độ ngoài màn.

| Hàm | Mô tả |
|-----|-------|
| `clear(color = 0)` / `fillScreen(color)` | Tô toàn bộ buffer. |
| `pixel(x, y, c)` | Một điểm ảnh. |
| `hline(x, y, w, c)`, `vline(x, y, h, c)` | Đường ngang / dọc. |
| `line(x0, y0, x1, y1, c)` | Đường thẳng (Bresenham). |
| `rect(x, y, w, h, c)`, `fillRect(...)` | Hình chữ nhật viền / đặc. |
| `circle(cx, cy, r, c)`, `fillCircle(...)` | Hình tròn viền / đặc. |
| `triangle(x0, y0, x1, y1, x2, y2, c)`, `fillTriangle(...)` | Tam giác viền / đặc. |
| `text(x, y, str, fg, size = 1)` | Chữ 5x7 (ASCII 32-126), nền trong suốt. `\n` xuống dòng. |
| `textBg(x, y, str, fg, bg, size = 1)` | Chữ có nền. |
| `textWidth(str, size = 1)` | Độ rộng dòng đầu của chuỗi (pixel). |
| `blit(x, y, w, h, src)` | Chép ảnh RGB565 `w*h` vào buffer. |
| `blitKey(x, y, w, h, src, key)` | Như `blit`, bỏ qua pixel có màu `key` (nền trong suốt cho sprite). |

### Hiển thị

| Hàm | Mô tả |
|-----|-------|
| `display()` | Đẩy buffer đang vẽ lên màn, **chờ xong**. |
| `flip()` | Đẩy buffer bằng **DMA** rồi chuyển sang buffer còn lại (nếu có 2 buffer). |
| `displayRect(x, y, w, h)` | Chỉ đẩy một vùng chữ nhật (chờ xong). Hữu ích khi chỉ một phần màn thay đổi. |
| `waitDMA()` | Chờ DMA hiện tại xong. |
| `bool dmaBusy()` | DMA có đang chạy không. |

Buffer ảnh dùng với `blit` phải ở dạng RGB565 theo thứ tự byte gốc của RP2040 (little-endian, giống `uint16_t` trong C), không cần đổi byte.

## 8. Ví dụ

| Ví dụ | Nội dung |
|-------|----------|
| `Basic` | Khởi tạo, vẽ hình và chữ, `display()`. |
| `BouncingBalls` | Hoạt ảnh nhiều quả bóng với `flip()`, hiển thị FPS. |
| `Cube3D` | Khối lập phương 3D xoay (fixed-point, loại mặt khuất). |
| `Benchmark` | Đo tốc độ 2D, 3D, đẩy màn và pipeline; kết quả in ra Serial. |

## 9. Mẹo hiệu năng

- Dùng `flip()` kèm double buffer thay vì `display()`; nhờ đó việc vẽ và việc đẩy màn chạy song song.
- Chỉ vẽ lại phần thay đổi và dùng `displayRect()` nếu phần lớn màn hình tĩnh.
- Tránh `float` trong vòng lặp nóng (RP2040 không có FPU). Ví dụ `Cube3D` dùng fixed-point cho phần biến đổi đỉnh.
- Dùng `fillRect`/`hline` thay vì nhiều lệnh `pixel`.
- Dùng `Optimize: Fastest (-O3)` và CPU 250 MHz.

## 10. Lưu ý

- **RAM**: mỗi framebuffer 240x240 chiếm 115.200 byte. RP2040 có 264 KB, nên double buffer chỉ cấp phát được khi còn dư heap. Kiểm tra bằng `tft.buffers()`. Độ phân giải lớn hơn (ví dụ 240x320) sẽ chỉ còn đủ chỗ cho một buffer.
- **`periFromSys`**: chuyển `clk_peri` sang `clk_sys` ảnh hưởng đến **mọi** thiết bị dùng `clk_peri` (UART phần cứng và SPI khác). USB Serial không bị ảnh hưởng, nhưng nếu dùng `Serial1`/`Serial2` hoặc SPI khác, hãy đặt `cfg.periFromSys = false` (tốc độ SPI khi đó tối đa 24 MHz) hoặc khởi tạo các thiết bị đó sau `tft.begin()`.
- Chữ chỉ hỗ trợ font 5x7 ASCII (32-126).
- Thư viện chỉ dành cho **RP2040 / arduino-pico**.
- Mỗi lần gửi frame, thư viện đặt lại cửa sổ hiển thị (CASET/RASET/RAMWR), nên có thể chạy lẫn `display()`, `flip()` và `displayRect()`.

## 11. Giấy phép

Thư viện phát hành theo **GNU General Public License v3.0 trở lên** (`GPL-3.0-or-later`). Xem file [`LICENSE`](LICENSE).

Điều này có nghĩa là bạn được tự do dùng, sửa và phân phối thư viện, nhưng khi phân phối firmware (hoặc sketch) có liên kết với thư viện này, toàn bộ mã nguồn của firmware đó cũng phải được phát hành theo GPL-3.0-or-later. Dùng cho mục đích cá nhân thì không có ràng buộc gì thêm.

## 12. Đóng góp và kế hoạch

- Hiện hỗ trợ: **ST7789 240x240** trên RP2040 (arduino-pico).
- Kế hoạch: hỗ trợ thêm các màn hình khác (cùng kiến trúc SPI 16 bit + DMA + framebuffer).
- Báo lỗi hoặc đề xuất: mở *Issue* trên GitHub, kèm log Serial và cấu hình `ST7789FastConfig` đang dùng.
- Pull request luôn được hoan nghênh; mã đóng góp sẽ được phát hành theo cùng giấy phép GPL-3.0-or-later.
