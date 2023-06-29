#include "core/syscall.h"
#include "core/task.h"
#include "tools/log.h"

typedef int (*syscall_handler_t)(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
static const syscall_handler_t sys_table[] = {
    [SYS_sleep] = (syscall_handler_t)sys_sleep,
};

void do_handler_syscall(syscall_frame_t *frame)
{
    if (frame->func_id < sizeof(sys_table) / sizeof(sys_table[0]))
    {
        syscall_handler_t handler = sys_table[frame->func_id];
        if (handler)
        {
            int ret = handler(frame->arg0, frame->arg1, frame->arg2, frame->arg3);
            return;
        }
    }

    task_t *task = task_current();
    log_printf("task: %s, Unknown syscall: %d", task->name, frame->func_id);
}