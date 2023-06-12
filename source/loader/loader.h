#ifndef LOADER_H
#define LOADER_H

#include "comm/boot_info.h"
#include "comm/types.h"

typedef struct SMAP_entry
{
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry Type, 值为1表明为我们可用的RAM空间
    uint32_t ACPI; // extended, bit0=1时表明此条目应当被忽略
} __attribute__((packed)) SMAP_entry_t;

#endif // LOADER_H