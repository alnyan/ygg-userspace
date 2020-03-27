#pragma once
#include <stdint.h>

#define PSF_FONT_MAGIC 0x864ab572

struct psf_font {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
};
