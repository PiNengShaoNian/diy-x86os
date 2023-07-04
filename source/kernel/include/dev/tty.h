#ifndef TTY_H
#define TTY_H

#define TTY_NR 8

#define TTY_OBUF_SIZE 512
#define TTY_IBUF_SIZE 512

typedef struct _tty_fifo_t
{
    char *buf;
    int size;
    int read, write;
    int count;
} tty_fifo_t;

typedef struct _tty_t
{
    char o_buf[TTY_OBUF_SIZE];
    tty_fifo_t o_fifo;
    char i_buf[TTY_IBUF_SIZE];
    tty_fifo_t i_fifo;
    int console_idx;
} tty_t;

#endif