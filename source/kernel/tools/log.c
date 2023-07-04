#include <stdarg.h>
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "dev/console.h"
#include "dev/dev.h"
#define COM1_PORT 0x3F8

static mutex_t mutex;

#define LOG_USE_COM 0

static int log_dev_id;

void log_init(void)
{
    mutex_init(&mutex);
    log_dev_id = dev_open(DEV_TTY, 0, (void *)0);

#if LOG_USE_COM
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x3);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xc7);
    outb(COM1_PORT + 4, 0x0F);
#endif
}

void log_printf(const char *fmt, ...)
{
    char str_buf[128];
    va_list args;
    va_start(args, fmt);
    kernel_memset(str_buf, '\0', sizeof(str_buf));
    kernel_vsprintf(str_buf, fmt, args);
    va_end(args);

    mutex_lock(&mutex);

#if USE_LOG_COM
    const char *p = str_buf;
    while (*p != '\0')
    {
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0)
            ;

        outb(COM1_PORT, *p++);
    }

    outb(COM1_PORT, '\r');
    outb(COM1_PORT, '\n');
#else
    // console_write(0, str_buf, kernel_strlen(str_buf));
    dev_write(log_dev_id, 0, str_buf, kernel_strlen(str_buf));
    char c = '\n';
    // console_write(0, &c, 1);
    dev_write(log_dev_id, 0, &c, 1);

#endif

    mutex_unlock(&mutex);
}