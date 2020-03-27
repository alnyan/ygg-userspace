#include "video.h"
#include "input.h"
#include "logo.h"
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static int running = 1;

static void signal_handler(int signum) {
    running = 0;
}

static void draw_logo(int x, int y) {
    char pixel[4];
    char *data = font_logo_header_data;
    uint32_t v;
    for (uint32_t j = y; j < y + FONT_LOGO_HEIGHT; ++j) {
        for (uint32_t i = x; i < x + FONT_LOGO_WIDTH; ++i) {
            FONT_LOGO_HEADER_PIXEL(data, pixel);
            v = pixel[2] | ((uint32_t) pixel[1] << 8) | ((uint32_t) pixel[0] << 16);
            if (v) {
                video_putpixel(i, j, v);
            }
        }
    }
}

static void draw_cursor(int x, int y) {
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            if (x + i >= fb_mode.width ||
                y + j >= fb_mode.height) {
                continue;
            }

            if (7 - j > i) {
                video_putpixel(x + i, y + j, 0xFFFFFF);
            }
        }
    }
}

static uint64_t get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void draw_progress(size_t count, size_t total) {
    size_t w = count * fb_mode.width / total;

    for (size_t i = 0; i < fb_mode.width; ++i) {
        for (size_t j = fb_mode.height - 16; j < fb_mode.height; ++j) {
            video_putpixel(i, j, i <= w ? 0x00FF00 : 0xCCCCCC);
        }
    }
}

#define CLICK_COUNT_EXIT        20
int main(int argc, char **argv) {
    struct input_event ev;
    int prev_x = 0, prev_y = 0;
    int logo_vel_x = 1, logo_vel_y = 1;
    double logo_x = 0, logo_y = 0;
    int click_count = 0;
    int err = 0;
    int res;

    signal(SIGINT, signal_handler);

    if (input_init() != 0) {
        return -1;
    }

    if (video_start() != 0) {
        input_close();
        return -1;
    }

    uint64_t t0, t, dt;

    t0 = get_time();

    while (running) {
        t = get_time();
        dt = t - t0;

        res = input_wait_event(&ev);

        if (res < 0) {
            err = errno;
            break;
        }

        if (res == 1) {
            // An event ocurred
            if (ev.type == IN_KEY_DOWN && ev.key == 'q') {
                break;
            }

            if (ev.type == IN_KEY_DOWN || ev.type == IN_BUTTON_UP) {
                ++click_count;

                if (click_count == CLICK_COUNT_EXIT) {
                    break;
                }
            }
        }

        for (size_t x = (int) logo_x; x < (int) logo_x + FONT_LOGO_WIDTH; ++x) {
            for (size_t y = (int) logo_y; y < (int) logo_y + FONT_LOGO_HEIGHT; ++y) {
                video_putpixel(x, y, 0);
            }
        }

        t0 = t;
        logo_x += logo_vel_x * ((double) dt / 10);
        logo_y += logo_vel_y * ((double) dt / 10);

        if (logo_x < 0) {
            logo_vel_x = 1;
            logo_x = 0;
        } else if (logo_x + FONT_LOGO_WIDTH >= fb_mode.width) {
            logo_vel_x = -1;
            logo_x = fb_mode.width - FONT_LOGO_WIDTH - 1;
        }
        if (logo_y < 0) {
            logo_vel_y = 1;
            logo_y = 0;
        } else if (logo_y + FONT_LOGO_HEIGHT >= fb_mode.height) {
            logo_vel_y = -1;
            logo_y = fb_mode.height - FONT_LOGO_HEIGHT - 1;
        }

        draw_logo(logo_x, logo_y);
        draw_progress(click_count, CLICK_COUNT_EXIT);

        video_draw_string(100, 100, "Test?", 0xFF0000);

        for (size_t x = prev_x; x < MIN(prev_x + 8, fb_mode.width); ++x) {
            for (size_t y = prev_y; y < MIN(prev_y + 8, fb_mode.height); ++y) {
                video_putpixel(x, y, 0);
            }
        }
        draw_cursor(cursor_x, cursor_y);
        prev_x = cursor_x;
        prev_y = cursor_y;
    }

    video_stop();
    input_close();
    puts2("\033[2J\033[1;1f");

    if (err) {
        printf("An error ocurred: %s\n", strerror(err));
    } else {
        printf("Execution finished normally\n");
    }

    return 0;
}
