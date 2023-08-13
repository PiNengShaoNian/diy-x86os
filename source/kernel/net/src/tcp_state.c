#include "tcp_state.h"

#include "tcp_in.h"
#include "tcp_out.h"
#include "tools.h"

const char *tcp_state_name(tcp_state_t state) {
  static const char *state_name[] = {
      [TCP_STATE_CLOSED] = "CLOSED",
      [TCP_STATE_LISTEN] = "LISTEN",
      [TCP_STATE_SYN_SENT] = "SYN_SENT",
      [TCP_STATE_SYN_RECVD] = "SYN_RECVD",
      [TCP_STATE_ESTABLISHED] = "ESTABLISHED",
      [TCP_STATE_FIN_WAIT_1] = "FIN_WAIT_1",
      [TCP_STATE_FIN_WAIT_2] = "FIN_WAIT_2",
      [TCP_STATE_CLOSING] = "CLOSING",
      [TCP_STATE_TIME_WAIT] = "TIME_WAIT",
      [TCP_STATE_CLOSE_WAIT] = "CLOSE_WAIT",
      [TCP_STATE_LAST_ACK] = "LAST_ACK",

      [TCP_STATE_MAX] = "UNKNOWN",
  };

  if (state > TCP_STATE_MAX) {
    state = TCP_STATE_MAX;
  }

  return state_name[state];
}

void tcp_set_state(tcp_t *tcp, tcp_state_t state) {
  tcp->state = state;
  tcp_show_info("tcp set state", tcp);
}

net_err_t tcp_closed_in(tcp_t *tcp, tcp_seg_t *seg) {
  if (!seg->hdr->f_rst) {
    tcp_send_reset(seg);
  }

  return NET_ERR_OK;
}

net_err_t tcp_listen_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    return NET_ERR_OK;
  }

  if (tcp_hdr->f_ack) {
    tcp_send_reset(seg);
    return NET_ERR_OK;
  }

  if (tcp_hdr->f_syn) {
    if (tcp_backlog_count(tcp) >= tcp->conn.backlog) {
      dbg_warning(DBG_TCP, "queue full");
      return NET_ERR_FULL;
    }

    tcp_t *child = tcp_create_child(tcp, seg);
    if (child == (tcp_t *)0) {
      dbg_error(DBG_TCP, "no tcp");
      return NET_ERR_MEM;
    }

    tcp_send_syn(child);
    tcp_set_state(child, TCP_STATE_SYN_RECVD);

    return NET_ERR_OK;
  }

  return NET_ERR_UNKNOWN;
}

void tcp_read_options(tcp_t *tcp, tcp_hdr_t *tcp_hdr) {
  uint8_t *opt_start = (uint8_t *)tcp_hdr + sizeof(tcp_hdr_t);
  uint8_t *opt_end = opt_start + (tcp_hdr_size(tcp_hdr) - sizeof(tcp_hdr_t));

  if (opt_end <= opt_start) {
    return;
  }

  while (opt_start < opt_end) {
    switch (opt_start[0]) {
      case TCP_OPT_MSS: {
        tcp_opt_mss_t *opt = (tcp_opt_mss_t *)opt_start;
        if (opt->length == 4) {
          uint16_t mss = x_ntohs(opt->mss);
          if (mss < tcp->mss) {
            tcp->mss = mss;
          }
          opt_start += opt->length;
        } else {
          opt_start++;
        }
        break;
        case TCP_OPT_NOP:
          opt_start++;
          break;
        case TCP_OPT_END:
          return;
        default:
          opt_start++;
          break;
      }
    }
  }
}

net_err_t tcp_syn_sent_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_ack) {
    if ((tcp_hdr->ack - tcp->snd.iss <= 0) ||
        (tcp_hdr->ack - tcp->snd.nxt > 0)) {
      dbg_warning(DBG_TCP, "%s: ack error", tcp_state_name(tcp->state));
      return tcp_send_reset(seg);
    }
  }

  if (tcp_hdr->f_rst) {
    if (!tcp_hdr->f_ack) {
      return NET_ERR_OK;
    }

    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    tcp->rcv.iss = tcp_hdr->seq;
    tcp->rcv.nxt = tcp_hdr->seq + 1;
    tcp->flags.irs_valid = 1;

    tcp_read_options(tcp, tcp_hdr);
    if (tcp_hdr->f_ack) {
      tcp_ack_process(tcp, seg);
      tcp_set_ostate(tcp, TCP_OSTATE_IDLE);
    }

    if (tcp->snd.una - tcp->snd.iss > 0) {
      tcp_send_ack(tcp, seg);
      tcp_set_state(tcp, TCP_STATE_ESTABLISHED);
      sock_wakeup(&tcp->base, SOCK_WAIT_CONN, NET_ERR_OK);
    } else {
      tcp_set_state(tcp, TCP_STATE_SYN_RECVD);
      tcp_send_syn(tcp);
    }
  }

  return NET_ERR_OK;
}

net_err_t tcp_syn_recvd_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  if (tcp_hdr->f_fin) {
    dbg_warning(DBG_TCP, "rcv a fin");
    return NET_ERR_UNREACH;
  }

  tcp_set_state(tcp, TCP_STATE_ESTABLISHED);
  if (tcp->parent) {
    sock_wakeup((sock_t *)tcp->parent, SOCK_WAIT_CONN, NET_ERR_OK);
  }

  return NET_ERR_OK;
}

net_err_t tcp_established_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  tcp_data_in(tcp, seg);

  tcp_out_event(tcp, TCP_OEVENT_SEND);

  if (tcp->flags.fin_in) {
    tcp_set_state(tcp, TCP_STATE_CLOSE_WAIT);
  }

  return NET_ERR_OK;
}

static void tcp_timewait_tmo(struct _net_timer_t *timer, void *arg) {
  tcp_t *tcp = (tcp_t *)arg;

  dbg_info(DBG_TCP, "tcp free: 2MSL");
  tcp_show_info("tcp free", tcp);
  tcp_free(tcp);
}

void tcp_time_wait(tcp_t *tcp) {
  tcp_set_state(tcp, TCP_STATE_TIME_WAIT);

  tcp_kill_all_timers(tcp);
  net_timer_add(&tcp->conn.keep_timer, "2msl timer", tcp_timewait_tmo, tcp,
                2 * TCP_TMO_MSL, 0);
  sock_wakeup(&tcp->base, SOCK_WAIT_ALL, NET_ERR_CLOSE);
}

net_err_t tcp_fin_wait_1_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  tcp_data_in(tcp, seg);

  tcp_out_event(tcp, TCP_OEVENT_SEND);

  if (tcp->flags.fin_out == 0) {
    if (tcp->flags.fin_in) {
      tcp_time_wait(tcp);
    } else {
      tcp_set_state(tcp, TCP_STATE_FIN_WAIT_2);
    }
  } else if (tcp->flags.fin_in) {
    tcp_set_state(tcp, TCP_STATE_CLOSING);
  }

  return NET_ERR_OK;
}

net_err_t tcp_fin_wait_2_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  tcp_data_in(tcp, seg);

  if (tcp->flags.fin_in) {
    tcp_time_wait(tcp);
  }

  return NET_ERR_OK;
}

net_err_t tcp_closing_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  tcp_out_event(tcp, TCP_OEVENT_SEND);

  if (tcp->flags.fin_out == 0) {
    tcp_time_wait(tcp);
  }

  return NET_ERR_OK;
}

net_err_t tcp_time_wait_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  if (tcp->flags.fin_in) {
    tcp_send_ack(tcp, seg);
    tcp_time_wait(tcp);
  }

  return NET_ERR_OK;
}

net_err_t tcp_close_wait_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  tcp_out_event(tcp, TCP_OEVENT_SEND);

  return NET_ERR_OK;
}

net_err_t tcp_last_ack_in(tcp_t *tcp, tcp_seg_t *seg) {
  tcp_hdr_t *tcp_hdr = seg->hdr;

  if (tcp_hdr->f_rst) {
    dbg_warning(DBG_TCP, "recv a rst");
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_hdr->f_syn) {
    dbg_warning(DBG_TCP, "recv a syn");
    tcp_send_reset(seg);
    return tcp_abort(tcp, NET_ERR_RESET);
  }

  if (tcp_ack_process(tcp, seg) < 0) {
    dbg_warning(DBG_TCP, "ack process failed.");
    return NET_ERR_UNREACH;
  }

  tcp_out_event(tcp, TCP_OEVENT_SEND);
  if (tcp->flags.fin_out == 0) {
    return tcp_abort(tcp, NET_ERR_CLOSE);
  }

  return NET_ERR_OK;
}