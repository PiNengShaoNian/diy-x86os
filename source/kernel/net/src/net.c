#include "net.h"

#include "arp.h"
#include "dbg.h"
#include "dns.h"
#include "ether.h"
#include "exmsg.h"
#include "icmpv4.h"
#include "ipv4.h"
#include "loop.h"
#include "net_plat.h"
#include "netif.h"
#include "pktbuf.h"
#include "raw.h"
#include "sock.h"
#include "tcp.h"
#include "timer.h"
#include "tools.h"
#include "udp.h"

net_err_t net_init(void) {
  dbg_info(DBG_INIT, "init net");

  net_plat_init();

  tools_init();

  exmsg_init();
  pktbuf_init();
  netif_init();

  net_timer_init();

  ether_init();
  arp_init();
  ipv4_init();
  icmpv4_init();

  socket_init();
  raw_init();
  udp_init();
  tcp_init();
  dns_init();

  loop_init();

  return NET_ERR_OK;
}

net_err_t net_start(void) {
  exmsg_start();

  dbg_info(DBG_INIT, "net is running");

  return NET_ERR_OK;
}
