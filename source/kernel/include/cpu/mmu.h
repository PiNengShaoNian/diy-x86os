#ifndef MMU_H
#define MMU_H

#include "comm/types.h"
#include "comm/cpu_instr.h"

#define PDE_CNT 1024
#define PTE_CNT 1024

#define PDE_P (1 << 0)
#define PTE_P (1 << 0)
#define PDE_W (1 << 1)
#define PTE_W (1 << 1)
#define PDE_U (1 << 2)
#define PTE_U (1 << 2)

typedef union _pde_t
{
    uint32_t v;
    struct
    {
        uint32_t present : 1;
        uint32_t write_enable : 1;
        uint32_t user_mode_acc : 1;
        uint32_t write_through : 1;
        uint32_t cache_disable : 1;
        uint32_t accessed : 1;
        uint32_t : 1;
        uint32_t ps : 1;
        uint32_t : 4;
        uint32_t phy_pt_addr : 20;
    };
} pde_t;

typedef union _pte_t
{
    uint32_t v;
    struct
    {
        uint32_t present : 1;
        uint32_t write_enable : 1;
        uint32_t user_mode_acc : 1;
        uint32_t write_through : 1;
        uint32_t cache_disable : 1;
        uint32_t accessed : 1;
        uint32_t dirty : 1;
        uint32_t pat : 1;
        uint32_t global : 1;
        uint32_t : 3;
        uint32_t phy_page_addr : 20;
    };
} pte_t;

static inline uint32_t pde_index(uint32_t vaddr)
{
    return (vaddr >> 22);
}

static inline uint32_t pte_index(uint32_t vaddr)
{
    return (vaddr >> 12) & 0x3FF;
}

static inline uint32_t pde_paddr(pde_t *pde)
{
    return pde->phy_pt_addr << 12;
}

static inline uint32_t pte_paddr(pte_t *pte)
{
    return pte->phy_page_addr << 12;
}

static inline void mmu_set_page_dir(uint32_t paddr)
{
    write_cr3(paddr);
}

static inline uint32_t get_pte_perm(pte_t *pte)
{
    return (pte->v & 0x3FF);
}

#endif