#include "fs/fatfs/fatfs.h"
#include "fs/fs.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "comm/boot_info.h"
#include "tools/klib.h"
#include "core/memory.h"

static int bread_sector(fat_t *fat, int sector)
{
    if (sector == fat->curr_sector)
        return 0;

    int cnt = dev_read(fat->fs->dev_id, sector, fat->fat_buffer, 1);

    if (cnt == 1)
    {
        fat->curr_sector = sector;
        return 0;
    }

    return -1;
}

void diritem_get_name(diritem_t *item, char *dest)
{
    char *c = dest;
    char *ext = (char *)0;
    kernel_memset(dest, 0, 12);

    for (int i = 0; i < 11; i++)
    {
        if (item->DIR_Name[i] != ' ')
            *c++ = item->DIR_Name[i];

        if (i == 7)
        {
            ext = c;
            *c++ = '.';
        }
    }

    if (ext && ext[1] == '\0')
        ext[0] = '\0';
}

static void to_sfn(char *dest, const char *src)
{
    kernel_memset(dest, ' ', 11);

    char *curr = dest;
    char *end = dest + 11;
    while (*src && curr < end)
    {
        char c = *src++;
        switch (c)
        {
        case '.':
            curr = dest + 8;
            break;
        default:
            if ((c >= 'a') && (c <= 'z'))
                c = c - 'a' + 'A';

            *curr++ = c;
            break;
        }
    }
}

int diritem_name_match(diritem_t *item, const char *path)
{
    char buf[11];
    to_sfn(buf, path);
    return kernel_memcmp(buf, item->DIR_Name, 11) == 0;
}

static diritem_t *read_dir_entry(fat_t *fat, int index)
{
    if (index < 0 || (index >= fat->root_ent_cnt))
        return (diritem_t *)0;

    int offset = index * sizeof(diritem_t);
    int sector = fat->root_start + offset / fat->bytes_per_sector;
    int err = bread_sector(fat, sector);
    if (err < 0)
        return (diritem_t *)0;

    return (diritem_t *)(fat->fat_buffer + offset % fat->bytes_per_sector);
}

file_type_t diritem_get_type(diritem_t *diritem)
{
    if (diritem->DIR_Attr & (DIRITEM_ATTR_VOLUME_ID |
                             DIRITEM_ATTR_HIDDEN | DIRITEM_ATTR_SYSTEM))
        return FILE_UNKOWN;

    if ((diritem->DIR_Attr & DIRITEM_ATTR_LONG_NAME) == DIRITEM_ATTR_LONG_NAME)
        return FILE_UNKOWN;

    return (diritem->DIR_Attr & DIRITEM_ATTR_DIRECTORY) ? FILE_DIR : FILE_NORMAL;
}

static void read_from_diritem(fat_t *fat, file_t *file, diritem_t *item, int index)
{
    file->type = diritem_get_type(item);
    file->size = item->DIR_FileSize;
    file->pos = 0;
    file->p_index = 0;
    file->sblk = (item->DIR_FstClusHI << 16) | (item->DIR_FstClusL0);
    file->cblk = file->sblk;
}

int cluster_is_valid(cluster_t cluster)
{
    return cluster < FAT_CLUSTER_INVALID && cluster >= 2;
}

int cluster_get_next(fat_t *fat, cluster_t curr)
{
    if (!cluster_is_valid(curr))
        return FAT_CLUSTER_INVALID;

    int offset = curr * sizeof(cluster_t);
    int sector = offset / fat->bytes_per_sector;
    int off_sector = offset % fat->bytes_per_sector;

    if (sector >= fat->tbl_sectors)
    {
        log_printf("cluster too big");
        return FAT_CLUSTER_INVALID;
    }

    int err = bread_sector(fat, fat->tbl_start + sector);
    if (err < 0)
        return FAT_CLUSTER_INVALID;

    return *(cluster_t *)(fat->fat_buffer + off_sector);
}

static int move_file_pos(file_t *file, fat_t *fat, uint32_t move_bytes, int expand)
{
    uint32_t c_offset = file->pos % fat->cluster_byte_size;
    if (c_offset + move_bytes >= fat->cluster_byte_size)
    {
        cluster_t next = cluster_get_next(fat, file->cblk);

        if (next == FAT_CLUSTER_INVALID)
            return -1;

        file->cblk = next;
    }

    file->pos += move_bytes;
    return 0;
}

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
    fat->curr_sector = -1;
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
    fat_t *fat = (fat_t *)fs->data;
    diritem_t *file_item = (diritem_t *)0;
    int p_index = -1;

    for (int i = 0; i < fat->root_ent_cnt; i++)
    {
        diritem_t *item = read_dir_entry(fat, i);
        if (item == (diritem_t *)0)
            return -1;

        if (item->DIR_Name[0] == DIRITEM_NAME_END)
            break;

        if (item->DIR_Name[0] == DIRITEM_NAME_FREE)
            continue;

        if (diritem_name_match(item, path))
        {
            file_item = item;
            p_index = i;
            break;
        }
    }

    if (file_item)
    {
        read_from_diritem(fat, file, file_item, p_index);
        return 0;
    }

    return -1;
}

int fatfs_read(char *buf, int size, file_t *file)
{
    fat_t *fat = (fat_t *)file->fs->data;

    uint32_t nbytes = size;
    if (file->pos + nbytes > file->size)
        nbytes = file->size - file->pos;

    uint32_t total_read = 0;

    while (nbytes > 0)
    {
        uint32_t curr_read = nbytes;
        uint32_t cluster_offset = file->pos % fat->cluster_byte_size;
        uint32_t start_sector = fat->data_start + (file->cblk - 2) * fat->sec_per_cluster;

        if (cluster_offset == 0 && nbytes == fat->cluster_byte_size)
        {
            int err = dev_read(fat->fs->dev_id, start_sector, buf, fat->sec_per_cluster);
            if (err < 0)
                return total_read;

            curr_read = fat->cluster_byte_size;
        }
        else
        {
            if (cluster_offset + curr_read > fat->cluster_byte_size)
                curr_read = fat->cluster_byte_size - cluster_offset;

            fat->curr_sector = -1;
            int err = dev_read(fat->fs->dev_id, start_sector, fat->fat_buffer, fat->sec_per_cluster);
            if (err < 0)
                return total_read;
            kernel_memcpy(buf, fat->fat_buffer + cluster_offset, curr_read);
        }

        buf += curr_read;
        nbytes -= curr_read;
        total_read += curr_read;

        int err = move_file_pos(file, fat, curr_read, 0);

        if (err < 0)
            return total_read;
    }

    return total_read;
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
    fat_t *fat = (fat_t *)fs->data;

    while (dir->index < fat->root_ent_cnt)
    {
        diritem_t *item = read_dir_entry(fat, dir->index);
        if (item == (diritem_t *)0)
            return -1;

        if (item->DIR_Name[0] == DIRITEM_NAME_END)
            break;

        if (item->DIR_Name[0] != DIRITEM_NAME_FREE)
        {
            file_type_t type = diritem_get_type(item);
            if (type == FILE_NORMAL || (type == FILE_DIR))
            {
                dirent->size = item->DIR_FileSize;
                dirent->type = type;
                diritem_get_name(item, dirent->name);
                dirent->index = dir->index++;
                return 0;
            }
        }

        dir->index++;
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