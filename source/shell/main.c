#include "lib_syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/file.h>
#include "fs/file.h"
#include "dev/tty.h"

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

static int do_clear(int argc, char **argv)
{
    printf("%s", ESC_CLEAR_SCREEN);
    printf("%s", ESC_MOVE_CURSOR(0, 0));

    return 0;
}

static int do_echo(int argc, char **argv)
{
    if (argc == 1)
    {
        char msg_buf[128];
        fgets(msg_buf, sizeof(msg_buf), stdin);
        msg_buf[sizeof(msg_buf) - 1] = '\0';
        puts(msg_buf);
        return 0;
    }

    int count = 1;
    int ch;
    optind = 1;
    while ((ch = getopt(argc, argv, "n:h")) != -1)
    {
        switch (ch)
        {
        case 'h':
            puts("echo any message");
            puts("Usage: echo [-n count] message");
            return 0;
        case 'n':
            count = atoi(optarg);
            break;
        case '?':
            if (optarg)
                fprintf(stderr, "Unknown option: -%s\n", optarg);

            return -1;
        default:
            break;
        }
    }

    if (optind > argc - 1)
    {
        fprintf(stderr, "Message is empty\n");
        return -1;
    }

    char *msg = argv[optind];
    for (int i = 0; i < count; i++)
        puts(msg);

    return 0;
}

static void do_exit(int argc, char **argv)
{
    exit(0);
}

static int do_ls(int argc, char **argv)
{
    DIR *p_dir = opendir("temp");
    if (p_dir == NULL)
    {
        printf("open dir failed.");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(p_dir)) != NULL)
    {
        printf("%c %s %d\n",
               entry->type == FILE_DIR ? 'd' : 'f',
               entry->name, entry->size);
    }

    closedir(p_dir);
}

static int do_less(int argc, char **argv)
{
    int line_mode = 0;

    int ch;
    optind = 1;
    while ((ch = getopt(argc, argv, "l")) != -1)
    {
        switch (ch)
        {
        case 'h':
            puts("show file content");
            puts("Usage: less [-l] file");
            return 0;
        case 'l':
            line_mode = 1;
            break;
        case '?':
            if (optarg)
                fprintf(stderr, "Unknown option: -%s\n", optarg);

            return -1;
        default:
            break;
        }
    }

    if (optind > argc - 1)
    {
        fprintf(stderr, "no file\n");
        return -1;
    }

    FILE *file = fopen(argv[optind], "r");
    if (file == NULL)
    {
        fprintf(stderr, "open file failed. %s", argv[optind]);
        return -1;
    }

    char *buf = (char *)malloc(255);

    if (line_mode == 0)
    {
        while (fgets(buf, 255, file) != NULL)
        {
            fputs(buf, stdout);
        }
    }
    else
    {
        setvbuf(stdin, NULL, _IONBF, 0);
        ioctl(0, TTY_CMD_ECHO, 0, 0);
        while (1)
        {
            char *b = fgets(buf, 255, file);
            if (b == NULL)
                break;

            puts(buf);

            int ch;
            while ((ch = getc(stdin)) != 'n')
            {
                if (ch == 'q')
                    goto less_quit;
            }
        }
    less_quit:
        setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
        ioctl(0, TTY_CMD_ECHO, 1, 1);
    }
    free(buf);
    fclose(file);

    return 0;
}

static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "help -- supported command",
        .do_func = do_help,
    },
    {
        .name = "clear",
        .usage = "clear -- clear screen",
        .do_func = do_clear,
    },
    {
        .name = "echo",
        .usage = "echo [-n count] msg",
        .do_func = do_echo,
    },
    {
        .name = "ls",
        .usage = "ls -- list directory",
        .do_func = do_ls,
    },
    {
        .name = "less",
        .usage = "less [-l] -- show file",
        .do_func = do_less,
    },
    {
        .name = "quit",
        .usage = "quit from shell",
        .do_func = (int (*)(int, char **))do_exit,
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

static void run_exec_file(const char *path, int argc, char **argv)
{
    int pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork failed %s", path);
    }
    else if (pid == 0)
    {
        for (int i = 0; i < argc; i++)
            printf("arg %d = %s\n", i, argv[i]);
        exit(-1);
    }
    else
    {
        int status;
        int pid = wait(&status);
        fprintf(stderr, "cmd %s result: %d, pid=%d\n", path, status, pid);
    }
}

int main(int argc, char **argv)
{

    open(argv[0], O_RDWR); // int fd = 0, stdin => tty0
    dup(0);                // int fd = 1, stdout => tty0
    dup(0);                // int fd = 2, stderr

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

        run_exec_file("", argc, argv);

        fprintf(stderr, ESC_COLOR_ERROR "Unknown command: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
    }
}