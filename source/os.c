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
    [KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf}};

void os_init(void)
{
    pg_dir[MAP_ADDR >> 22] = (unit32_t)page_table | PDE_P | PDE_W | PDE_U;
    page_table[(MAP_ADDR >> 12) & 0x3FF] = (unit32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}