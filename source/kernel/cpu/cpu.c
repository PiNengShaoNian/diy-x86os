#include "cpu/cpu.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];

void segment_desc_set(int selector, uint32_t base,
                      uint32_t limit, uint16_t attr)
{
    if (limit > 0xFFFFF)
    {
        attr |= 0x8000;
        limit /= 0x1000;
    }

    segment_desc_t *desc = gdt_table + (selector >> 3);
    desc->limit15_0 = limit & 0xFFFF;
    desc->base15_0 = base & 0xFFFF;
    desc->base23_16 = (base >> 16) & 0xFF;
    desc->attr = attr | (((limit >> 16) & 0xF) << 8);
    desc->base31_24 = (base >> 24) & 0xFF;
}

void init_gdt(void)
{
    for (int i = 0; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_set(i << 3, 0, 0, 0);
    }

    // 数据段
    segment_desc_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL |
                         SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D | SEG_G);

    // 只能用非一致代码段，以便通过调用门更改当前任务的CPL执行关键的资源访问操作
    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL |
                         SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D | SEG_G);

    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}

/**
 * CPU初始化
 */
void cpu_init(void)
{
    init_gdt();
}