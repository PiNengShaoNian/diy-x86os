#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_PARAM_COUNT 5

#define SYS_msleep 0
#define SYS_getpid 1
#define SYS_fork 2
#define SYS_execve 3
#define SYS_yield 4

#define SYS_open 50
#define SYS_read 51
#define SYS_write 52
#define SYS_close 53
#define SYS_lseek 54
#define SYS_isatty 55
#define SYS_fstat 56
#define SYS_sbrk 57
#define SYS_dup 58
#define SYS_exit 59
#define SYS_wait 60

#define SYS_printmsg 100

void exception_handler_syscall(void);

typedef struct _syscall_frame_t
{
    int eflags;
    int gs, fs, es, ds;
    int edi, esi, ebp, dummy, ebx, edx, ecx, eax;
    int eip, cs;
    int func_id, arg0, arg1, arg2, arg3;
    int esp, ss;
} syscall_frame_t;

#endif