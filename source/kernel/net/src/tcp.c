#include "tcp.h"

#include "dbg.h"
#include "mblock.h"
#include "protocol.h"
#include "socket.h"
#include "tcp_out.h"
#include "tcp_state.h"
#include "tools.h"

static tcp_t tcp_tbl[TCP_MAX_NR];
static mblock_t tcp_mblock;
static nlist_t tcp_list;

#if DBG_DISP_ENABLED(DBG_TCP)
void tcp_show_info(char *msg, tcp_t *tcp) {
  plat_printf("%s\n", msg);
  plat_printf("  local port: %u, remote port: %u\n", tcp->base.local_port,
              tcp->base.remote_port);
}

void tcp_show_pkt(char *msg, tcp_hdr_t *tcp_hdr, pktbuf_t *buf) {
  plat_printf("%s\n", msg);
  plat_printf("  sport: %u, dport: %u\n", tcp_hdr->sport, tcp_hdr->dport);
  plat_printf("  seq: %u, ack: %u, win: %u\n", tcp_hdr->seq, tcp_hdr->ack,
              tcp_hdr->win);
  plat_printf("  flags:");
  if (tcp_hdr->f_syn) {
    plat_printf(" syn");
  }
  if (tcp_hdr->f_rst) {
    plat_printf(" reset");
  }
  if (tcp_hdr->ack) {
    plat_printf(" ack");
  }
  if (tcp_hdr->f_psh) {
    plat_printf(" push");
  }
  if (tcp_hdr->f_fin) {
    plat_printf(" finish");
  }
  plat_printf("\nlen=%d\n", buf->total_size - tcp_hdr_size(tcp_hdr));
}

void tcp_show_list(void) {
  plat_printf("---------tcp list--------\n");
  nlist_node_t *node;
  nlist_for_each(node, &tcp_list) {
    tcp_t *tcp = (tcp_t *)nlist_entry(node, sock_t, node);

    tcp_show_info("", tcp);
  }
}
#endif

net_err_t tcp_init(void) {
  dbg_info(DBG_TCP, "tcp init.");

  nlist_init(&tcp_list);
  mblock_init(&tcp_mblock, tcp_tbl, sizeof(tcp_t), TCP_MAX_NR, NLOCKER_NONE);

  dbg_info(DBG_TCP, "init done.");
  return NET_ERR_OK;
}

static tcp_t *tcp_get_free(int wait) {
  tcp_t *tcp = mblock_alloc(&tcp_mblock, wait ? 0 : -1);
  if (!tcp) {
    nlist_node_t *node;
    nlist_for_each(node, &tcp_list) {
      tcp_t *s = (tcp_t *)nlist_entry(node, sock_t, node);
      if (s->state == TCP_STATE_TIME_WAIT) {
        net_timer_remove(&s->conn.keep_timer);
        tcp_free(s);
        return (tcp_t *)mblock_alloc(&tcp_mblock, -1);
      }
    }

    dbg_error(DBG_TCP, "no tcp sock");
    return (tcp_t *)0;
  }

  return tcp;
}

int tcp_alloc_port(void) {
#if 0
  srand((unsigned int)time(NULL));
  int search_idx = rand() % 1000 + NET_PORT_DYN_START;
#else
  static int search_idx = NET_PORT_DYN_START;
#endif

  for (int i = NET_PORT_DYN_START; i < NET_PORT_DYN_END; i++) {
    nlist_node_t *node;
    nlist_for_each(node, &tcp_list) {
      sock_t *sock = nlist_entry(node, sock_t, node);
      if (sock->local_port == search_idx) {
        break;
      }
    }

    if (!node) {
      return search_idx;
    }

    if (++search_idx >= NET_PORT_DYN_END) {
      search_idx = NET_PORT_DYN_START;
    }
  }

  return -1;
}

static uint32_t tcp_get_iss(void) {
  static uint32_t seq = 0;

  return ++seq;
}

static net_err_t tcp_init_connect(tcp_t *tcp) {
  rentry_t *rt = rt_find(&tcp->base.remote_ip);
  if (rt->netif->mtu == 0) {
    tcp->mss = TCP_DEFAULT_MSS;
  } else if (!ipaddr_is_any(&rt->next_hop)) {
    tcp->mss = TCP_DEFAULT_MSS;
  } else {
    tcp->mss = rt->netif->mtu - sizeof(ipv4_hdr_t) - sizeof(tcp_hdr_t);
  }

  tcp_buf_init(&tcp->snd.buf, tcp->snd.data, TCP_SBUF_SIZE);
  tcp_buf_init(&tcp->rcv.buf, tcp->rcv.data, TCP_RBUF_SIZE);
  tcp->snd.iss = tcp_get_iss();
  tcp->snd.una = tcp->snd.nxt = tcp->snd.iss;

  tcp->rcv.iss = 0;
  return NET_ERR_OK;
}

net_err_t tcp_connect(struct _sock_t *s, const struct x_sockaddr *addr,
                      x_socklen_t addr_len) {
  tcp_t *tcp = (tcp_t *)s;
  const struct x_sockaddr_in *addr_in = (const struct x_sockaddr_in *)addr;

  if (tcp->state != TCP_STATE_CLOSED) {
    dbg_error(DBG_TCP, "tcp is not closed.");
    return NET_ERR_STATE;
  }

  ipaddr_from_buf(&s->remote_ip, (const uint8_t *)&addr_in->sin_addr.s_addr);
  s->remote_port = x_ntohs(addr_in->sin_port);

  if (s->local_port == NET_PORT_EMPTY) {
    int port = tcp_alloc_port();
    if (port == -1) {
      dbg_error(DBG_TCP, "alloc port failed.");
      return NET_ERR_NONE;
    }

    s->local_port = port;
  }

  if (ipaddr_is_any(&s->local_ip)) {
    rentry_t *rt = rt_find(&s->remote_ip);
    if (rt == (rentry_t *)0) {
      dbg_error(DBG_TCP, "no route to host");
      return NET_ERR_UNREACH;
    }

    ipaddr_copy(&s->local_ip, &rt->netif->ipaddr);
  }

  net_err_t err;
  if ((err = tcp_init_connect(tcp)) < 0) {
    dbg_error(DBG_TCP, "init conn failed.");
    return err;
  }

  if ((err = tcp_send_syn(tcp)) < 0) {
    dbg_error(DBG_TCP, "send syn failed.");
    return err;
  }

  tcp_set_state(tcp, TCP_STATE_SYN_SENT);
  return NET_ERR_NEED_WAIT;
}

void tcp_free(tcp_t *tcp) {
  sock_wait_destroy(&tcp->conn.wait);
  sock_wait_destroy(&tcp->snd.wait);
  sock_wait_destroy(&tcp->rcv.wait);

  nlist_remove(&tcp_list, &tcp->base.node);
  mblock_free(&tcp_mblock, tcp);
}

void tcp_clear_parent(tcp_t *tcp) {
  nlist_node_t *node;

  nlist_for_each(node, &tcp_list) {
    tcp_t *child = (tcp_t *)nlist_entry(node, sock_t, node);
    if (child->parent == tcp) {
      child->parent = (tcp_t *)0;
    }
  }
}

net_err_t tcp_close(struct _sock_t *sock) {
  tcp_t *tcp = (tcp_t *)sock;

  switch (tcp->state) {
    case TCP_STATE_CLOSED:
      dbg_info(DBG_TCP, "tcp already closed.");
      tcp_free(tcp);
      return NET_ERR_OK;
    case TCP_STATE_LISTEN:
      tcp_clear_parent(tcp);
      tcp_abort(tcp, NET_ERR_CLOSE);
      tcp_free(tcp);
      return NET_ERR_OK;
    case TCP_STATE_SYN_SENT:
    case TCP_STATE_SYN_RECVD:
      tcp_abort(tcp, NET_ERR_CLOSE);
      tcp_free(tcp);
      return NET_ERR_OK;
    case TCP_STATE_CLOSE_WAIT:
      tcp_send_fin(tcp);
      tcp_set_state(tcp, TCP_STATE_LAST_ACK);
      return NET_ERR_NEED_WAIT;
    case TCP_STATE_ESTABLISHED:
      tcp_send_fin(tcp);
      tcp_set_state(tcp, TCP_STATE_FIN_WAIT_1);
      return NET_ERR_NEED_WAIT;
    default:
      dbg_error(DBG_TCP, "tcp state error.");
      return NET_ERR_STATE;
  }

  return NET_ERR_OK;
}

static net_err_t tcp_send(struct _sock_t *s, const void *buf, size_t len,
                          int flags, ssize_t *result_len) {
  tcp_t *tcp = (tcp_t *)s;

  switch (tcp->state) {
    case TCP_STATE_CLOSED:
      dbg_error(DBG_TCP, "tcp closed.");
      return NET_ERR_CLOSE;
    case TCP_STATE_FIN_WAIT_1:
    case TCP_STATE_FIN_WAIT_2:
    case TCP_STATE_TIME_WAIT:
    case TCP_STATE_LAST_ACK:
    case TCP_STATE_CLOSING:
      dbg_error(DBG_TCP, "tcp closed.");
      return NET_ERR_CLOSE;
    case TCP_STATE_CLOSE_WAIT:
    case TCP_STATE_ESTABLISHED:
      break;
    case TCP_STATE_LISTEN:
    case TCP_STATE_SYN_RECVD:
    case TCP_STATE_SYN_SENT:
    default:
      dbg_error(DBG_TCP, "tcp state error");
      return NET_ERR_STATE;
  }

  int size = tcp_write_sndbuf(tcp, (uint8_t *)buf, (int)len);
  if (size <= 0) {
    *result_len = 0;
    return NET_ERR_NEED_WAIT;
  } else {
    *result_len = size;

    tcp_out_event(tcp, TCP_OEVENT_SEND);
    // tcp_transmit(tcp);

    return NET_ERR_OK;
  }
}

net_err_t tcp_recv(struct _sock_t *sock, void *buf, size_t len, int flags,
                   ssize_t *result_len) {
  tcp_t *tcp = (tcp_t *)sock;

  int need_wait = NET_ERR_NEED_WAIT;
  switch (tcp->state) {
    case TCP_STATE_LAST_ACK:
    case TCP_STATE_CLOSED:
      dbg_error(DBG_TCP, "tcp closed.");
      return NET_ERR_CLOSE;
    case TCP_STATE_CLOSE_WAIT:
    case TCP_STATE_CLOSING:
      if (tcp_buf_cnt(&tcp->rcv.buf) == 0) {
        return NET_ERR_CLOSE;
      }
      need_wait = NET_ERR_OK;
      break;
    case TCP_STATE_FIN_WAIT_1:
    case TCP_STATE_FIN_WAIT_2:
    case TCP_STATE_ESTABLISHED:
      break;
    case TCP_STATE_LISTEN:
    case TCP_STATE_SYN_SENT:
    case TCP_STATE_SYN_RECVD:
    case TCP_STATE_TIME_WAIT:
    default:
      dbg_error(DBG_TCP, "tcp state error");
      return NET_ERR_STATE;
  }

  *result_len = 0;
  int cnt = tcp_buf_read_rcv(&tcp->rcv.buf, buf, (int)len);
  if (cnt > 0) {
    *result_len = cnt;
    return NET_ERR_OK;
  }
  return need_wait;
}

net_err_t tcp_setopt(struct _sock_t *s, int level, int optname,
                     const char *optval, int optlen) {
  net_err_t err = sock_setopt(s, level, optname, optval, optlen);
  if (err == NET_ERR_OK) {
    return NET_ERR_OK;
  } else if ((err < 0) && (err != NET_ERR_UNKNOWN)) {
    return err;
  }

  tcp_t *tcp = (tcp_t *)s;

  if (level == SOL_SOCKET) {
    if (optname == SO_KEEPALIVE) {
      if (optlen != sizeof(int)) {
        dbg_error(DBG_TCP, "param size error");
        return NET_ERR_PARAM;
      }
      tcp_keepalive_start(tcp, *(int *)optval);
      return NET_ERR_OK;
    }

    return NET_ERR_PARAM;
  } else if (level == SOL_TCP) {
    switch (optname) {
      case TCP_KEEPIDLE: {
        if (optlen != sizeof(int)) {
          dbg_error(DBG_TCP, "param size error");
          return NET_ERR_PARAM;
        }
        tcp->conn.keep_idle = *(int *)optval;
        tcp_keepalive_restart(tcp);
        break;
      }
      case TCP_KEEPINTVL: {
        if (optlen != sizeof(int)) {
          dbg_error(DBG_TCP, "param size error");
          return NET_ERR_PARAM;
        }
        tcp->conn.keep_intvl = *(int *)optval;
        tcp_keepalive_restart(tcp);
        break;
      }
      case TCP_KEEPCNT: {
        if (optlen != sizeof(int)) {
          dbg_error(DBG_TCP, "param size error");
          return NET_ERR_PARAM;
        }
        tcp->conn.keep_cnt = *(int *)optval;
        tcp_keepalive_restart(tcp);
        break;
      }
      default:
        dbg_error(DBG_TCP, "unknown param");
        return NET_ERR_PARAM;
    }
  }

  return NET_ERR_OK;
}

net_err_t tcp_bind(struct _sock_t *s, const struct x_sockaddr *addr,
                   x_socklen_t addr_len) {
  tcp_t *tcp = (tcp_t *)s;

  if (tcp->state != TCP_STATE_CLOSED) {
    dbg_error(DBG_TCP, "state error");
    return NET_ERR_STATE;
  }

  if (s->local_port != NET_PORT_EMPTY) {
    dbg_error(DBG_TCP, "already bind.");
    return NET_ERR_PARAM;
  }

  const struct x_sockaddr_in *addr_in = (const struct x_sockaddr_in *)addr;
  if (addr_in->sin_port == NET_PORT_EMPTY) {
    dbg_error(DBG_TCP, "port is empty");
    return NET_ERR_PARAM;
  }

  ipaddr_t local_ip;
  ipaddr_from_buf(&local_ip, (const uint8_t *)&addr_in->sin_addr.addr_array);
  if (!ipaddr_is_any(&local_ip)) {
    rentry_t *rt = rt_find(&local_ip);
    if (rt == (rentry_t *)0) {
      dbg_error(DBG_TCP, "ip addr error");
      return NET_ERR_ADDR;
    }

    if (!ipaddr_is_equal(&local_ip, &rt->netif->ipaddr)) {
      dbg_error(DBG_TCP, "ipaddr error");
      return NET_ERR_ADDR;
    }
  }

  nlist_node_t *node;
  nlist_for_each(node, &tcp_list) {
    sock_t *curr = (sock_t *)nlist_entry(node, sock_t, node);

    if (curr == s) {
      continue;
    }

    // server socket don't need remote port but a queue
    if (curr->remote_port != NET_PORT_EMPTY) {
      continue;
    }

    if (ipaddr_is_equal(&curr->local_ip, &local_ip) &&
        curr->local_port == addr_in->sin_port) {
      dbg_error(DBG_TCP, "ipaddr and port already bind.");
      return NET_ERR_ADDR;
    }
  }

  ipaddr_copy(&s->local_ip, &local_ip);
  s->local_port = x_ntohs(addr_in->sin_port);

  return NET_ERR_OK;
}

net_err_t tcp_listen(struct _sock_t *s, int backlog) {
  tcp_t *tcp = (tcp_t *)s;

  if (tcp->state != TCP_STATE_CLOSED) {
    dbg_error(DBG_TCP, "tcp state error");
    return NET_ERR_STATE;
  }

  tcp->state = TCP_STATE_LISTEN;
  tcp->conn.backlog = backlog;
  return NET_ERR_OK;
}

net_err_t tcp_accept(struct _sock_t *s, struct x_sockaddr *addr,
                     x_socklen_t *len, struct _sock_t **client) {
  nlist_node_t *node;
  nlist_for_each(node, &tcp_list) {
    sock_t *sock = nlist_entry(node, sock_t, node);
    tcp_t *tcp = (tcp_t *)sock;

    if (sock == s) {
      continue;
    }

    if (tcp->parent != (tcp_t *)s) {
      continue;
    }

    if (tcp->flags.inactive && tcp->state == TCP_STATE_ESTABLISHED) {
      struct x_sockaddr_in *addr_in = (struct x_sockaddr_in *)addr;
      plat_memset(addr_in, 0, *len);
      addr_in->sin_family = AF_INET;
      addr_in->sin_port = x_htons(tcp->base.remote_port);
      ipaddr_to_buf(&tcp->base.remote_ip, (uint8_t *)&addr_in->sin_addr.s_addr);
      if (len) {
        *len = sizeof(struct x_sockaddr_in);
      }

      tcp->flags.inactive = 0;
      *client = sock;
      return NET_ERR_OK;
    }
  }

  return NET_ERR_NEED_WAIT;
}

int tcp_backlog_count(tcp_t *tcp) {
  int count = 0;

  nlist_node_t *node;
  nlist_for_each(node, &tcp_list) {
    tcp_t *child = (tcp_t *)nlist_entry(node, sock_t, node);
    if (child->parent == tcp && child->flags.inactive) {
      count++;
    }
  }

  return count;
}

static tcp_t *tcp_alloc(int wait, int family, int protocol);
static void tcp_insert(tcp_t *tcp);
tcp_t *tcp_create_child(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_t *child = (tcp_t *)tcp_alloc(0, tcp->base.family, tcp->base.protocol);
  if (!child) {
    dbg_error(DBG_TCP, "no child tcp");
    return (tcp_t *)0;
  }
  tcp_insert(child);
  ipaddr_copy(&child->base.local_ip, &seg->local_ip);
  ipaddr_copy(&child->base.remote_ip, &seg->remote_ip);
  child->base.local_port = seg->hdr->dport;
  child->base.remote_port = seg->hdr->sport;
  child->parent = tcp;
  child->flags.irs_valid = 1;
  child->flags.inactive = 1;

  tcp_init_connect(child);
  child->rcv.iss = seg->seq;
  child->rcv.nxt = child->rcv.iss + 1;
  tcp_read_options(child, seg->hdr);

  return child;
}

void tcp_destroy(struct _sock_t *s) {
  tcp_t *tcp = (tcp_t *)s;

  if (tcp->state == TCP_STATE_TIME_WAIT) {
    return;
  }

  tcp_kill_all_timers(tcp);
  tcp_free((tcp_t *)s);
}

static tcp_t *tcp_alloc(int wait, int family, int protocol) {
  static sock_ops_t ops = {
      .connect = tcp_connect,
      .close = tcp_close,
      .send = tcp_send,
      .recv = tcp_recv,
      .setopt = tcp_setopt,
      .bind = tcp_bind,
      .listen = tcp_listen,
      .accept = tcp_accept,
      .destroy = tcp_destroy,
  };
  tcp_t *tcp = tcp_get_free(wait);
  if (!tcp) {
    dbg_error(DBG_TCP, "no tcp sock");
    return (tcp_t *)0;
  }

  plat_memset(tcp, 0, sizeof(tcp_t));

  net_err_t err = sock_init((sock_t *)tcp, family, protocol, &ops);
  if (err < 0) {
    dbg_error(DBG_TCP, "sock init failed.");
    mblock_free(&tcp_mblock, tcp);
    return (tcp_t *)0;
  }

  tcp->state = TCP_STATE_CLOSED;
  tcp->flags.keep_enable = 0;
  tcp->conn.keep_idle = TCP_KEEPALIVE_TIME;
  tcp->conn.keep_intvl = TCP_KEEPALIVE_INTVL;
  tcp->conn.keep_cnt = TCP_KEEPALIVE_PROBES;
  tcp->conn.keep_retry = 0;

  tcp->snd.ostate = TCP_OSTATE_IDLE;
  tcp->snd.rto = TCP_INIT_RTO;
  tcp->snd.remix_max = TCP_INIT_RETRIES;

  if (sock_wait_init(&tcp->conn.wait) < 0) {
    dbg_error(DBG_TCP, "create conn.wait failed.");
    goto alloc_failed;
  }
  tcp->base.conn_wait = &tcp->conn.wait;

  if (sock_wait_init(&tcp->snd.wait) < 0) {
    dbg_error(DBG_TCP, "create snd.wait failed.");
    goto alloc_failed;
  }
  tcp->base.snd_wait = &tcp->snd.wait;

  if (sock_wait_init(&tcp->rcv.wait) < 0) {
    dbg_error(DBG_TCP, "create rcv.wait failed.");
    goto alloc_failed;
  }
  tcp->base.rcv_wait = &tcp->rcv.wait;

  return tcp;

alloc_failed:
  if (tcp->base.conn_wait) {
    sock_wait_destroy(tcp->base.conn_wait);
  }
  if (tcp->base.snd_wait) {
    sock_wait_destroy(tcp->base.snd_wait);
  }
  if (tcp->base.rcv_wait) {
    sock_wait_destroy(tcp->base.rcv_wait);
  }
  mblock_free(&tcp_mblock, tcp);
  return (tcp_t *)0;
}

static void tcp_insert(tcp_t *tcp) {
  nlist_insert_last(&tcp_list, &tcp->base.node);
  dbg_assert(tcp_list.count <= TCP_MAX_NR, "tcp count err");
}

sock_t *tcp_create(int family, int protocol) {
  tcp_t *tcp = tcp_alloc(1, family, protocol);
  if (!tcp) {
    dbg_error(DBG_TCP, "alloc tcp failed.");
    return (sock_t *)0;
  }

  tcp_insert(tcp);
  return (sock_t *)tcp;
}

tcp_t *tcp_find(ipaddr_t *local_ip, uint16_t local_port, ipaddr_t *remote_ip,
                uint16_t remote_port) {
  tcp_t *match = (tcp_t *)0;
  nlist_node_t *node;

  nlist_for_each(node, &tcp_list) {
    sock_t *s = nlist_entry(node, sock_t, node);

    if (s->local_port == local_port &&
        ipaddr_is_equal(&s->remote_ip, remote_ip) &&
        s->remote_port == remote_port) {
      if (ipaddr_is_any(&s->local_ip) ||
          ipaddr_is_equal(&s->local_ip, local_ip)) {
        return (tcp_t *)s;
      }
    }

    tcp_t *tcp = (tcp_t *)s;
    if (tcp->state == TCP_STATE_LISTEN && s->local_port == local_port) {
      if (ipaddr_is_equal(&s->local_ip, local_ip)) {
        return tcp;
      } else if (ipaddr_is_any(&s->local_ip)) {
        match = tcp;
      }
    }
  }

  return match;
}

net_err_t tcp_abort(tcp_t *tcp, net_err_t err) {
  tcp_kill_all_timers(tcp);
  tcp_set_state(tcp, TCP_STATE_CLOSED);
  sock_wakeup(&tcp->base, SOCK_WAIT_ALL, err);
  return NET_ERR_OK;
}

int tcp_rcv_window(tcp_t *tcp) {
  int windows = tcp_buf_free_cout(&tcp->rcv.buf);
  return windows;
}

void tcp_kill_all_timers(tcp_t *tcp) {
  net_timer_remove(&tcp->conn.keep_timer);
  net_timer_remove(&tcp->snd.timer);
}

void tcp_alive_tmo(net_timer_t *timer, void *arg) {
  tcp_t *tcp = (tcp_t *)arg;

  if (++tcp->conn.keep_retry <= tcp->conn.keep_cnt) {
    net_timer_remove(&tcp->conn.keep_timer);
    net_timer_add(&tcp->conn.keep_timer, "keepalive", tcp_alive_tmo, tcp,
                  tcp->conn.keep_intvl * 1000, 0);
    tcp_send_keepalive(tcp);
    dbg_info(DBG_TCP, "tcp alive tmo, retry: %d", tcp->conn.keep_retry);
  } else {
    tcp_send_reset_for_tcp(tcp);
    tcp_abort(tcp, NET_ERR_CLOSE);
    dbg_error(DBG_TCP, "tcp alive tmo, give up");
  }
}

static void keepalive_start_timer(tcp_t *tcp) {
  net_timer_add(&tcp->conn.keep_timer, "keepalive", tcp_alive_tmo, tcp,
                tcp->conn.keep_idle * 1000, 0);
}

void tcp_keepalive_start(tcp_t *tcp, int run) {
  if (tcp->flags.keep_enable && !run) {
    net_timer_remove(&tcp->conn.keep_timer);
  } else if (run && !tcp->flags.keep_enable) {
    keepalive_start_timer(tcp);
  }
  tcp->flags.keep_enable = run;
}

void tcp_keepalive_restart(tcp_t *tcp) {
  if (tcp->flags.keep_enable) {
    net_timer_remove(&tcp->conn.keep_timer);
    keepalive_start_timer(tcp);
    tcp->conn.keep_retry = 0;
  }
}