#include "nlocker.h"

#include "net_err.h"
#include "sys_plat.h"

net_err_t nlocker_init(nlocker_t *locker, nlocker_type_t type) {
  if (type == NLOCKER_THREAD) {
    sys_mutex_t mutex = sys_mutex_create();
    if (mutex == SYS_MUTEX_INVALID) {
      return NET_ERR_SYS;
    }

    locker->mutex = mutex;
  }

  locker->type = type;
  return NET_ERR_OK;
}

void nlocker_destroy(nlocker_t *locker) {
  if (locker->type == NLOCKER_THREAD) {
    sys_mutex_free(locker->mutex);
  }
}

void nlocker_lock(nlocker_t *locker) {
  if (locker->type == NLOCKER_THREAD) {
    sys_mutex_lock(locker->mutex);
  } else if (locker->type == NLOCKER_INT) {
#if NETIF_USE_INT
    locker->state = sys_intlocker_lock();
#endif
  }
}

void nlocker_unlock(nlocker_t *locker) {
  if (locker->type == NLOCKER_THREAD) {
    sys_mutex_unlock(locker->mutex);
  } else if (locker->type == NLOCKER_INT) {
#if NETIF_USE_INT
    sys_intlocker_unlock(locker->state);
#endif
  }
}
