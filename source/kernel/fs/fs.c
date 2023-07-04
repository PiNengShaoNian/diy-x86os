#include "fs/fs.h"
#include "tools/klib.h"
#include "comm/types.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include <sys/stat.h>
#include "dev/console.h"
#include "fs/file.h"
#include "dev/dev.h"
#include "core/task.h"
#include "tools/log.h"

static uint8_t TEMP_ADDR[100 * 1024];
static uint8_t *temp_pos;

#define TEMP_FILE_ID 100

static void read_disk(uint32_t sector, int sector_count, uint8_t *buf)
{
    outb(0x1F6, 0xE0);
    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24));
    outb(0x1F4, 0);
    outb(0x1F5, 0);

    outb(0x1F2, (uint8_t)sector_count);
    outb(0x1F3, (uint8_t)(sector));
    outb(0x1F4, (uint8_t)(sector >> 8));
    outb(0x1F5, (uint8_t)(sector >> 16));

    outb(0x1F7, 0x24);

    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count--)
    {
        while ((inb(0x1F7) & 0x88) != 0x8)
            ;

        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
}

static int is_path_valid(const char *path)
{
    if (path == (const char *)0 || path[0] == '\0')
        return 0;

    return 1;
}

int sys_open(const char *name, int flags, ...)
{
    if (kernel_strncmp(name, "tty", 3) == 0)
    {
        if (!is_path_valid(name))
        {
            log_printf("path is not valid");
            return -1;
        }

        int fd = -1;
        file_t *file = file_alloc();
        if (file)
        {
            fd = task_alloc_fd(file);
            if (fd < 0)
                goto sys_open_failed;
        }
        else
        {
            goto sys_open_failed;
        }

        if (kernel_strlen(name) < 5)
            goto sys_open_failed;

        int num = name[4] - '0';
        int dev_id = dev_open(DEV_TTY, num, (void *)0);
        if (dev_id < 0)
            goto sys_open_failed;

        file->dev_id = dev_id;
        file->mode = 0;
        file->pos = 0;
        file->ref = 1;
        file->type = FILE_TTY;
        kernel_strncpy(file->file_name, name, FILE_NAME_SIZE);
        return fd;

    sys_open_failed:
        if (file)
            file_free(file);

        if (fd >= 0)
            task_remove_fd(fd);
    }
    else
    {
        if (name[0] == '/')
        {
            read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
            temp_pos = (uint8_t *)TEMP_ADDR;
            return TEMP_FILE_ID;
        }
    }

    return -1;
}

int sys_read(int file, char *ptr, int len)
{
    if (file == TEMP_FILE_ID)
    {
        kernel_memcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }

    return -1;
}

int sys_write(int file, char *ptr, int len)
{
    file = 0;
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened");
        return -1;
    }

    return dev_write(p_file->dev_id, 0, ptr, len);
}

int sys_lseek(int file, int ptr, int dir)
{
    if (file == TEMP_FILE_ID)
    {
        temp_pos = (uint8_t *)(TEMP_ADDR + ptr);
        return 0;
    }

    return -1;
}

int sys_close(int file)
{
    return 0;
}

int sys_isatty(int file)
{
    return -1;
}

int sys_fstat(int file, struct stat *st)
{
    return -1;
}

void fs_init(void)
{
    file_table_init();
}