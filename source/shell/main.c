#include "lib_syscall.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("Hello from shell\n");
    printf("os version: %s\n", "1.0.0");
    printf("%d %d %d\n", 1, 2, 3);

    for (int i = 0; i < argc; i++)
    {
        print_msg("arg: %s", argv[i]);
    }
    fork();
    yield();

    for (;;)
    {
        print_msg("shell pid=%d", getpid());
        msleep(1000);
    }
}