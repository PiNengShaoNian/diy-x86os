#ifndef FS_H
#define FS_H

#include <sys/stat.h>
#include "file.h"

struct _fs_t;

typedef struct _fs_op_t
{
    int (*mount)(struct _fs_t *fs, int major, int minor);
    void (*unmount)(struct _fs_t *fs);
    int (*open)(struct _fs_t *fs, const char *path, file_t *file);
    int (*read)(char *buf, int size, file_t *file);
    int (*write)(char *buf, int size, file_t *file);
    void (*close)(file_t *file);
    int (*seek)(file_t *file, uint32_t offset, int dir);
    int (*stat)(file_t *file, struct stat *st);
} fs_op_t;

typedef struct _fs_t
{
    fs_op_t *op;
} fs_t;

void fs_init(void);

int sys_open(const char *name, int flags, ...);
int sys_read(int file, char *ptr, int len);
int sys_write(int file, char *ptr, int len);
int sys_lseek(int file, int ptr, int dir);
int sys_close(int file);
int sys_isatty(int file);
int sys_fstat(int file, struct stat *st);
int sys_dup(int file);

#endif