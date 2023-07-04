#include "dev/dev.h"

extern dev_desc_t dev_tty_desc;

int dev_open(int major, int minor, void *data)
{
    return -1;
}

int dev_read(int dev_id, int addr, char *buf, int size)
{
    return size;
}

int dev_write(int dev_id, int addr, char *buf, int size)
{
}

int dev_control(int dev_int, int cmd, int arg0, int arg1)
{
    return 0;
}

int dev_close(int dev_id)
{
}