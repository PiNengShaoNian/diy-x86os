#ifndef FILE_H
#define FILE_H

#include "comm/types.h"

#define FILE_NAME_SIZE 32
#define FILE_TABLE_SIZE 2048

typedef enum _file_type_t
{
    FILE_UNKOWN = 0,
    FILE_TTY,
    FILE_DIR,
    FILE_NORMAL,
} file_type_t;
struct _fs_t;

typedef struct _file_t
{
    char file_name[FILE_NAME_SIZE];
    file_type_t type;
    uint32_t size;
    int ref;
    int dev_id;
    int pos;
    int mode;

    struct _fs_t *fs;
} file_t;

file_t *file_alloc(void);
void file_free(file_t *file);
void file_table_init(void);
void file_inc_ref(file_t *file);

#endif