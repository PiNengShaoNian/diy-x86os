#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int unit32_t;

// 页表项是否有效
#define PDE_P (1 << 0)
// 映射区域是否可写
#define PDE_W (1 << 1)
// 普通用户权限是否有权访问
#define PDE_U (1 << 2)
// 是否为4M模式
#define PDE_PS (1 << 7)

#define MAP_ADDR 0x80000000

unit32_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36};
static unit32_t page_table[1024] __attribute__((aligned(4096))) = {PDE_U};
unit32_t pg_dir[1024] __attribute__((aligned(4096))) = {
    [0] = 0 | PDE_P | PDE_W | PDE_U | PDE_PS,
};

struct
{
    uint16_t limit_l, base_l, basel_attr, base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    [KERNEL_CODE_SEG / 8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    [KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf},

    [APP_CODE_SEG / 8] = {0xffff, 0x0000, 0xfa00, 0x00cf},
    [APP_DATA_SEG / 8] = {0xffff, 0x0000, 0xf300, 0x00cf}};

unit32_t task0_dpl3_stack[1024] = {0};

struct
{
    uint16_t offset_l, selector, attr, offset_h;
} idt_table[256] __attribute__((aligned(8))) = {};

void outb(uint8_t data, uint16_t port)
{
    __asm__ __volatile__("outb %[v], %[p]" ::[p] "d"(port), [v] "a"(data));
}

void timer_int(void);

void os_init(void)
{
    outb(0x11, 0x20);
    outb(0x11, 0xA0);
    outb(0x20, 0x21);
    outb(0x28, 0xA1);
    outb(1 << 2, 0x21);
    outb(2, 0xA1);

    outb(0x1, 0x21);
    outb(0x1, 0xA1);
    outb(0xfe, 0x21);
    outb(0xff, 0xa1);

    // 配置中断频率
    int tmo = 1193180 / 10;
    outb(0x36, 0x43);
    outb((uint8_t)tmo, 0x40);
    outb(tmo >> 8, 0x40);

    idt_table[0x20].offset_l = (unit32_t)timer_int & 0xFFFF;
    idt_table[0x20].offset_h = (unit32_t)timer_int >> 16;
    idt_table[0x20].selector = KERNEL_CODE_SEG;
    idt_table[0x20].attr = 0x8E00;

    pg_dir[MAP_ADDR >> 22] = (unit32_t)page_table | PDE_P | PDE_W | PDE_U;
    page_table[(MAP_ADDR >> 12) & 0x3FF] = (unit32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}