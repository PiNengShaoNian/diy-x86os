#ifndef MAIN_H
#define MAIN_H

#define CLI_INPUT_SIZE 1024

#define CLI_MAX_ARG_COUNT 10

typedef struct _cli_cmd_t
{
    const char *name;
    const char *usage;
    int (*do_func)(int argc, char **argv);
} cli_cmd_t;

typedef struct _cli_t
{
    char curr_input[CLI_INPUT_SIZE];
    const cli_cmd_t *cmd_start;
    const cli_cmd_t *cmd_end;
    const char *prompt;
} cli_t;

#endif