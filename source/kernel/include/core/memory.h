#ifndef MEMORY_H
#define MEMORY_H

#include "comm/types.h"
#include "tools/bitmap.h"
#include "ipc/mutex.h"
#include "comm/boot_info.h"

#define MEM_EXT_START (1024 * 1024)
#define MEM_EXT_END (127 * 1024 * 1024)
#define MEM_PAGE_SIZE 4096
#define MEM_EBDA_START 0x80000
#define MEMORY_TASK_BASE 0x80000000

#define MEM_TASK_STACK_TOP 0xE0000000
#define MEM_TASK_STACK_SIZE (MEM_PAGE_SIZE * 500)
#define MEM_TASK_ARG_SIZE (MEM_PAGE_SIZE * 4)

typedef struct _addr_alloc_t
{
    mutex_t mutex;
    bitmap_t bitmap;
    uint32_t start;
    uint32_t size;
    uint32_t page_size;
} addr_alloc_t;

typedef struct _memory_map_t
{
    void *vstart;
    void *vend;
    void *pstart;
    uint32_t perm;
} memory_map_t;

void memory_init(boot_info_t *boot_info);

uint32_t memory_create_uvm(void);

int memory_alloc_page_for(uint32_t addr, uint32_t size, int perm);
int memory_alloc_for_page_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, int perm);

uint32_t memory_alloc_page(void);

void memory_free_page(uint32_t addr);

void memory_destroy_uvm(uint32_t page_dir);

uint32_t memory_copy_uvm(uint32_t page_dir);

uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr);

int memory_copy_uvm_data(uint32_t to,
                         uint32_t page_dir,
                         uint32_t from,
                         uint32_t size);

char *sys_sbrk(int incr);

#endif