#ifndef DISK_H
#define DISK_H

#include "comm/types.h"

#define DISK_NAME_SIZE 32
#define PART_NAME_SIZE 32
#define DISK_PRIMARY_PART_CNT (4 + 1)
#define DISK_CNT 2
#define DISK_PER_CHANNEL 2

#define IOBASE_PRIMARY 0x1F0
#define DISK_DATA(disk) (disk->port_base + 0)
#define DISK_ERROR(disk) (disk->port_base + 1)
#define DISK_SECTOR_COUNT(disk) (disk->port_base + 2)
#define DISK_LBA_LO(disk) (disk->port_base + 3)
#define DISK_LBA_MID(disk) (disk->port_base + 4)
#define DISK_LBA_HI(disk) (disk->port_base + 5)
#define DISK_DRIVE(disk) (disk->port_base + 6)
#define DISK_STATUS(disk) (disk->port_base + 7)
#define DISK_CMD(disk) (disk->port_base + 7)

#define DISK_CMD_IDENTIFY 0xEC
#define DISK_CMD_READ 0x24
#define DISK_CMD_WRITE 0x34

// 状态寄存器
#define DISK_STATUS_ERR (1 << 0)  // 发生了错误
#define DISK_STATUS_DRQ (1 << 3)  // 准备好接受数据或者输出数据
#define DISK_STATUS_DF (1 << 5)   // 驱动错误
#define DISK_STATUS_BUSY (1 << 7) // 正忙

#define DISK_DRIVE_BASE 0xE0 // 驱动器号基础值:0xA0 + LBA

struct _disk_t;

typedef struct _partinfo_t
{
    char name[PART_NAME_SIZE];
    struct _disk_t *disk;

    enum
    {
        FS_INVALID = 0x00,
        FS_FAT16_0 = 0x6,
        FS_FAT16_1 = 0xE,
    } type;

    int start_sector;
    int total_sector;
} partinfo_t;

typedef struct _disk_t
{
    char name[DISK_NAME_SIZE];
    enum
    {
        DISK_MASTER = (0 << 4),
        DISK_SLAVE = (1 << 4),
    } drive;
    uint16_t port_base;
    int sector_size;
    int sector_count;
    partinfo_t partinfo[DISK_PRIMARY_PART_CNT];
} disk_t;

void disk_init(void);

#endif