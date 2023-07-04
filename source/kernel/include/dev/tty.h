#ifndef TTY_H
#define TTY_H

#define TTY_NR 8

#define TTY_OBUF_SIZE 512
#define TTY_IBUF_SIZE 512

#include "ipc/sem.h"
#include "dev.h"

typedef struct _tty_fifo_t
{
    char *buf;
    int size;
    int read, write;
    int count;
} tty_fifo_t;

#define TTY_OCRLF (1 << 0)

#define TTY_INCLR (1 << 0)
#define TTY_IECHO (1 << 1)

typedef struct _tty_t
{
    char o_buf[TTY_OBUF_SIZE];
    tty_fifo_t o_fifo;
    sem_t o_sem;
    char i_buf[TTY_IBUF_SIZE];
    tty_fifo_t i_fifo;
    sem_t i_sem;

    int iflags;
    int oflags;
    int console_idx;
} tty_t;

int tty_fifo_put(tty_fifo_t *fifo, char c);

int tty_fifo_get(tty_fifo_t *fifo, char *c);

void tty_fifo_init(tty_fifo_t *fifo, char *buf, int size);

int tty_fifo_put(tty_fifo_t *fifo, char c);

int tty_fifo_get(tty_fifo_t *fifo, char *c);

int tty_open(device_t *dev);

int tty_read(device_t *dev, int addr, char *buf, int size);

int tty_write(device_t *dev, int addr, char *buf, int size);

int tty_control(device_t *dev, int cmd, int arg0, int arg1);

void tty_close(device_t *dev);

void tty_select(int tty);
void tty_in(char ch);

#endif