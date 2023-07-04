#include "dev/dev.h"
#include "cpu/irq.h"
#include "tools/klib.h"

#define DEV_TABLE_SIZE 128

extern dev_desc_t dev_tty_desc;

static dev_desc_t *dev_desc_tbl[] = {
    &dev_tty_desc,

};

static device_t dev_tbl[DEV_TABLE_SIZE];

static int is_devid_bad(int dev_id)
{
    if (dev_id < 0 || dev_id >= DEV_TABLE_SIZE)
        return 1;

    if (dev_tbl[dev_id].desc == (dev_desc_t *)0)
        return 1;

    return 0;
}

int dev_open(int major, int minor, void *data)
{
    irq_state_t state = irq_enter_protection();

    device_t *free_dev = (device_t *)0;
    for (int i = 0; i < sizeof(dev_tbl) / sizeof(dev_tbl[0]); i++)
    {
        device_t *dev = dev_tbl + i;
        if (dev->open_count == 0)
        {
            free_dev = dev;
        }
        else if ((dev->desc->major == major) && (dev->minor == minor))
        {
            dev->open_count++;
            irq_leave_protection(state);
            return i;
        }
    }

    dev_desc_t *desc = (dev_desc_t *)0;
    for (int i = 0; i < sizeof(dev_desc_tbl) / sizeof(dev_desc_tbl[0]); i++)
    {
        dev_desc_t *d = dev_desc_tbl[i];
        if (d->major == major)
        {
            desc = d;
            break;
        }
    }

    if (desc && free_dev)
    {
        free_dev->minor = minor;
        free_dev->data = data;
        free_dev->desc = desc;

        int err = desc->open(free_dev);
        if (err == 0)
        {
            free_dev->open_count = 1;
            irq_leave_protection(state);
            return free_dev - dev_tbl;
        }
    }

    irq_leave_protection(state);
    return -1;
}

int dev_read(int dev_id, int addr, char *buf, int size)
{
    if (is_devid_bad(dev_id))
        return -1;

    device_t *dev = dev_tbl + dev_id;
    return dev->desc->read(dev, addr, buf, size);
}

int dev_write(int dev_id, int addr, char *buf, int size)
{
    if (is_devid_bad(dev_id))
        return -1;

    device_t *dev = dev_tbl + dev_id;
    return dev->desc->write(dev, addr, buf, size);
}

int dev_control(int dev_id, int cmd, int arg0, int arg1)
{
    if (is_devid_bad(dev_id))
        return -1;

    device_t *dev = dev_tbl + dev_id;
    return dev->desc->control(dev, cmd, arg0, arg1);
}

int dev_close(int dev_id)
{
    if (is_devid_bad(dev_id))
        return -1;

    device_t *dev = dev_tbl + dev_id;
    irq_state_t state = irq_enter_protection();

    if (--dev->open_count == 0)
    {
        dev->desc->close(dev);
        kernel_memset(dev, 0, sizeof(device_t));
    }

    irq_leave_protection(state);
}