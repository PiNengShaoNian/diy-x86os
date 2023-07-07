#ifndef FATFS_H
#define FATFS_H

#include "comm/types.h"

#pragma pack(1)

/**
 * 完整的DBR类型
 */
typedef struct _dbr_t
{
    uint8_t BS_jmpBoot[3];    // 跳转代码
    uint8_t BS_OEMName[8];    // OEM名称
    uint16_t BPB_BytesPerSec; // 每扇区字节数
    uint8_t BPB_SecPerClus;   // 每簇扇区数
    uint16_t BPB_RsvdSecCnt;  // 保留区扇区数
    uint8_t BPB_NumFATs;      // FAT表项数
    uint16_t BPB_RootEntCnt;  // 根目录项目数
    uint16_t BPB_TotSec16;    // 总的扇区数
    uint8_t BPB_Media;        // 媒体类型
    uint16_t BPB_FATSz16;     // FAT表项大小
    uint16_t BPB_SecPerTrk;   // 每磁道扇区数
    uint16_t BPB_NumHeads;    // 磁头数
    uint32_t BPB_HiddSec;     // 隐藏扇区数
    uint32_t BPB_TotSec32;    // 总的扇区数

    uint8_t BS_DrvNum;         // 磁盘驱动器参数
    uint8_t BS_Reserved1;      // 保留字节
    uint8_t BS_BootSig;        // 扩展引导标记
    uint32_t BS_VolID;         // 卷标序号
    uint8_t BS_VolLab[11];     // 磁盘卷标
    uint8_t BS_FileSysType[8]; // 文件类型名称
} dbr_t;

#pragma pack()

typedef struct _fat_t
{
    uint32_t tbl_start;
    uint32_t tbl_cnt;
    uint32_t tbl_sectors;
    uint32_t bytes_per_sector;
    uint32_t sec_per_cluster;
    uint32_t root_start;
    uint32_t root_ent_cnt;
    uint32_t data_start;
    uint32_t cluster_byte_size;

    uint8_t *fat_buffer;

    struct _fs_t *fs;
} fat_t;

#endif