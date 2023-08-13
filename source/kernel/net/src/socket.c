#include "socket.h"

#include "dbg.h"
#include "dns.h"
#include "exmsg.h"
#include "sock.h"
#include "sys.h"

int x_socket(int family, int type, int protocol) {
  sock_req_t req;

  req.wait = (sock_wait_t*)0;
  req.wait_tmo = 0;
  req.sockfd = -1;
  req.create.family = family;
  req.create.protocol = protocol;
  req.create.type = type;

  net_err_t err = exmsg_func_exec(sock_create_req_in, &req);

  if (err < 0) {
    dbg_error(DBG_SOCKET, "create socket failed.");
    return -1;
  }

  return req.sockfd;
}

ssize_t x_sendto(int s, const void* buf, size_t len, int flags,
                 const struct x_sockaddr* dest, x_socklen_t dest_len) {
  if (!buf || !len) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  if (dest->sin_family != AF_INET ||
      (dest_len != sizeof(struct x_sockaddr_in))) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  ssize_t send_size = 0;
  uint8_t* start = (uint8_t*)buf;
  while (len > 0) {
    sock_req_t req;
    req.wait = (sock_wait_t*)0;
    req.wait_tmo = 0;
    req.sockfd = s;
    req.data.buf = start;
    req.data.len = len;
    req.data.flags = 0;
    req.data.addr = (struct x_sockaddr*)dest;
    req.data.addr_len = &dest_len;
    req.data.comp_len = 0;

    net_err_t err = exmsg_func_exec(sock_sendto_req_in, &req);
    if (err < 0) {
      dbg_error(DBG_SOCKET, "create socket failed.");
      return -1;
    }

    if (req.wait && (err = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
      dbg_error(DBG_SOCKET, "send failed.");
      return err;
    }

    len -= req.data.comp_len;
    start += req.data.comp_len;
    send_size += (ssize_t)req.data.comp_len;
  }

  return send_size;
}

ssize_t x_send(int s, const void* buf, size_t len, int flags) {
  if (!buf || !len) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  ssize_t send_size = 0;
  uint8_t* start = (uint8_t*)buf;
  while (len > 0) {
    sock_req_t req;
    req.wait = (sock_wait_t*)0;
    req.wait_tmo = 0;
    req.sockfd = s;
    req.data.buf = start;
    req.data.len = len;
    req.data.flags = 0;
    req.data.comp_len = 0;

    net_err_t err = exmsg_func_exec(sock_send_req_in, &req);
    if (err < 0) {
      dbg_error(DBG_SOCKET, "create socket failed.");
      return -1;
    }

    if (req.wait && (err = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
      dbg_error(DBG_SOCKET, "send failed.");
      return err;
    }

    len -= req.data.comp_len;
    start += req.data.comp_len;
    send_size += (ssize_t)req.data.comp_len;
  }

  return send_size;
}

ssize_t x_recvfrom(int s, const void* buf, size_t len, int flags,
                   const struct x_sockaddr* src, x_socklen_t* slen) {
  if (!buf || !len || !src) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  while (1) {
    sock_req_t req;
    req.wait = (sock_wait_t*)0;
    req.wait_tmo = 0;
    req.sockfd = s;
    req.data.buf = (uint8_t*)buf;
    req.data.len = len;
    req.data.flags = 0;
    req.data.addr = (struct x_sockaddr*)src;
    req.data.addr_len = slen;
    req.data.comp_len = 0;

    net_err_t err = exmsg_func_exec(sock_recvfrom_req_in, &req);
    if (err < 0) {
      dbg_error(DBG_SOCKET, "recv socket failed.");
      return -1;
    }

    if (req.data.comp_len) {
      return (ssize_t)req.data.comp_len;
    }

    if (req.wait) {
      err = sock_wait_enter(req.wait, req.wait_tmo);
    }
    if (err == NET_ERR_CLOSE) {
      dbg_info(DBG_SOCKET, "remote close");
      return 0;
    } else if (err < 0) {
      dbg_error(DBG_SOCKET, "recv failed.");
      return -1;
    }
  }

  return -1;
}

ssize_t x_recv(int s, const void* buf, size_t len, int flags) {
  if (!buf || !len) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  while (1) {
    sock_req_t req;
    req.wait = (sock_wait_t*)0;
    req.wait_tmo = 0;
    req.sockfd = s;
    req.data.buf = (uint8_t*)buf;
    req.data.len = len;
    req.data.flags = 0;
    req.data.comp_len = 0;

    net_err_t err = exmsg_func_exec(sock_recv_req_in, &req);
    if (err < 0) {
      dbg_error(DBG_SOCKET, "recv socket failed.");
      return -1;
    }

    if (req.data.comp_len) {
      return (ssize_t)req.data.comp_len;
    }

    if (req.wait) {
      err = sock_wait_enter(req.wait, req.wait_tmo);
    }

    if (err == NET_ERR_CLOSE) {
      dbg_info(DBG_SOCKET, "connection close");
      return 0;
    } else if (err < 0) {
      dbg_error(DBG_SOCKET, "recv failed.");
      return -1;
    }
  }

  return -1;
}

int x_setsockopt(int s, int level, int optname, const char* optval, int len) {
  if (!optval || !len) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  sock_req_t req;
  req.wait = (sock_wait_t*)0;
  req.wait_tmo = 0;
  req.sockfd = s;
  req.opt.level = level;
  req.opt.optname = optname;
  req.opt.optval = optval;
  req.opt.len = len;

  net_err_t err = exmsg_func_exec(sock_setsockopt_req_in, &req);
  if (err < 0) {
    dbg_error(DBG_SOCKET, "setopt failed.");
    return -1;
  }

  return 0;
}

int x_close(int s) {
  sock_req_t req;

  req.wait = (sock_wait_t*)0;
  req.wait_tmo = 0;
  req.sockfd = s;

  net_err_t err = exmsg_func_exec(sock_close_req_in, &req);
  if (err < 0) {
    dbg_error(DBG_SOCKET, "close failed.");
    exmsg_func_exec(sock_destroy_req_in, &req);
    return -1;
  }

  if (req.wait) {
    sock_wait_enter(req.wait, req.wait_tmo);
    exmsg_func_exec(sock_destroy_req_in, &req);
  }

  return 0;
}

int x_connect(int s, const struct x_sockaddr* addr, x_socklen_t addr_len) {
  if (!addr || addr_len != sizeof(struct x_sockaddr) || s < 0) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  if (addr->sin_family != AF_INET) {
    dbg_error(DBG_SOCKET, "family error");
    return -1;
  }

  sock_req_t req;
  req.wait = 0;
  req.sockfd = s;
  req.conn.addr = (struct x_sockaddr*)addr;
  req.conn.addr_len = addr_len;
  net_err_t err = exmsg_func_exec(sock_conn_req_in, &req);
  if (err < 0) {
    dbg_error(DBG_SOCKET, "conn failed.");
    return -1;
  }

  if (req.wait && (err = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
    dbg_error(DBG_SOCKET, "conn failed.");
    return -1;
  }

  return 0;
}

int x_bind(int s, const struct x_sockaddr* addr, x_socklen_t addr_len) {
  if (!addr || addr_len != sizeof(struct x_sockaddr) || s < 0) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  if (addr->sin_family != AF_INET) {
    dbg_error(DBG_SOCKET, "family error");
    return -1;
  }

  sock_req_t req;
  req.wait = 0;
  req.sockfd = s;
  req.bind.addr = (struct x_sockaddr*)addr;
  req.bind.addr_len = addr_len;
  net_err_t err = exmsg_func_exec(sock_bind_req_in, &req);
  if (err < 0) {
    dbg_error(DBG_SOCKET, "bind failed.");
    return -1;
  }

  return 0;
}

int x_listen(int s, int backlog) {
  if (backlog <= 0) {
    dbg_error(DBG_SOCKET, "backlog <= 0");
    return NET_ERR_PARAM;
  }

  sock_req_t req;
  req.wait = 0;
  req.sockfd = s;
  req.listen.backlog = backlog;
  net_err_t err = exmsg_func_exec(sock_listen_req_in, &req);

  if (err < 0) {
    dbg_error(DBG_SOCKET, "listen error");
    return -1;
  }

  return 0;
}

int x_accept(int s, struct x_sockaddr* addr, x_socklen_t* len) {
  if (!addr || !len) {
    dbg_error(DBG_SOCKET, "param error");
    return -1;
  }

  while (1) {
    sock_req_t req;
    req.wait = 0;
    req.sockfd = s;
    req.accept.addr = addr;
    req.accept.len = len;
    req.accept.client = -1;

    net_err_t err = exmsg_func_exec(sock_accept_req_in, &req);
    if (err < 0) {
      dbg_error(DBG_SOCKET, "accept failed.");
      return -1;
    }

    if (req.accept.client >= 0) {
      dbg_info(DBG_TCP, "get new connection");
      return req.accept.client;
    }

    if (req.wait && (err = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
      dbg_error(DBG_SOCKET, "accept failed.");
      return -1;
    }
  }
}

int x_gethostbyname_r(const char* name, struct x_hostent* ret, char* buf,
                      size_t len, struct x_hostent** result, int* err) {
  int internal_err;
  if (!err) {
    err = &internal_err;
  }

  if (!name || !ret || !buf || !len || !result) {
    dbg_error(DBG_SOCKET, "param error");
    *err = NET_ERR_PARAM;
    return -1;
  }

  size_t name_len = plat_strlen(name);
  if (len < sizeof(hostent_extra_t) + name_len) {
    dbg_error(DBG_SOCKET, "buf too small");
    *err = NET_ERR_PARAM;
    return -1;
  }

  dns_req_t* dns_req = dns_alloc_req();
  plat_strncpy(dns_req->domain_name, name, DNS_DOMAIN_NAME_MAX);
  ipaddr_set_any(&dns_req->ipaddr);
  dns_req->err = NET_ERR_OK;
  dns_req->wait_sem = SYS_SEM_INVALID;

  net_err_t e = exmsg_func_exec(dns_req_in, dns_req);
  if (e < 0) {
    dbg_error(DBG_SOCKET, "get host ip failed.");
    *err = e;
    goto dns_req_err;
  }

  if ((dns_req->wait_sem != SYS_SEM_INVALID) &&
      (sys_sem_wait(dns_req->wait_sem, 0) < 0)) {
    dbg_error(DBG_SOCKET, "wait sem failed.");
    *err = NET_ERR_TMO;
    goto dns_req_err;
  }

  if (dns_req->err < 0) {
    dbg_error(DBG_SOCKET, "dns resolve failed.");
    *err = dns_req->err;
    goto dns_req_err;
  }

  hostent_extra_t* extra = (hostent_extra_t*)buf;
  extra->addr = dns_req->ipaddr.q_addr;

  plat_strncpy(extra->name, name, name_len);
  ret->h_name = extra->name;
  ret->h_aliases = (char**)0;
  ret->h_addrtype = AF_INET;
  ret->h_length = 4;
  ret->h_addr_list = (char**)extra->addr_tbl;
  ret->h_addr_list[0] = (char*)&extra->addr;
  ret->h_addr_list[1] = (char*)0;

  *result = ret;
  *err = NET_ERR_OK;
  dns_free_req(dns_req);
  return 0;

dns_req_err:
  dns_free_req(dns_req);
  return -1;
}