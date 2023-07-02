#include "lib_syscall.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("Hello from shell\n");

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