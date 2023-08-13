#ifndef TCH_OUT_H
#define TCH_OUT_H

#include "tcp.h"

typedef enum _tcp_oevent_t {
  TCP_OEVENT_SEND,
} tcp_oevent_t;

net_err_t tcp_send_reset(tcp_seg_t *seg);
net_err_t tcp_send_syn(tcp_t *tcp);
net_err_t tcp_ack_process(tcp_t *tcp, tcp_seg_t *seg);
net_err_t tcp_send_ack(tcp_t *tcp, tcp_seg_t *seg);
net_err_t tcp_send_fin(tcp_t *tcp);
int tcp_write_sndbuf(tcp_t *tcp, const uint8_t *buf, int len);
net_err_t tcp_transmit(tcp_t *tcp);

net_err_t tcp_send_keepalive(tcp_t *tcp);
net_err_t tcp_send_reset_for_tcp(tcp_t *tcp);

const char *tcp_ostate_name(tcp_t *tcp);

void tcp_out_event(tcp_t *tcp, tcp_oevent_t event);
void tcp_set_ostate(tcp_t *tcp, tcp_ostate_t state);

#endif