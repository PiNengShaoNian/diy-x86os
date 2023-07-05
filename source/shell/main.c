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
    const cli_cmd_t *start = cli.cmd_start;

    while (start < cli.cmd_end)
    {
        printf("%s %s\n", start->name, start->usage);
        start++;
    }

    return 0;
}

static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "help -- he supported command",
        .do_func = do_help,
    },
};

static void show_prompt(void)
{
    printf("%s", cli.prompt);
    fflush(stdout);
}

static const cli_cmd_t *find_builtin(const char *name)
{
    for (const cli_cmd_t *cmd = cli.cmd_start; cmd < cli.cmd_end; cmd++)
    {
        if (strcmp(cmd->name, name) != 0)
            continue;

        return cmd;
    }

    return (const cli_cmd_t *)0;
}

static void run_builtin(const cli_cmd_t *cmd, int argc, char **argv)
{
    int ret = cmd->do_func(argc, argv);
    if (ret < 0)
        fprintf(stderr, "error: %d\n", ret);
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
        char *str = fgets(cli.curr_input, CLI_INPUT_SIZE, stdin);
        if (!str)
            continue;

        char *cr = strchr(cli.curr_input, '\n');
        if (cr)
            *cr = '\0';

        cr = strchr(cli.curr_input, '\r');
        if (cr)
            *cr = '\0';

        int argc = 0;
        char *argv[CLI_MAX_ARG_COUNT];
        memset(argv, 0, sizeof(argv));

        const char *space = " ";
        char *token = strtok(cli.curr_input, space);
        while (token)
        {
            argv[argc++] = token;
            token = strtok(NULL, space);
        }

        if (argc == 0)
            continue;

        const cli_cmd_t *cmd = find_builtin(argv[0]);
        if (cmd)
        {
            run_builtin(cmd, argc, argv);
            continue;
        }
    }
}