#include "tcp_in.h"

#include "dbg.h"
#include "protocol.h"
#include "tcp_out.h"
#include "tcp_state.h"
#include "tools.h"

void tcp_seg_init(tcp_seg_t *seg, pktbuf_t *buf, ipaddr_t *local,
                  ipaddr_t *remote) {
  seg->buf = buf;
  seg->hdr = (tcp_hdr_t *)pktbuf_data(buf);

  ipaddr_copy(&seg->local_ip, local);
  ipaddr_copy(&seg->remote_ip, remote);
  seg->data_len = buf->total_size - tcp_hdr_size(seg->hdr);
  seg->seq = seg->hdr->seq;
  seg->seq_len = seg->data_len + seg->hdr->f_syn + seg->hdr->f_fin;
}

static int tcp_seq_acceptable(tcp_t *tcp, tcp_seg_t *seg) {
  uint32_t rcv_win = tcp_rcv_window(tcp);
  if (seg->seq_len == 0) {
    if (rcv_win == 0) {
      return seg->seq == tcp->rcv.nxt;
    } else {
      int v = TCP_SEG_LE(tcp->rcv.nxt, seg->seq) &&
              TCP_SEG_LT(seg->seq, tcp->rcv.nxt + rcv_win);
      return v;
    }
  } else {
    if (rcv_win == 0) {
      return 0;
    } else {
      int v = TCP_SEG_LE(tcp->rcv.nxt, seg->seq) &&
              TCP_SEG_LT(seg->seq, tcp->rcv.nxt + rcv_win);
      uint32_t slast = seg->seq + seg->seq_len - 1;
      v |= TCP_SEG_LE(tcp->rcv.nxt, slast) &&
           TCP_SEG_LT(slast, tcp->rcv.nxt + rcv_win);
      return v;
    }
  }
}

net_err_t tcp_in(pktbuf_t *buf, ipaddr_t *src_ip, ipaddr_t *dest_ip) {
  static const tcp_proc_t tcp_state_proc[] = {
      [TCP_STATE_CLOSED] = tcp_closed_in,
      [TCP_STATE_LISTEN] = tcp_listen_in,
      [TCP_STATE_SYN_SENT] = tcp_syn_sent_in,
      [TCP_STATE_SYN_RECVD] = tcp_syn_recvd_in,
      [TCP_STATE_ESTABLISHED] = tcp_established_in,
      [TCP_STATE_FIN_WAIT_1] = tcp_fin_wait_1_in,
      [TCP_STATE_FIN_WAIT_2] = tcp_fin_wait_2_in,
      [TCP_STATE_CLOSING] = tcp_closing_in,
      [TCP_STATE_TIME_WAIT] = tcp_time_wait_in,
      [TCP_STATE_CLOSE_WAIT] = tcp_close_wait_in,
      [TCP_STATE_LAST_ACK] = tcp_last_ack_in,
  };

  if (pktbuf_set_cont(buf, sizeof(tcp_hdr_t)) < 0) {
    dbg_error(DBG_TCP, "set cond failed.");
    return NET_ERR_NONE;
  }

  tcp_hdr_t *tcp_hdr = (tcp_hdr_t *)pktbuf_data(buf);
  if (tcp_hdr->checksum) {
    pktbuf_reset_acc(buf);
    if (checksum_peso(buf, dest_ip, src_ip, NET_PROTOCOL_TCP)) {
      dbg_warning(DBG_TCP, "tcp check sum failed.");
      return NET_ERR_BROKEN;
    }
  }

  if (buf->total_size < sizeof(tcp_hdr_t) ||
      buf->total_size < tcp_hdr_size(tcp_hdr)) {
    dbg_warning(DBG_TCP, "tcp pkt size error");
    return NET_ERR_SIZE;
  }

  if (!tcp_hdr->sport || !tcp_hdr->dport) {
    dbg_warning(DBG_TCP, "port == 0");
    return NET_ERR_BROKEN;
  }

  if (tcp_hdr->flags == 0) {
    dbg_warning(DBG_TCP, "flags == 0");
    return NET_ERR_BROKEN;
  }

  tcp_hdr->sport = x_ntohs(tcp_hdr->sport);
  tcp_hdr->dport = x_ntohs(tcp_hdr->dport);
  tcp_hdr->seq = x_ntohl(tcp_hdr->seq);
  tcp_hdr->ack = x_ntohl(tcp_hdr->ack);
  tcp_hdr->win = x_ntohs(tcp_hdr->win);
  tcp_hdr->urgptr = x_ntohs(tcp_hdr->urgptr);

  tcp_show_pkt("tcp in", tcp_hdr, buf);

  tcp_seg_t seg;
  tcp_seg_init(&seg, buf, dest_ip, src_ip);

  tcp_t *tcp = tcp_find(dest_ip, tcp_hdr->dport, src_ip, tcp_hdr->sport);

  if (!tcp) {
    dbg_info(DBG_TCP, "no tcp find");
    tcp_closed_in((tcp_t *)0, &seg);
    pktbuf_free(buf);

    tcp_show_list();
    return NET_ERR_OK;
  }

  tcp_keepalive_restart(tcp);

  net_err_t err = pktbuf_seek(buf, tcp_hdr_size(tcp_hdr));
  if (err < 0) {
    dbg_error(DBG_TCP, "seek failed.");
    return NET_ERR_SIZE;
  }

  if (tcp->state != TCP_STATE_CLOSED && tcp->state != TCP_STATE_SYN_SENT &&
      tcp->state != TCP_STATE_LISTEN) {
    if (!tcp_seq_acceptable(tcp, &seg)) {
      dbg_info(DBG_TCP, "seq error");
      goto seg_drop;
    }
  }

  tcp_state_proc[tcp->state](tcp, &seg);
  tcp_show_info("after tcp in", tcp);

seg_drop:
  pktbuf_free(buf);
  return NET_ERR_OK;
}

static int copy_data_to_rcvbuf(tcp_t *tcp, tcp_seg_t *seg) {
  int doff = seg->seq - tcp->rcv.nxt;
  if (seg->data_len) {
    return tcp_buf_write_rcv(&tcp->rcv.buf, doff, seg->buf, seg->data_len);
  }
  return 0;
}

net_err_t tcp_data_in(tcp_t *tcp, tcp_seg_t *seg) {
  int size = copy_data_to_rcvbuf(tcp, seg);
  if (size < 0) {
    dbg_error(DBG_TCP, "copy data to rcvbuf failed.");
    return NET_ERR_SIZE;
  }

  int wakeup = 0;
  if (size) {
    tcp->rcv.nxt += size;
    wakeup++;
  }

  tcp_hdr_t *tcp_hdr = seg->hdr;
  if (tcp_hdr->f_fin && (tcp->rcv.nxt == seg->seq)) {
    tcp->flags.fin_in = 1;
    tcp->rcv.nxt++;
    wakeup++;
  }

  if (wakeup) {
    if (tcp->flags.fin_in) {
      sock_wakeup(&tcp->base, SOCK_WAIT_ALL, NET_ERR_CLOSE);
    } else {
      sock_wakeup(&tcp->base, SOCK_WAIT_READ, NET_ERR_OK);
    }

    tcp_send_ack(tcp, seg);
  }

  return NET_ERR_OK;
}