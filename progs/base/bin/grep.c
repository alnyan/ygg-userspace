#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define PAT_ANY     (-1)
#define PAT_ALPHA   (-2)
#define PAT_DIGIT   (-3)

static int char_cmp(char c, int p) {
    switch (p) {
    case PAT_ANY:
        return 1;
    case PAT_ALPHA:
        return isalpha(c);
    case PAT_DIGIT:
        return isdigit(c);
    default:
        return c == (char) p;
    }
}

static int grep_match(const char *line, const char *pattern) {
    // TODO: proper implementation
    const char *p, *t;
    char pc;
#define MATCH_NONE          0
#define MATCH_ONE           1
#define MATCH_ONE_MORE      2
#define MATCH_ZERO_MORE     3
#define MATCH_ONE_OPT       4
    int match_type = MATCH_NONE;
    int match_char = 0;

    // Line:  abcdef abcdef
    // p:     abcdef abcdef
    //        bcdef abcdef
    //        ...
    for (p = line; *p; ++p) {
        const char *c = p;
        t = pattern;
        int fail = 0;

        while (!fail && *c && *t) {
            pc = *t;

            // Character match
            // TODO: groups
            // TODO: ranges
            if (pc != '(') {
                ++t;

                if (isalnum(pc) || pc == '_') {
                    match_char = pc;
                } else {
                    if (pc == '.') {
                        match_char = PAT_ANY;
                    } else if (pc == '\\') {
                        pc = *t++;

                        switch (pc) {
                        case 'd':
                            match_char = PAT_DIGIT;
                            break;
                        case 'a':
                            match_char = PAT_ALPHA;
                            break;
                        default:
                            fprintf(stderr, "Invalid pattern char: \\%c\n", pc);
                            return 0;
                        }
                    }
                }

                if (*t == '+') {
                    // Match 1 or more
                    match_type = MATCH_ONE_MORE;
                    ++t;
                } else if (*t == '*') {
                    // Match 0 or more
                    match_type = MATCH_ZERO_MORE;
                    ++t;
                } else if (*t == '?') {
                    // Match 0 or 1
                    match_type = MATCH_ONE_OPT;
                    ++t;
                } else {
                    // Match single char
                    match_type = MATCH_ONE;
                }
            }

            if (match_type == MATCH_NONE) {
                fprintf(stderr, "Couldn't parse pattern string\n");
                return 0;
            }

            switch (match_type) {
            case MATCH_ONE:
                if (!char_cmp(*c, match_char)) {
                    fail = 1;
                }
                ++c;
                break;
            case MATCH_ONE_MORE:
                if (!char_cmp(*c, match_char)) {
                    fail = 1;
                    break;
                }
                while (char_cmp(*c, match_char)) {
                    ++c;
                }
                break;
            case MATCH_ZERO_MORE:
                while (char_cmp(*c, match_char)) {
                    ++c;
                }
                break;
            case MATCH_ONE_OPT:
                if (char_cmp(*c, match_char)) {
                    ++c;
                }
                break;
            default:
                fprintf(stderr, "Unhandled pattern\n");
                return 0;
            }
        }

        if (*t && !*c) {
            continue;
        }

        if (!fail) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    char buf[1024];

    if (argc < 2) {
        fprintf(stderr, "usage: %s PATTERN [FILE]", argv[0]);
        return -1;
    }

    const char *pattern = argv[1];

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (grep_match(buf, pattern) != 0) {
            fwrite(buf, 1, strlen(buf), stdout);
        }
    }

    return 0;
}
