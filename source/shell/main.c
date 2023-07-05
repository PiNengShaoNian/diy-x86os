#include "lib_syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

char cmd_buf[1024];

static cli_t cli;
static const char *prompt = "sh >>";

static int do_help(int argc, char **argv)
{
    return 0;
}

static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "help -- list supported command",
        .do_func = do_help,
    },
};

static void show_prompt(void)
{
    printf("%s", cli.prompt);
    fflush(stdout);
}

static void cli_init(const char *prompt, const cli_cmd_t *cmd_list, int size)
{
    cli.prompt = prompt;
    memset(cli.curr_input, 0, CLI_INPUT_SIZE);
    cli.cmd_start = cmd_list;
    cli.cmd_end = cmd_list + size;
}

int main(int argc, char **argv)
{

    open(argv[0], 0); // int fd = 0, stdin => tty0
    dup(0);           // int fd = 1, stdout => tty0
    dup(0);           // int fd = 2, stderr

    printf("Hello from shell\n");
    printf("os version: %s\n", "1.0.0");
    printf("%d %d %d\n", 1, 2, 3);

    cli_init(prompt, cmd_list, sizeof(cmd_list) / sizeof(cmd_list[0]));

    for (;;)
    {
        show_prompt();
        gets(cli.curr_input);
    }
}