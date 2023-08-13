#include "dns.h"

#include "dbg.h"
#include "mblock.h"
#include "net_api.h"
#include "net_cfg.h"
#include "nlist.h"
#include "socket.h"
#include "timer.h"
#include "tools.h"
#include "udp.h"

static net_timer_t entry_update_timer;
static dns_entry_t dns_entry_tbl[DNS_ENTRY_SIZE];
static nlist_t req_list;
static mblock_t req_mblock;
static dns_req_t dns_req_tbl[DNS_REQ_SIZE];

static char working_buf[DNS_WORKING_BUF_SIZE];
static udp_t *dns_udp;

#if DBG_DISP_ENABLED(DBG_DNS)
static void show_entry_list(void) {
  for (int i = 0; i < DNS_ENTRY_SIZE; i++) {
    dns_entry_t *entry = dns_entry_tbl + i;

    if (ipaddr_is_any(&entry->ipaddr)) {
      continue;
    }

    plat_printf("%s ttl(%d) ", entry->domain_name, entry->ttl);
    dbg_dump_ip(DBG_DNS, "ip: ", &entry->ipaddr);
  }
}

static void show_req_list(void) {
  nlist_node_t *node;

  nlist_for_each(node, &req_list) {
    dns_req_t *req = nlist_entry(node, dns_req_t, node);

    plat_printf("name(%s), id: %d\n", req->domain_name, req->query_id);
  }
}
#else
#define show_entry_list()
#define show_req_list()
#endif

static void dns_entry_free(dns_entry_t *entry);
static void dns_req_failed(dns_req_t *req, net_err_t err);
static net_err_t dns_send_query(dns_req_t *req);

static void dns_update_tmo(struct _net_timer_t *timer, void *arg) {
  for (int i = 0; i < DNS_ENTRY_SIZE; i++) {
    dns_entry_t *entry = dns_entry_tbl + i;
    if (ipaddr_is_any(&entry->ipaddr)) {
      continue;
    }

    if (!entry->ttl || (--entry->ttl == 0)) {
      dns_entry_free(entry);
      show_entry_list();
    }
  }

  nlist_node_t *curr;
  nlist_node_t *next;
  for (curr = nlist_first(&req_list); curr; curr = next) {
    next = nlist_node_next(curr);

    dns_req_t *req = nlist_entry(curr, dns_req_t, node);

    if (--req->retry_tmo == 0) {
      if (--req->retry_cnt == 0) {
        dns_req_failed(req, NET_ERR_TMO);
      } else {
        req->retry_tmo = DNS_QUERY_RETRY_TMO;
        dns_send_query(req);
      }
    }
  }
}

void dns_init(void) {
  dbg_info(DBG_DNS, "DNS init");

  plat_memset(dns_entry_tbl, 0, sizeof(dns_entry_tbl));

  nlist_init(&req_list);
  mblock_init(&req_mblock, dns_req_tbl, sizeof(dns_req_t), DNS_REQ_SIZE,
              NLOCKER_THREAD);

  net_timer_add(&entry_update_timer, "dns timer", dns_update_tmo, (void *)0,
                DNS_UPDATE_PERIOD * 1000, NET_TIMER_RELOAD);
  dns_udp = (udp_t *)udp_create(AF_INET, IPPROTO_UDP);

  dbg_assert(dns_udp != (udp_t *)0, "create dns socket failed.");

  dbg_info(DBG_DNS, "DNS init done");
}

dns_req_t *dns_alloc_req(void) { return mblock_alloc(&req_mblock, 0); }

void dns_free_req(dns_req_t *req) {
  if (req->wait_sem != SYS_SEM_INVALID) {
    sys_sem_free(req->wait_sem);
  }
  mblock_free(&req_mblock, req);
}

static void dns_req_add(dns_req_t *req) {
  static int id = 0;

  req->retry_tmo = DNS_QUERY_RETRY_TMO;
  req->retry_cnt = DNS_QUERY_RETRY_CNT;
  req->query_id = ++id;
  req->err = NET_ERR_OK;
  ipaddr_set_any(&req->ipaddr);

  nlist_insert_last(&req_list, &req->node);

  show_req_list();
}

static uint8_t *add_query_field(const char *domain_name, char *buf,
                                size_t size) {
  if (size < plat_strlen(domain_name) + 2 + 4) {
    dbg_error(DBG_DNS, "no enough space");
    return (uint8_t *)0;
  }

  char *name_buf = buf;
  name_buf[0] = '.';
  plat_strcpy(name_buf + 1, domain_name);

  char *c = name_buf;
  while (*c) {
    if (*c == '.') {
      char *dot = c++;

      while (*c && (*c != '.')) {
        c++;
      }
      *dot = (uint8_t)(c - dot - 1);
    } else {
      c++;
    }
  }

  *c++ = '\0';
  dns_qfield_t *f = (dns_qfield_t *)c;
  f->class = x_htons(DNS_QUERY_ClASS_INET);
  f->type = x_htons(DNS_QUERY_TYPE_A);
  return (uint8_t *)f + sizeof(dns_qfield_t);
}

static net_err_t dns_send_query(dns_req_t *req) {
  dns_hdr_t *dns_hdr = (dns_hdr_t *)working_buf;
  dns_hdr->id = x_htons(req->query_id);
  dns_hdr->flags.all = 0;
  dns_hdr->flags.qr = 0;
  dns_hdr->flags.rd = 1;
  dns_hdr->flags.all = x_htons(dns_hdr->flags.all);
  dns_hdr->qdcount = x_htons(1);
  dns_hdr->ancount = 0;
  dns_hdr->nscount = 0;
  dns_hdr->arcount = 0;

  uint8_t *buf = working_buf + sizeof(dns_hdr_t);
  buf = add_query_field(req->domain_name, buf,
                        sizeof(working_buf) - ((char *)buf - working_buf));

  if (!buf) {
    dbg_error(DBG_DNS, "add query failed.");
    return NET_ERR_MEM;
  }

  struct x_sockaddr_in dest;
  plat_memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = x_htons(DNS_PORT_DEFAULT);
  dest.sin_addr.s_addr = x_inet_addr("8.8.8.8");
  return udp_sendto((sock_t *)dns_udp, working_buf, (char *)buf - working_buf,
                    0, (const struct x_sockaddr *)&dest, sizeof(dest),
                    (ssize_t *)0);
}

static dns_entry_t *dns_entry_find(const char *domain_name) {
  for (int i = 0; i < DNS_ENTRY_SIZE; i++) {
    dns_entry_t *curr = dns_entry_tbl + i;
    if (!ipaddr_is_any(&curr->ipaddr)) {
      if (plat_stricmp(domain_name, curr->domain_name) == 0) {
        return curr;
      }
    }
  }

  return (dns_entry_t *)0;
}

static void dns_entry_init(dns_entry_t *entry, const char *domain_name, int ttl,
                           ipaddr_t *ipaddr) {
  ipaddr_copy(&entry->ipaddr, ipaddr);
  plat_strncpy(entry->domain_name, domain_name, DNS_DOMAIN_NAME_MAX - 1);
  entry->domain_name[DNS_DOMAIN_NAME_MAX - 1] = '\0';
  entry->ttl = ttl;
}

static void dns_entry_free(dns_entry_t *entry) {
  ipaddr_set_any(&entry->ipaddr);
}

static void dns_entry_insert(const char *domain_name, int ttl,
                             ipaddr_t *ipaddr) {
  dns_entry_t *free = (dns_entry_t *)0;
  dns_entry_t *oldest = (dns_entry_t *)0;

  for (int i = 0; i < DNS_ENTRY_SIZE; i++) {
    dns_entry_t *entry = dns_entry_tbl + i;

    if (ipaddr_is_any(&entry->ipaddr)) {
      free = entry;
      break;
    }

    if ((oldest == (dns_entry_t *)0) || (entry->ttl < oldest->ttl)) {
      oldest = entry;
    }
  }

  if (free == (dns_entry_t *)0) {
    free = oldest;
  }

  dns_entry_init(free, domain_name, ttl, ipaddr);
}

int dns_is_arrive(udp_t *udp) { return udp == dns_udp; }

static const char *domain_name_cmp(const char *domain_name, const char *name,
                                   size_t size) {
  const char *src = domain_name;
  const char *dest = name;

  while (*src) {
    int cnt = *dest++;
    for (int i = 0; i < cnt && *src && *dest; i++) {
      if (*dest++ != *src++) {
        return (const char *)0;
      }
    }

    if (*src == '\0') {
      break;
    } else if (*src++ != '.') {
      return (const char *)0;
    }
  }

  return (dest >= name + size) ? (const char *)0 : dest + 1;
}

static const uint8_t *domain_name_skip(const uint8_t *name, size_t size) {
  const uint8_t *c = name;
  const uint8_t *end = name + size;

  while (*c && (c < end)) {
    if (*c & 0xc0) {
      c += 2;
      return c;
    } else {
      c += *c;
    }
  }

  if (c < end) {
    c++;
  } else {
    return (const uint8_t *)0;
  }

  return c;
}

static void dns_req_remove(dns_req_t *req, net_err_t err) {
  nlist_remove(&req_list, &req->node);

  req->err = err;
  if (req->err < 0) {
    ipaddr_set_any(&req->ipaddr);
  }
  sys_sem_notify(req->wait_sem);
  sys_sem_free(req->wait_sem);
  req->wait_sem = SYS_SEM_INVALID;

  show_req_list();
}

static void dns_req_failed(dns_req_t *req, net_err_t err) {
  dns_req_remove(req, err);
}

void dns_in(void) {
  ssize_t rcv_len;
  struct x_sockaddr_in src;
  x_socklen_t addr_len;
  net_err_t err =
      udp_recvfrom((sock_t *)dns_udp, working_buf, sizeof(working_buf), 0,
                   (struct x_sockaddr *)&src, &addr_len, &rcv_len);
  if (err < 0) {
    dbg_error(DBG_DNS, "rcv udp error");
    return;
  }

  const uint8_t *rcv_start = working_buf;
  const uint8_t *rcv_end = working_buf + rcv_len;
  dns_hdr_t *dns_hdr = (dns_hdr_t *)working_buf;
  dns_hdr->id = x_ntohs(dns_hdr->id);
  dns_hdr->flags.all = x_ntohs(dns_hdr->flags.all);
  dns_hdr->qdcount = x_ntohs(dns_hdr->qdcount);
  dns_hdr->ancount = x_ntohs(dns_hdr->ancount);
  dns_hdr->nscount = x_ntohs(dns_hdr->nscount);
  dns_hdr->arcount = x_ntohs(dns_hdr->arcount);

  nlist_node_t *curr;
  nlist_for_each(curr, &req_list) {
    dns_req_t *req = nlist_entry(curr, dns_req_t, node);
    if (req->query_id != dns_hdr->id) {
      continue;
    }

    if (dns_hdr->flags.qr == 0) {
      dbg_warning(DBG_DNS, "not a response");
      goto req_failed;
    }

    if (dns_hdr->flags.tc == 1) {
      dbg_warning(DBG_DNS, "truncated");
      goto req_failed;
    }

    if (dns_hdr->flags.ra == 0) {
      dbg_warning(DBG_DNS, "recursion not support");
      goto req_failed;
    }

    switch (dns_hdr->flags.rcode) {
      // 以下应当更换服务器
      case DNS_ERR_NONE:
        // 没有错误
        break;
      case DNS_ERR_NOTIMP:
        dbg_warning(DBG_DNS, "server reply: not support");
        err = NET_ERR_NOT_SUPPORT;
        goto req_failed;
      case DNS_ERR_REFUSED:
        dbg_warning(DBG_DNS, "server reply: refused");
        err = NET_ERR_REFUSED;
        goto req_failed;
      case DNS_ERR_SERV_FAIL:
        dbg_warning(DBG_DNS, "server reply: server failure");
        err = NET_ERR_SERVER_FAILURE;
        goto req_failed;
      case DNS_ERR_NXMOMAIN:
        dbg_warning(DBG_DNS, "server reply: domain not exist");
        err = NET_ERR_NOT_EXIST;
        goto req_failed;
      // 以下直接删除请求
      case DNS_ERR_FORMAT:
        dbg_warning(DBG_DNS, "server reply: format error");
        err = NET_ERR_FORMAT;
        goto req_failed;
      default:
        dbg_warning(DBG_DNS, "unknow error");
        err = NET_ERR_UNKNOWN;
        goto req_failed;
    }

    if (dns_hdr->qdcount == 1) {
      rcv_start += sizeof(dns_hdr_t);
      rcv_start = domain_name_cmp(req->domain_name, (const char *)rcv_start,
                                  rcv_end - rcv_start);
      if (rcv_start == (uint8_t *)0) {
        dbg_warning(DBG_DNS, "domain name not match");
        err = NET_ERR_BROKEN;
        goto req_failed;
      }

      if (rcv_start + sizeof(dns_qfield_t) > rcv_end) {
        dbg_warning(DBG_DNS, "size error");
        err = NET_ERR_SIZE;
        goto req_failed;
      }

      dns_qfield_t *qf = (dns_qfield_t *)rcv_start;
      if (qf->class != x_ntohs(DNS_QUERY_ClASS_INET)) {
        dbg_warning(DBG_DNS, "query class not inet");
        err = NET_ERR_BROKEN;
        goto req_failed;
      }

      if (qf->type != x_ntohs(DNS_QUERY_TYPE_A)) {
        dbg_warning(DBG_DNS, "query type not A");
        err = NET_ERR_BROKEN;
        goto req_failed;
      }

      rcv_start += sizeof(dns_qfield_t);
    }

    if (dns_hdr->ancount < 1) {
      dbg_warning(DBG_DNS, "query answer == 0");
      err = NET_ERR_NONE;
      goto req_failed;
    }

    for (int i = 0; (i < dns_hdr->ancount) && (rcv_start < rcv_end); i++) {
      rcv_start = domain_name_skip(rcv_start, rcv_end - rcv_start);
      if (rcv_start == (uint8_t *)0) {
        dbg_warning(DBG_DNS, "size error");
        err = NET_ERR_BROKEN;
        goto req_failed;
      }

      if (rcv_start + sizeof(dns_afield_t) > rcv_end) {
        dbg_warning(DBG_DNS, "size error");
        err = NET_ERR_BROKEN;
        goto req_failed;
      }

      dns_afield_t *af = (dns_afield_t *)rcv_start;
      if ((af->class == x_ntohs(DNS_QUERY_ClASS_INET)) &&
          (af->type == x_ntohs(DNS_QUERY_TYPE_A)) &&
          (af->rd_len == x_ntohs(IPV4_ADDR_SIZE))) {
        ipaddr_from_buf(&req->ipaddr, (uint8_t *)af->rdata);
        dns_entry_insert(req->domain_name, x_ntohl(af->ttl), &req->ipaddr);
        dns_req_remove(req, NET_ERR_OK);
        return;
      }

      rcv_start += sizeof(dns_afield_t) + x_ntohs(af->rd_len) - 1;
    }

  req_failed:
    dns_req_failed(req, err);
    return;
  }
}

net_err_t dns_req_in(func_msg_t *msg) {
  dns_req_t *dns_req = (dns_req_t *)msg->param;

  if (plat_strcmp(dns_req->domain_name, "localhost") == 0) {
    ipaddr_from_str(&dns_req->ipaddr, "127.0.0.1");
    dns_req->err = NET_ERR_OK;
    return NET_ERR_OK;
  }

  ipaddr_t ipaddr;
  if (ipaddr_from_str(&ipaddr, dns_req->domain_name) == NET_ERR_OK) {
    ipaddr_copy(&dns_req->ipaddr, &ipaddr);
    dns_req->err = NET_ERR_OK;
    return NET_ERR_OK;
  }

  dns_entry_t *entry = dns_entry_find(dns_req->domain_name);
  if (entry) {
    ipaddr_copy(&dns_req->ipaddr, &entry->ipaddr);
    dns_req->err = NET_ERR_OK;
    return NET_ERR_OK;
  }

  dns_req->wait_sem = sys_sem_create(0);
  if (dns_req->wait_sem == SYS_SEM_INVALID) {
    dbg_error(DBG_DNS, "create sem failed.");
    return NET_ERR_SYS;
  }
  dns_req_add(dns_req);
  net_err_t err = dns_send_query(dns_req);
  if (err < 0) {
    dbg_error(DBG_DNS, "snd dns query failed.");
    goto dns_req_end;
  }

  return NET_ERR_OK;
dns_req_end:
  if (dns_req->wait_sem != SYS_SEM_INVALID) {
    sys_sem_free(dns_req->wait_sem);
    dns_req->wait_sem = SYS_SEM_INVALID;
  }
  return err;
}
