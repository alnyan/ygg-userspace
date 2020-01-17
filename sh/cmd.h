#pragma once

struct cmd_exec {
    size_t argc;
    char *args[12];
};

int eval(char *str);
