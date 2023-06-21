#ifndef TASK_H
#define TASK_H

#include "comm/types.h"
#include "cpu/cpu.h"
#include "tools/list.h"

typedef struct _task_t
{
    // uint32_t *stack;
    tss_t tss;
    int tss_sel;
} task_t;

int task_init(task_t *task, uint32_t entry, uint32_t esp);

void task_switch_from_to(task_t *from, task_t *to);

typedef struct _task_manager_t
{
    task_t *curr_task;
    list_t ready_list;
    list_t task_list;

    task_t first_task;
} task_manager_t;

void task_manager_init(void);

void task_first_init(void);

task_t *task_first_task(void);

#endif