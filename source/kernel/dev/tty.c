#include "dev/tty.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "dev/kbd.h"
#include "dev/console.h"

static tty_t tty_devs[TTY_NR];

dev_desc_t dev_tty_desc = {
    .name = "tty",
    .major = DEV_TTY,
    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close,
};

static tty_t *get_tty(device_t *dev)
{
    int tty = dev->minor;
    if (tty < 0 || (tty >= TTY_NR) || !dev->open_count)
    {
        log_printf("tty is not opened. tty=%d", tty);
        return (tty_t *)0;
    }

    return tty_devs + tty;
}

void tty_fifo_init(tty_fifo_t *fifo, char *buf, int size)
{
    fifo->buf = buf;
    fifo->count = 0;
    fifo->size = size;
    fifo->read = fifo->write = 0;
}

int tty_fifo_put(tty_fifo_t *fifo, char c)
{
    if (fifo->count >= fifo->size)
        return -1;

    fifo->buf[fifo->write++] = c;
    if (fifo->write >= fifo->size)
        fifo->write = 0;

    fifo->count++;

    return 0;
}

int tty_fifo_get(tty_fifo_t *fifo, char *c)
{
    if (fifo->count <= 0)
        return -1;

    *c = fifo->buf[fifo->read++];
    if (fifo->read >= fifo->size)
        fifo->read = 0;

    fifo->count--;

    return 0;
}

int tty_open(device_t *dev)
{
    int idx = dev->minor;
    if (idx < 0 || idx >= TTY_NR)
    {
        log_printf("open tty failed. incorrect tty num =%d", idx);
        return -1;
    }

    tty_t *tty = tty_devs + idx;
    tty_fifo_init(&tty->o_fifo, tty->o_buf, TTY_OBUF_SIZE);
    sem_init(&tty->o_sem, TTY_OBUF_SIZE);
    tty_fifo_init(&tty->i_fifo, tty->i_buf, TTY_IBUF_SIZE);
    tty->oflags = TTY_OCRLF;
    tty->console_idx = idx;

    kbd_init();
    console_init(idx);

    return 0;
}

int tty_read(device_t *dev, int addr, char *buf, int size)
{
    return size;
}

int tty_write(device_t *dev, int addr, char *buf, int size)
{
    if (size < 0)
        return -1;

    tty_t *tty = get_tty(dev);
    if (!tty)
        return -1;

    int len = 0;
    while (size)
    {
        char c = *buf++;

        if (c == '\n' && (tty->oflags & TTY_OCRLF))
        {
            sem_wait(&tty->o_sem);
            int err = tty_fifo_put(&tty->o_fifo, '\r');
            if (err < 0)
                break;
        }

        sem_wait(&tty->o_sem);
        int err = tty_fifo_put(&tty->o_fifo, c);
        if (err < 0)
            break;

        len++;
        size--;

        console_write(tty);
    }

    return size;
}

int tty_control(device_t *dev, int cmd, int arg0, int arg1)
{
    return 0;
}

void tty_close(device_t *dev)
{
}
