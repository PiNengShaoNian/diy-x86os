#include "lib_syscall.h"

int main(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        print_msg("arg: %s", argv[i]);
    }

    for (;;)
    {
        msleep(1000);
    }
}