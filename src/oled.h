#pragma once
#include <stdint.h>
#include <stdbool.h>

void oled_init(void);
void oled_clear(void);
void oled_flush(void);
void oled_pixel(int x, int y, bool on);
void oled_char(int x, int y, char c);
void oled_text(int x, int y, const char *s);
void oled_text_inv(int x, int y, const char *s);    /* 反白 ASCII (先画好底色) */
void oled_text_center(int y, const char *s);
void oled_rect(int x, int y, int w, int h, bool fill);
void oled_progress(int pct, const char *stage);

/* 中文混排: 中文 16x16, ASCII 自动用 5x7 画在行内垂直居中 */
int  oled_text_zh(int x, int y, const char *s);
int  oled_text_zh_inv(int x, int y, const char *s);
void oled_text_zh_center(int y, const char *s);
int  oled_text_zh_width(const char *s);