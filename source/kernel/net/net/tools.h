#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>

#include "ipaddr.h"
#include "net_cfg.h"
#include "net_err.h"
#include "pktbuf.h"

net_err_t tools_init(void);

static inline uint16_t swap_u16(uint16_t v) {
  uint16_t r = ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
  return r;
}

static inline uint32_t swap_u32(uint32_t v) {
  uint32_t r = (((v >> 0) & 0xFF) << 24) | (((v >> 8) & 0xFF) << 16) |
               (((v >> 16) & 0xFF) << 8) | (((v >> 24) & 0xFF) << 0);
  return r;
}

#if NET_ENDIAN_LITTLE
#define x_htons(v) swap_u16(v)
#define x_ntohs(v) swap_u16(v)
#define x_htonl(v) swap_u32(v)
#define x_ntohl(v) swap_u32(v)
#else
#define x_htons(v) v
#define x_ntohs(v) v
#define x_htonl(v) v
#define x_ntohl(v) v
#endif

uint16_t checksum_16(uint32_t offset, void *buf, uint16_t len, uint32_t pre_sum,
                     int complement);
uint16_t checksum_peso(pktbuf_t *buf, const ipaddr_t *dest, const ipaddr_t *src,
                       uint8_t protocol);

#endif