#include "lib_syscall.h"
#include <stdio.h>

int main(int argc, char **argv)
{
#if 0
    printf("abef\b\b\b\bcd\n");
    printf("abcd\x7f;fg\n");
    printf("\0337hello, world!\03381234\n"); // 123o, world!
    printf("\033[31;42mHello,world!\033[39;49m123\n");

    printf("123\033[2DHello,world!\n"); // 光标左移2, 1Hello,world!
    printf("123\033[2CHello,world!\n"); // 光标右移2, 123  Hello,world!

    printf("\033[31m");             // ESC [pn m, Hello,world红色其余绿色
    printf("\033[10;10H test!\n");  // 定位到10, 10, test!
    printf("\033[20;20H test!\n");  // 定位到20, 20, test!
    printf("\033[32;25;39m123!\n"); // ESC [pn m, Hello,world红色，其余绿色

    printf("\033[2J");
#endif
    open("tty:0,", 0);
    printf("Hello from shell\n");
    printf("os version: %s\n", "1.0.0");
    printf("%d %d %d\n", 1, 2, 3);

    for (;;)
    {
        printf("shell pid=%d\n", getpid());
        msleep(1000);
    }
}