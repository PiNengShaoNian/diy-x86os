#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int unit32_t;

struct
{
    uint16_t limit_l, base_l, basel_attr, base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    [KERNEL_CODE_SEG / 8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    [KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf}};
