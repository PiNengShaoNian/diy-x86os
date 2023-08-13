#include "dbg.h"

#include <stdarg.h>

#include "sys_plat.h"

void dbg_print(int m_level, int s_level, const char *file, const char *func,
               int line, const char *fmt, ...) {
  static const char *title[] = {
      [DBG_LEVEL_NONE] = "none",
      [DBG_LEVEL_ERROR] = DBG_STYLE_ERROR "error",
      [DBG_LEVEL_WARNING] = DBG_STYLE_WARNING "warning",
      [DBG_LEVEL_INFO] = "info",
  };

  if (m_level >= s_level) {
    const char *end = file + plat_strlen(file);
    while (end >= file) {
      if (*end == '\\' || *end == '/') {
        break;
      }

      end--;
    }

    end++;

    plat_printf(title[s_level]);
    plat_printf("(%s-%s-%d): ", end, func, line);

    va_list args;
    char str_buf[128];

    va_start(args, fmt);

    plat_vsprintf(str_buf, fmt, args);
    plat_printf("%s\n" DBG_STYLE_RESET, str_buf);

    va_end(args);
  }
}

void dbg_dump_hwaddr(int module, const char *msg, const uint8_t *hwaddr,
                     int len) {
  if (module < DBG_LEVEL_INFO) {
    return;
  }

  if (msg) {
    plat_printf("%s", msg);
  }

  if (len) {
    for (int i = 0; i < len; i++) {
      plat_printf("%02x-", hwaddr[i]);
    }
  } else {
    plat_printf("none");
  }
}

void dbg_dump_ip(int module, const char *msg, ipaddr_t *ip) {
  if (module < DBG_LEVEL_INFO) {
    return;
  }

  if (msg) {
    plat_printf("%s", msg);
  }

  if (ip) {
    plat_printf("%d.%d.%d.%d\n", ip->a_addr[0], ip->a_addr[1], ip->a_addr[2],
                ip->a_addr[3]);
  } else {
    plat_printf("0.0.0.0\n");
  }
}

void dbg_dump_ip_buf(int module, const char *msg, const uint8_t *ipaddr) {
  if (module < DBG_LEVEL_INFO) {
    return;
  }

  if (msg) {
    plat_printf("%s", msg);
  }

  if (ipaddr) {
    plat_printf("%d.%d.%d.%d\n", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
  } else {
    plat_printf("0.0.0.0\n");
  }
}