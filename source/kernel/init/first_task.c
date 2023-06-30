#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"

int first_task_main(void)
{
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
        print_msg("child: %d\n", count);
    }
    else
    {
        print_msg("child task id=%d\n", pid);
        print_msg("parent: %d\n", count);
    }

    for (;;)
    {
        print_msg("task id=%d", pid);
        msleep(1000);
    }

    return 0;
}