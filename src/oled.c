/* SSD1315/SSD1306 128x64 I2C 驱动
 * ASCII 用 5x7 字体, 中文用 font_zh.h 的 16x16 点阵, 同一行内自动混排 */
#include "oled.h"
#include "board_config.h"
#include "font5x7.h"
#include "font_zh.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

static uint8_t fb[1024];

static void cmd(uint8_t c) {
    uint8_t b[2] = {0x00, c};
    i2c_write_blocking(OLED_I2C, OLED_ADDR, b, 2, false);
}

void oled_init(void) {
    i2c_init(OLED_I2C, 400 * 1000);
    gpio_set_function(PIN_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_OLED_SDA);
    gpio_pull_up(PIN_OLED_SCL);
    sleep_ms(50);

    static const uint8_t init_seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (unsigned i = 0; i < sizeof(init_seq); i++) cmd(init_seq[i]);
    oled_clear();
    oled_flush();
}

void oled_clear(void) { memset(fb, 0, sizeof(fb)); }

void oled_flush(void) {
    static uint8_t buf[1 + 1024];
    cmd(0x21); cmd(0); cmd(127);
    cmd(0x22); cmd(0); cmd(7);
    buf[0] = 0x40;
    memcpy(buf + 1, fb, 1024);
    i2c_write_blocking(OLED_I2C, OLED_ADDR, buf, sizeof(buf), false);
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x > 127 || y < 0 || y > 63) return;
    if (on) fb[(y >> 3) * 128 + x] |=  (uint8_t)(1 << (y & 7));
    else    fb[(y >> 3) * 128 + x] &= (uint8_t)~(1 << (y & 7));
}

/* ---- 5x7 ASCII ---- */

static void draw_glyph57(int x, int y, char c, bool inv) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 32 || c > 95) c = '?';
    const uint8_t *g = FONT5X7[c - 32];
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 7; j++) {
            bool on = g[i] & (1 << j);
            oled_pixel(x + i, y + j, inv ? !on : on);
        }
}

void oled_char(int x, int y, char c) { draw_glyph57(x, y, c, false); }

void oled_text(int x, int y, const char *s) {
    while (*s && x < 122) { draw_glyph57(x, y, *s++, false); x += 6; }
}

void oled_text_inv(int x, int y, const char *s) {
    while (*s && x < 122) { draw_glyph57(x, y, *s++, true); x += 6; }
}

void oled_text_center(int y, const char *s) {
    int w = (int)strlen(s) * 6 - 6;
    oled_text((128 - w) / 2, y, s);
}

void oled_rect(int x, int y, int w, int h, bool fill) {
    for (int i = 0; i < w; i++)
        for (int j = 0; j < h; j++)
            if (fill || i == 0 || j == 0 || i == w - 1 || j == h - 1)
                oled_pixel(x + i, y + j, true);
}

/* ---- 16x16 中文 + 混排 ---- */

static int draw_glyph16(int x, int y, const char *s, bool inv) {
    const char *const *bmp = zh_find(s);
    for (int r = 0; r < 16; r++)
        for (int c = 0; c < 16; c++) {
            bool on = bmp ? (bmp[r][c] == '#')
                          : (r == 0 || r == 15 || c == 0 || c == 15); /* 缺字画空框 */
            oled_pixel(x + c, y + r, inv ? !on : on);
        }
    return 16;
}

static int text_zh_ex(int x, int y, const char *s, bool inv) {
    while (*s && x < 128) {
        if ((uint8_t)*s < 0x80) {                       /* ASCII: 5x7, 行内居中 */
            draw_glyph57(x, y + 4, *s, inv);
            x += 7; s++;
        } else {                                        /* 中文: UTF-8 三字节 */
            x += draw_glyph16(x, y, s, inv);
            s += 3;
        }
    }
    return x;
}

int oled_text_zh(int x, int y, const char *s)     { return text_zh_ex(x, y, s, false); }
int oled_text_zh_inv(int x, int y, const char *s) { return text_zh_ex(x, y, s, true);  }

int oled_text_zh_width(const char *s) {
    int w = 0;
    while (*s) {
        if ((uint8_t)*s < 0x80) { w += 7;  s++; }
        else                    { w += 16; s += 3; }
    }
    return w;
}

void oled_text_zh_center(int y, const char *s) {
    oled_text_zh((128 - oled_text_zh_width(s)) / 2, y, s);
}

void oled_progress(int pct, const char *stage) {
    char t[12];
    oled_clear();
    oled_text_zh_center(2, stage);
    oled_rect(4, 26, 120, 14, false);
    if (pct > 0) oled_rect(6, 28, pct * 116 / 100, 10, true);
    snprintf(t, sizeof t, "%d%%", pct);
    oled_text_center(48, t);
    oled_flush();
}