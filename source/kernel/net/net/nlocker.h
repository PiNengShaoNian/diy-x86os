#ifndef NLOCKER_H
#define NLOCKER_H

#include "net_err.h"
#include "sys_plat.h"

typedef enum _nlocker_type_t {
  NLOCKER_NONE,
  NLOCKER_THREAD,
  NLOCKER_INT,
} nlocker_type_t;

typedef struct _nlocker_t {
  nlocker_type_t type;
  union {
    sys_mutex_t mutex;
#if NETIF_USE_INT == 1
    sys_intlocker_t state;  // 中断锁
#endif
  };
} nlocker_t;

net_err_t nlocker_init(nlocker_t *locker, nlocker_type_t type);
void nlocker_destroy(nlocker_t *locker);
void nlocker_lock(nlocker_t *locker);
void nlocker_unlock(nlocker_t *locker);

#endif