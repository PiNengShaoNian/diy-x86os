#include "lib_syscall.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("abef\b\b\b\bcd\n");
    printf("abcd\x7f;fg\n");

    printf("Hello from shell\n");
    printf("os version: %s\n", "1.0.0");
    printf("%d %d %d\n", 1, 2, 3);

    for (int i = 0; i < argc; i++)
    {
        printf("arg: %s\n", argv[i]);
    }
    fork();
    yield();

    for (;;)
    {
        printf("shell pid=%d\n", getpid());
        msleep(1000);
    }
}