#ifndef DBG_H
#define DBG_H

#include "ipaddr.h"
#include "net_cfg.h"

#define DBG_STYLE_ERROR "\033[31m"
#define DBG_STYLE_WARNING "\033[33m"
#define DBG_STYLE_RESET "\033[0m"

#define DBG_LEVEL_NONE 0
#define DBG_LEVEL_ERROR 1
#define DBG_LEVEL_WARNING 2
#define DBG_LEVEL_INFO 3

void dbg_print(int m_level, int s_level, const char *file, const char *func,
               int line, const char *fmt, ...);
void dbg_dump_hwaddr(int module, const char *msg, const uint8_t *hwaddr,
                     int len);
void dbg_dump_ip(int module, const char *msg, ipaddr_t *ip);
void dbg_dump_ip_buf(int module, const char *msg, const uint8_t *ipaddr);

#define dbg_info(module, fmt, ...)                                         \
  dbg_print(module, DBG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, \
            ##__VA_ARGS__)

#define dbg_warning(module, fmt, ...)                                         \
  dbg_print(module, DBG_LEVEL_WARNING, __FILE__, __FUNCTION__, __LINE__, fmt, \
            ##__VA_ARGS__)

#define dbg_error(module, fmt, ...)                                         \
  dbg_print(module, DBG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, \
            ##__VA_ARGS__)

#define dbg_assert(expr, msg)                                             \
  do {                                                                    \
    if (!(expr)) {                                                        \
      dbg_print(DBG_LEVEL_ERROR, DBG_LEVEL_ERROR, __FILE__, __FUNCTION__, \
                __LINE__, "assert failed: " #expr "," msg);               \
      while (1)                                                           \
        ;                                                                 \
    }                                                                     \
  } while (0);

#define DBG_DISP_ENABLED(module) (module >= DBG_LEVEL_INFO)

#endif