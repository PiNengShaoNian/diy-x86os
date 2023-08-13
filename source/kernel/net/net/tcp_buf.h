#ifndef TCP_BUFF_H
#define TCP_BUFF_H

#include <stdint.h>

#include "pktbuf.h"

typedef struct _tcp_buf_t {
  uint8_t *data;
  int count;
  int size;
  int in, out;
} tcp_buf_t;

void tcp_buf_init(tcp_buf_t *buf, uint8_t *data, int size);

static inline int tcp_buf_size(tcp_buf_t *buf) { return buf->size; }

static inline int tcp_buf_cnt(tcp_buf_t *buf) { return buf->count; }

static inline int tcp_buf_free_cout(tcp_buf_t *buf) {
  return buf->size - buf->count;
}

void tcp_buf_write_send(tcp_buf_t *buf, const uint8_t *buffer, int len);
void tcp_buf_read_send(tcp_buf_t *buf, int offset, pktbuf_t *dest, int count);
int tcp_buf_write_rcv(tcp_buf_t *dest, int offset, pktbuf_t *src, int total);
int tcp_buf_read_rcv(tcp_buf_t *buf, uint8_t *dest, int count);
int tcp_buf_remove(tcp_buf_t *buf, int cnt);

#endif