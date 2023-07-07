#include "fs/fatfs/fatfs.h"
#include "fs/fs.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "comm/boot_info.h"
#include "tools/klib.h"
#include "core/memory.h"

int fatfs_mount(struct _fs_t *fs, int major, int minor)
{
    int dev_id = dev_open(major, minor, (void *)0);

    if (dev_id < 0)
    {
        log_printf("open disk failed. major: %x, minor: %x", major, minor);
        return -1;
    }

    dbr_t *dbr = (dbr_t *)memory_alloc_page();
    if (!dbr)
    {
        log_printf("mount failed: can't alloc buf");
        goto mount_failed;
    }

    int cnt = dev_read(dev_id, 0, (char *)dbr, 1);
    if (cnt < 1)
    {
        log_printf("read dbr failed");
        goto mount_failed;
    }

    fat_t *fat = &fs->fat_data;
    fat->fat_buffer = (uint8_t *)dbr;
    fat->bytes_per_sector = dbr->BPB_BytesPerSec;
    fat->tbl_start = dbr->BPB_RsvdSecCnt;
    fat->tbl_sectors = dbr->BPB_FATSz16;
    fat->tbl_cnt = dbr->BPB_NumFATs;
    fat->root_ent_cnt = dbr->BPB_RootEntCnt;
    fat->sec_per_cluster = dbr->BPB_SecPerClus;
    fat->cluster_byte_size = fat->sec_per_cluster * dbr->BPB_BytesPerSec;
    fat->root_start = fat->tbl_start + fat->tbl_sectors * fat->tbl_cnt;
    fat->data_start = fat->root_start + fat->root_ent_cnt * 32 / SECTOR_SIZE;
    fat->fs = fs;

    if (fat->tbl_cnt != 2)
    {
        log_printf("fat table error: major: %x, minor: %x", major, minor);
        goto mount_failed;
    }

    if (kernel_memcmp(dbr->BS_FileSysType, "FAT16", 5) != 0)
    {
        log_printf("not a fat filesystem, major: %x, minor: %x", major, minor);
        goto mount_failed;
    }

    fs->type = FS_FAT16;
    fs->data = &fs->fat_data;
    fs->dev_id = dev_id;

    return 0;

mount_failed:
    if (dbr)
        memory_free_page((uint32_t)dbr);

    dev_close(dev_id);
    return -1;
}

void fatfs_unmount(struct _fs_t *fs)
{
    fat_t *fat = &fs->fat_data;
    memory_free_page((uint32_t)fat->fat_buffer);
}

int fatfs_open(struct _fs_t *fs, const char *path, file_t *file)
{
    return -1;
}

int fatfs_read(char *buf, int size, file_t *file)
{
    return -1;
}

int fatfs_write(char *buf, int size, file_t *file)
{
    return -1;
}

void fatfs_close(file_t *file)
{
}

int fatfs_seek(file_t *file, uint32_t offset, int dir)
{
    return -1;
}

int fatfs_stat(file_t *file, struct stat *st)
{
    return -1;
}

int fatfs_opendir(struct _fs_t *fs, const char *name, DIR *dir)
{
    dir->index = 0;
    return 0;
}

int fatfs_readdir(struct _fs_t *fs, DIR *dir, struct dirent *dirent)
{
    if (dir->index++ < 10)
    {
        dirent->type = FILE_NORMAL;
        dirent->size = 1000;
        kernel_memcpy(dirent->name, "hello", 6);
        return 0;
    }

    return -1;
}

int fatfs_closedir(struct _fs_t *fs, DIR *dir)
{
    return 0;
}

fs_op_t fatfs_op = {
    .mount = fatfs_mount,
    .unmount = fatfs_unmount,
    .open = fatfs_open,
    .read = fatfs_read,
    .write = fatfs_write,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .stat = fatfs_stat,

    .opendir = fatfs_opendir,
    .readdir = fatfs_readdir,
    .closedir = fatfs_closedir,
};