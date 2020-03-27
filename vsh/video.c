#include "video.h"
#include "psf.h"
#include <sys/ioctl.h>
#include <ygg/video.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

// Framebuffer
static int fb;
static size_t fb_size;
static uint32_t *fb_mem;
static uint16_t fb_charw, fb_charh;
struct ioc_vmode fb_mode;

// PSF font
static struct psf_font *font;
static uintptr_t psf_fb_addr, psf_fb_pitch;

int psf_init(uintptr_t addr, uintptr_t pitch, uint16_t *charw, uint16_t *charh);

void video_putpixel(size_t x, size_t y, uint32_t v) {
    fb_mem[y * fb_mode.width + x] = v;
}

int video_start(void) {
    int off = 0;

    if ((fb = open("/dev/fb0", O_RDONLY, 0)) < 0) {
        perror("open(/dev/fb0)");
        return -1;
    }

    // Get framebuffer size
    if (ioctl(fb, IOC_GETVMODE, &fb_mode) != 0) {
        perror("ioctl(IOC_GETVMODE)");
        close(fb);
        return -1;
    }

    fb_size = (fb_mode.width * fb_mode.height * 4 + 0xFFF) & ~0xFFF;

    // Lock framebuffer
    if (ioctl(fb, IOC_FBCON, &off) != 0) {
        perror("ioctl(IOC_FBCON = off)");
        close(fb);
        return -1;
    }

    // Map framebuffer into memory
    if ((fb_mem = mmap(NULL, fb_size, 0, MAP_PRIVATE, fb, 0)) == MAP_FAILED) {
        perror("mmap()");
        // Unlock framebuffer for cleanup
        off = 1;
        if (ioctl(fb, IOC_FBCON, &off) != 0) {
            perror("ioctl(IOC_FBCON = off)");
            close(fb);
            return -1;
        }
        close(fb);
        return -1;
    }

    psf_init((uintptr_t) fb_mem, fb_mode.width * 4, &fb_charw, &fb_charh);
    memset(fb_mem, 0, fb_size);

    return 0;
}

void video_stop(void) {
    int on = 1;
    ioctl(fb, IOC_FBCON, &on);
    munmap(fb_mem, fb_size);
    close(fb);
}

extern char font_start;

int psf_init(uintptr_t addr, uintptr_t pitch, uint16_t *charw, uint16_t *charh) {
    font = (struct psf_font *) &font_start;
    if (font->magic != PSF_FONT_MAGIC) {
        return -1;
    }

    psf_fb_addr = addr;
    psf_fb_pitch = pitch;

    *charw = font->width;
    *charh = font->height;

    return 0;
}

void video_draw_string(size_t x, size_t y, const char *str, uint32_t fg) {
    for (; *str; x += fb_charw, ++str) {
        video_draw_glyph(y, x, *str, fg, 0);
    }
}

void video_draw_glyph(uint16_t row, uint16_t col, uint8_t c, uint32_t fg, uint32_t bg) {
    if (c >= font->numglyph) {
        c = 0;
    }

    int bytesperline = (font->width + 7) / 8;

    uint8_t *glyph = (uint8_t *) &font_start + font->headersize + c * font->bytesperglyph;
    // XXX: Assuming BPP is 32
    /* calculate the upper left corner on screen where we want to display.
       we only do this once, and adjust the offset later. This is faster. */
    uintptr_t offs = (row * psf_fb_pitch) + (col * 4);
    /* finally display pixels according to the bitmap */
    uint32_t x, y, line, mask;
    for (y = 0; y < font->height; ++y) {
        /* save the starting position of the line */
        line = offs;
        mask = 1 << (font->width - 1);
        /* display a row */
        for (x = 0; x < font->width; ++x) {
            *((uint32_t *)(psf_fb_addr + line)) = ((int) *glyph) & (mask) ? fg : bg;
            /* adjust to the next pixel */
            mask >>= 1;
            line += 4;
        }
        /* adjust to the next line */
        glyph += bytesperline;
        offs  += psf_fb_pitch;
    }
}
