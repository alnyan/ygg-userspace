#pragma once
#include <ygg/video.h>

extern struct ioc_vmode fb_mode;

void video_putpixel(size_t x, size_t y, uint32_t v);
void video_draw_glyph(uint16_t row, uint16_t col, uint8_t c, uint32_t fg, uint32_t bg);
void video_draw_string(size_t x, size_t y, const char *str, uint32_t fg);

int video_start(void);
void video_stop(void);
