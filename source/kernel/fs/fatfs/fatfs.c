#include "fs/fatfs/fatfs.h"
#include "fs/fs.h"

int fatfs_mount(struct _fs_t *fs, int major, int minor)
{
    return -1;
}

void fatfs_unmount(struct _fs_t *fs)
{
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

fs_op_t fatfs_op = {
    .mount = fatfs_mount,
    .unmount = fatfs_unmount,
    .open = fatfs_open,
    .read = fatfs_read,
    .write = fatfs_write,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .stat = fatfs_stat,
};