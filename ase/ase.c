#include <sys/termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define KEY_UP      (256)
#define KEY_DOWN    (257)
#define KEY_RIGHT   (258)
#define KEY_LEFT    (259)
#define KEY_HOME    (260)
#define KEY_END     (261)

#define MODE_NORMAL     0
#define MODE_COMMAND    1
#define MODE_INSERT     2

static int term_width = 0;
static int term_height = 0;

struct ase_line {
    char text_data[256];
    uint32_t length;
    uint32_t dirty;
};

struct ase_buffer {
    char filename[256];
    int mode;

    char cmd_buf[256];
    size_t cmd_len;

    uint32_t c_row;
    uint32_t c_col;
    size_t scroll;
    size_t line_count;
    size_t line_cap;
    struct ase_line *lines;
    // TODO: window size
};

int ase_buffer_init(struct ase_buffer *buf, size_t cap) {
    if (!(buf->lines = malloc(cap * sizeof(struct ase_line)))) {
        return -1;
    }
    for (size_t i = 0; i < cap; ++i) {
        buf->lines[i].length = 0;
    }
    buf->filename[0] = 0;
    buf->scroll = 0;
    buf->line_count = 0;
    buf->line_cap = cap;
    buf->c_row = 0;
    buf->c_col = 0;
    return 0;
}

static int ase_buffer_expand(struct ase_buffer *buf, size_t new_cap) {
    if (!(buf->lines = realloc(buf->lines, new_cap * sizeof(struct ase_line)))) {
        return -1;
    }
    buf->line_cap = new_cap;
    return 0;
}

int ase_buffer_load(struct ase_buffer *buf, const char *filename) {
    int res;
    int fd;
    ssize_t len;

    if ((res = ase_buffer_init(buf, 32)) != 0) {
        return res;
    }

    if ((fd = open(filename, O_RDONLY, 0)) < 0) {
        perror(filename);
        return -1;
    }

    strcpy(buf->filename, filename);

    while (1) {
        // Expand if needed
        if (buf->line_count + 5 >= buf->line_cap) {
            if ((res = ase_buffer_expand(buf, buf->line_cap + 32)) != 0) {
                close(fd);
                return -1;
            }
        }

        if ((len = gets_safe(fd, buf->lines[buf->line_count].text_data, 256)) < 0) {
            break;
        }

        buf->lines[buf->line_count++].length = len;
    }

    close(fd);
    return 0;
}

int ase_buffer_write(struct ase_buffer *buf, const char *filename) {
    if (filename == NULL) {
        if (!buf->filename[0]) {
            return -1;
        }
        filename = buf->filename;
    }

    int fd;
    char b;

    if ((fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
        perror(filename);
        return -1;
    }

    for (size_t i = 0; i < buf->line_count; ++i) {
        if (write(fd, buf->lines[i].text_data, buf->lines[i].length) != buf->lines[i].length) {
            close(fd);
            return -1;
        }
        b = '\n';
        write(fd, &b, 1);
    }

    close(fd);

    if (!buf->filename[0]) {
        strcpy(buf->filename, filename);
    }

    return 0;
}

static int getch_del(void) {
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 500000
    };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    int res;

    if ((res = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv)) < 0) {
        return res;
    }

    if (FD_ISSET(STDIN_FILENO, &fds)) {
        res = 0;
        if (read(STDIN_FILENO, &res, 1) != 1) {
            return -1;
        }
        return res;
    } else {
        return -1;
    }
}

static int escape_read(void) {
    // Maybe [
    int c = getch_del();

    if (c < 0) {
        return '\033';
    }

    if (c != '[') {
        return -1;
    }

    c = getch_del();

    switch (c) {
    case 'A':
        return KEY_UP;
    case 'B':
        return KEY_DOWN;
    case 'C':
        return KEY_RIGHT;
    case 'D':
        return KEY_LEFT;
    case '[':
        c = getch_del();
        switch (c) {
        case 'H':
            return KEY_HOME;
        case 'F':
            return KEY_END;
        default:
            return -1;
        }
        break;
    default:
        return -1;
    }
}

static int getch(void) {
    char c;

    if (read(STDIN_FILENO, &c, 1) < 0) {
        return -1;
    }

    if (c == '\033') {
        int r = escape_read();

        if (r > 0) {
            return r;
        }
    }

    return c;
}

static void ase_buffer_draw(struct ase_buffer *buf) {
    for (size_t i = buf->scroll; i < buf->line_count && (i - buf->scroll < term_height - 1); ++i) {
        printf("\033[%u;%uf", i - buf->scroll + 1, 1);
        printf("%3d ", i + 1);
        write(STDOUT_FILENO, buf->lines[i].text_data, buf->lines[i].length);
    }

    switch (buf->mode) {
    case MODE_INSERT:
        printf("\033[%u;2fINSERT", term_height);
        assert(buf->c_col < term_width);
        assert(buf->c_row >= buf->scroll);
        printf("\033[%u;%uf", buf->c_row + 1 - buf->scroll, buf->c_col + 5);
        break;
    case MODE_COMMAND:
        printf("\033[%u;1f:", term_height);
        write(STDOUT_FILENO, buf->cmd_buf, buf->cmd_len);
        break;
    case MODE_NORMAL:
        printf("\033[%u;2fNORMAL", term_height);
        break;
    }
}

static void ase_type_insert(struct ase_buffer *buf, char ch) {
    assert(buf->c_row <= buf->line_count);

    if (buf->c_row == buf->line_count) {
        assert(buf->line_count != buf->line_cap); // TODO
        buf->lines[buf->c_row].length = 0;
        ++buf->line_count;
    }

    struct ase_line *line = &buf->lines[buf->c_row];
    assert(buf->c_col <= line->length);
    for (ssize_t i = line->length; i > buf->c_col; --i) {
        line->text_data[i] = line->text_data[i - 1];
    }
    line->text_data[buf->c_col++] = ch;
    ++line->length;
}

static void ase_cursor_move(struct ase_buffer *buf, int dy, int dx) {
    if (dy) {
        int max_y = MAX((int) buf->line_count - 1, 0);
        int ny = MIN(max_y, MAX(0, (int) buf->c_row + dy));

        buf->c_row = ny;

        if (dy < 0 && buf->c_row < buf->scroll) {
            buf->scroll = buf->c_row;
        }
        if (dy > 0 && buf->c_row - buf->scroll >= term_height - 2) {
            buf->scroll = buf->c_row - term_height + 2;
        }
    }

    int max = 0;
    if (buf->c_row < buf->line_count) {
        max = (int) buf->lines[buf->c_row].length;
    }
    int nx = MIN(max, MAX(0, (int) buf->c_col + dx));

    buf->c_col = nx;
}

static void ase_erase_line(struct ase_buffer *buf, int n) {
    if (buf->c_row == n) {
        buf->c_col = 0;
    }

    for (ssize_t i = n; i < buf->line_count - 1; ++i) {
        buf->lines[i].length = buf->lines[i + 1].length;
        memcpy(buf->lines[i].text_data, buf->lines[i + 1].text_data, buf->lines[i + 1].length);
    }
    --buf->line_count;
}

static void ase_erase_char(struct ase_buffer *buf, int dir) {
    if (dir < 0) {
        assert(buf->c_row < buf->line_count);
        struct ase_line *line = &buf->lines[buf->c_row];
        if (buf->c_col > 0) {
            // Do the shift stuff and so on
            --buf->c_col;
            for (ssize_t i = buf->c_col; i < line->length - 1; ++i) {
                line->text_data[i] = line->text_data[i + 1];
            }
            --line->length;
        } else if (buf->c_row > 0) {
            // Merge lines
            struct ase_line *prev = &buf->lines[buf->c_row - 1];
            char *append_point = &prev->text_data[prev->length];
            size_t old_len = prev->length;
            assert(old_len + line->length < 256);
            prev->length += line->length;
            memcpy(append_point, line->text_data, line->length);

            --buf->c_row;
            if (buf->c_row < buf->scroll) {
                buf->scroll = buf->c_row;
            }
            buf->c_col = old_len;

            ase_erase_line(buf, buf->c_row + 1);
        }
    }
}

static void ase_line_insert(struct ase_buffer *buf, int br) {
    assert(buf->line_count < buf->line_cap);

    if (buf->line_count + 5 >= buf->line_cap) {
        assert(ase_buffer_expand(buf, buf->line_cap + 32) == 0);
    }

    assert(buf->c_row <= buf->line_count);
    if (br && buf->c_row < buf->line_count) {
        struct ase_line *src_line = &buf->lines[buf->c_row];
        assert(buf->c_col <= src_line->length);

        for (ssize_t i = buf->line_count; i > buf->c_row; --i) {
            buf->lines[i].length = buf->lines[i - 1].length;
            memcpy(buf->lines[i].text_data, buf->lines[i - 1].text_data, buf->lines[i - 1].length);
        }

        struct ase_line *dst_line = &buf->lines[buf->c_row + 1];
        dst_line->length = src_line->length - buf->c_col;
        memcpy(dst_line->text_data, src_line->text_data + buf->c_col, src_line->length - buf->c_col);
        src_line->length = buf->c_col;
        ++buf->line_count;
        ++buf->c_row;
        assert(buf->c_row >= buf->scroll);
        if (buf->c_row - buf->scroll >= term_height - 2) {
            buf->scroll++;
        }

        buf->c_col = 0;
    } else {
        //for (ssize_t i = buf->line_count; i > buf->c_row; --i) {
        //    buf->lines[i].length = buf->lines[i - 1].length;
        //    memcpy(buf->lines[i].text_data, buf->lines[i - 1].text_data, buf->lines[i - 1].length);
        //}
        //buf->lines[buf->c_row].length = 0;
        //buf->c_col = 0;
        //++buf->line_count;
    }
}

static void key_insert(struct ase_buffer *buf, int ch) {
    switch (ch) {
    case KEY_HOME:
        buf->c_col = 0;
        break;
    case KEY_END:
        if (buf->c_row < buf->line_count) {
            buf->c_col = buf->lines[buf->c_row].length;
        }
        break;
    case KEY_LEFT:
        ase_cursor_move(buf, 0, -1);
        break;
    case KEY_RIGHT:
        ase_cursor_move(buf, 0, +1);
        break;
    case KEY_UP:
        ase_cursor_move(buf, -1, 0);
        break;
    case KEY_DOWN:
        ase_cursor_move(buf, +1, 0);
        break;
    case '\n':
        ase_line_insert(buf, 1);
        break;
    case '\b':
        ase_erase_char(buf, -1);
        break;
    default:
        if (ch >= ' ') {
            ase_type_insert(buf, ch);
        }
        break;
    }
}

static void cmd_exec(struct ase_buffer *buf) {
    if (!strcmp(buf->cmd_buf, "q")) {
        printf("\033[2J\033[1;1f");
        exit(0);
    } else if (!strncmp(buf->cmd_buf, "e ", 2)) {
        // TODO: drop current buffer
        ase_buffer_load(buf, buf->cmd_buf + 2);
    } else if (!strncmp(buf->cmd_buf, "w ", 2)) {
        ase_buffer_write(buf, buf->cmd_buf + 2);
    } else if (!strcmp(buf->cmd_buf, "w")) {
        ase_buffer_write(buf, NULL);
    }
}

int main(int argc, char **argv) {
    // Main buffer
    struct ase_buffer buf;
    struct winsize ws;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
        perror("Failed to get window size");
        printf("This program should be run in terminal\n");
        return -1;
    }

    term_width = ws.ws_col;
    term_height = ws.ws_row;

    buf.mode = MODE_NORMAL;
    buf.cmd_len = 0;

    if (argc == 2) {
        if (access(argv[1], R_OK) == 0) {
            if (ase_buffer_load(&buf, argv[1]) != 0) {
                return -1;
            }
        } else {
            if (ase_buffer_init(&buf, term_height) != 0) {
                return -1;
            }
            strcpy(buf.filename, argv[1]);
        }
    } else if (argc == 1) {
        if (ase_buffer_init(&buf, term_height) != 0) {
            return -1;
        }
    } else {
        printf("usage: ase [filename]\n");
        return -1;
    }

    while (1) {
        puts2("\033[2J");
        ase_buffer_draw(&buf);

        int ch = getch();

        if (ch == '\033') {
            buf.mode = MODE_NORMAL;
            continue;
        }

        switch (buf.mode) {
        case MODE_NORMAL:
            switch (ch) {
            case ':':
                buf.mode = MODE_COMMAND;
                buf.cmd_len = 0;
                break;
            case 'i':
                buf.mode = MODE_INSERT;
                break;
            default:
                break;
            }
            break;
        case MODE_COMMAND:
            if (ch >= ' ' && ch < 255) {
                buf.cmd_buf[buf.cmd_len++] = ch;
            } else switch (ch) {
            case '\n':
                buf.cmd_buf[buf.cmd_len] = 0;
                cmd_exec(&buf);
                buf.mode = MODE_NORMAL;
                break;
            }
            break;
        case MODE_INSERT:
            key_insert(&buf, ch);
            break;
        }
    }

    return 0;
}
