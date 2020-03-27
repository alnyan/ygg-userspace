#pragma once

#define BUTTON_LEFT         1

struct input_event {
    enum {
        IN_KEY_DOWN,
        IN_BUTTON_DOWN,
        IN_BUTTON_UP,
        IN_MOUSE_MOVE,
    } type;
    union {
        char key;
        struct {
            int16_t dx, dy;
        } mouse_move;
        int button;
    };
};

extern int cursor_x, cursor_y;

int input_wait_event(struct input_event *ev);
int input_init(void);
void input_close(void);
