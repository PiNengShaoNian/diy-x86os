#include "lib_syscall.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("abef\b\b\b\bcd\n");
    printf("abcd\x7f;fg\n");
    printf("\0337hello, world!\03381234\n"); // 123o, world!
    printf("\033[31;42mHello,world!\033[39;49m123\n");

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