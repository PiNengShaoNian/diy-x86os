#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"
#include "dev/tty.h"

int first_task_main(void)
{
#if 0
    int count = 3;
    int pid = getpid();

    print_msg("first task id=%d\n", pid);

    pid = fork();
    if (pid < 0)
    {
        print_msg("create child process failed.\n", 0);
    }
    else if (pid == 0)
    {
        count += 3;
        print_msg("child: %d\n", count);

        char *argv[] = {"arg0", "arg1", "arg2", "arg3"};

        execve("/shell.elf", argv, (char **)0);
    }
    else
    {
        count += 1;
        print_msg("child task id=%d\n", pid);
        print_msg("parent: %d\n", count);
    }
#endif

    for (int i = 0; i < 1; i++)
    {
        int pid = fork();
        if (pid < 0)
        {
            print_msg("create shell failed.", 0);
            break;
        }
        else if (pid == 0)
        {
            char tty_num[5] = "tty:?";
            tty_num[4] = i + '0';
            char *argv[] = {tty_num, (char *)0};
            execve("/shell.elf", argv, (char **)0);
            while (1)
            {
                msleep(1000);
            }
        }
    }

    for (;;)
    {
        // print_msg("task id=%d", pid);
        int status;
        wait(&status);
    }

    return 0;
}