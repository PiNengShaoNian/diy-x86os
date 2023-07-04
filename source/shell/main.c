#include "lib_syscall.h"
#include <stdio.h>

char cmd_buf[1024];

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
    open("tty:0", 0); // int fd = 0, stdin => tty0
    dup(0);           // int fd = 1, stdout => tty0
    dup(0);           // int fd = 2, stderr

    printf("Hello from shell\n");
    printf("os version: %s\n", "1.0.0");
    printf("%d %d %d\n", 1, 2, 3);

    for (;;)
    {
        gets(cmd_buf);
        puts(cmd_buf);
    }
}