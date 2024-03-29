#ifndef NET_CFG_H
#define NET_CFG_H

#define DBG_MBLOCK DBG_LEVEL_ERROR
#define DBG_QUEUE DBG_LEVEL_ERROR
#define DBG_MSG DBG_LEVEL_ERROR
#define DBG_BUF DBG_LEVEL_ERROR
#define DBG_INIT DBG_LEVEL_ERROR
#define DBG_PLAT DBG_LEVEL_ERROR
#define DBG_NETIF DBG_LEVEL_ERROR
#define DBG_ETHER DBG_LEVEL_ERROR
#define DBG_TOOLS DBG_LEVEL_ERROR
#define DBG_TIMER DBG_LEVEL_ERROR
#define DBG_ARP DBG_LEVEL_ERROR
#define DBG_IP DBG_LEVEL_ERROR
#define DBG_ICMPv4 DBG_LEVEL_ERROR
#define DBG_SOCKET DBG_LEVEL_ERROR
#define DBG_RAW DBG_LEVEL_ERROR
#define DBG_UDP DBG_LEVEL_ERROR
#define DBG_TCP DBG_LEVEL_INFO
#define DBG_DNS DBG_LEVEL_INFO

#define NET_ENDIAN_LITTLE 1

#define EXMSG_MSG_CNT 10
#define EXMSG_LOCKER NLOCKER_THREAD

#define PKTBUF_BLK_SIZE 128
#define PKTBUF_BLK_CNT 100
#define PKTBUF_BUF_CNT 100

#define NETIF_USE_INT 1

#define NETIF_HWADDR_SIZE 10
#define NETIF_NAME_SIZE 10
#define NETIF_INQ_SIZE 50
#define NETIF_OUTQ_SIZE 50

#define NETIF_DEV_CNT 10

#define TIMER_NAME_SIZE 32

#define ARP_CACHE_SIZE 50
#define ARP_MAX_PKT_WAIT 5

#define ARP_TIMER_TMO 1
#define ARP_ENTRY_STABLE_TMO 20
#define ARP_ENTRY_PENDING_TMO 3
#define ARP_ENTRY_RETRY_CNT 5

#define IP_FRAGS_MAX_NR 5
#define IP_FRAG_MAX_BUF_NR 10
#define IP_FRAG_SCAN_PERIOD 1
#define IP_FRAG_TMO 10
#define IP_RTTABLE_SIZE 80

#define RAW_MAX_NR 10
#define RAW_MAX_RECV 50

#define UDP_MAX_NR 10
#define UDP_MAX_RECV 50

#define TCP_MAX_NR 10
#define TCP_SBUF_SIZE 4096
#define TCP_RBUF_SIZE 4096

#define TCP_KEEPALIVE_TIME (2 * 60 * 60)
#define TCP_KEEPALIVE_INTVL (5 * 1)
#define TCP_KEEPALIVE_PROBES 10

#define TCP_INIT_RTO 1000  // tcp initial timeout
#define TCP_INIT_RETRIES 5
#define TCP_RTO_MAX 8000

#define NET_CLOSE_MAX_TMO 5000
#define TCP_TMO_MSL 5000

#define DNS_DOMAIN_NAME_MAX 64
#define DNS_REQ_SIZE 10
#define DNS_WORKING_BUF_SIZE 512
#define DNS_REQ_TMO 5000
#define DNS_ENTRY_SIZE 10
#define DNS_UPDATE_PERIOD 1
#define DNS_QUERY_RETRY_TMO 5
#define DNS_QUERY_RETRY_CNT 5

#endif