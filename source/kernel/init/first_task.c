#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"

int first_task_main(void)
{
    int pid = getpid();

    for (;;)
    {
        msleep(1000);
    }

    return 0;
}