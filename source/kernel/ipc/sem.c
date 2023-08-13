#include "ipc/sem.h"
#include "tools/list.h"
#include "core/task.h"
#include "cpu/irq.h"
#include "os_cfg.h"

void sem_init(sem_t *sem, int init_count)
{
    sem->count = init_count;
    list_init(&sem->wait_list);
}

void sem_wait(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();

    if (sem->count > 0)
    {
        sem->count--;
    }
    else
    {
        task_t *curr = task_current();
        task_set_block(curr);
        list_insert_last(&sem->wait_list, &curr->wait_node);
        task_dispatch();
    }

    irq_leave_protection(state);
}

int sem_wait_tmo(sem_t *sem, int ms) {
    irq_state_t irq_state = irq_enter_protection();

    if(sem->count > 0) {
        sem->count--;
    }
    else {
        task_t *curr = task_current();
        task_set_block(curr);
        if(ms > 0) {
            task_set_sleep(curr, (ms + (OS_TICK_MS - 1))/ OS_TICK_MS);
        }

        curr->wait_list = &sem->wait_list;
        list_insert_last(&sem->wait_list, &curr->wait_node);

        task_dispatch();
        irq_leave_protection(irq_state);
        return curr->status;
    }

    irq_leave_protection(irq_state);
    return 0;
}

void sem_notify(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();

    if (list_count(&sem->wait_list))
    {
        list_node_t *node = list_remove_first(&sem->wait_list);
        task_t *task = list_node_parent(node, task_t, wait_node);
        
        // 如果进程同时还延时，先延时队列中移除
        if(task->sleep_ticks > 0)
        {
            task_set_wakeup(task);
        }

        task_set_ready(task);
        task->status = 0;
        task->wait_list = (list_t *)0;
        task->sleep_ticks = 0;
        task_dispatch();
    }
    else
    {
        sem->count++;
    }

    irq_leave_protection(state);
}

int sem_count(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();
    int count = sem->count;
    irq_leave_protection(state);

    return count;
}
