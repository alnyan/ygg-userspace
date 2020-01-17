#pragma once

struct cmd_exec;

int builtin_exec(const struct cmd_exec *cmd, int *res);
