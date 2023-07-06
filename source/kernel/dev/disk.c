#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"

static disk_t disk_buf[DISK_CNT];

static void disk_send_cmd(disk_t *disk, uint32_t start_sector, uint32_t sector_count, int cmd)
{
    outb(DISK_DRIVE(disk), DISK_DRIVE_BASE | disk->drive);
    outb(DISK_SECTOR_COUNT(disk), (uint8_t)(sector_count >> 8));
    outb(DISK_LBA_LO(disk), (uint8_t)(start_sector >> 24));
    outb(DISK_LBA_MID(disk), 0);
    outb(DISK_LBA_HI(disk), 0);

    outb(DISK_SECTOR_COUNT(disk), (uint8_t)(sector_count));
    outb(DISK_LBA_LO(disk), (uint8_t)(start_sector));
    outb(DISK_LBA_MID(disk), (uint8_t)(start_sector >> 8));
    outb(DISK_LBA_HI(disk), (uint8_t)(start_sector >> 16));

    outb(DISK_CMD(disk), cmd);
}

static void disk_read_data(disk_t *disk, void *buf, int size)
{
    uint16_t *c = (uint16_t *)buf;

    for (int i = 0; i < size / 2; i++)
        *c++ = inw(DISK_DATA(disk));
}

static void disk_write_data(disk_t *disk, void *buf, int size)
{
    uint16_t *c = (uint16_t *)buf;

    for (int i = 0; i < size / 2; i++)
        outw(DISK_DATA(disk), *c++);
}

static int disk_wait_data(disk_t *disk)
{
    uint8_t status;

    do
    {
        status = inb(DISK_STATUS(disk));
        if ((status & (DISK_STATUS_BUSY | DISK_STATUS_DRQ | DISK_STATUS_ERR)) != DISK_STATUS_BUSY)
        {
            break;
        }
    } while (1);

    return (status & DISK_STATUS_ERR) ? -1 : 0;
}

static void print_disk_info(disk_t *disk)
{
    log_printf("%s", disk->name);
    log_printf("  port base: %x", disk->port_base);
    log_printf("  total size: %d m", disk->sector_count * disk->sector_size / 1024 / 1024);
}

static int identify_disk(disk_t *disk)
{
    disk_send_cmd(disk, 0, 0, DISK_CMD_IDENTIFY);

    int err = inb(DISK_STATUS(disk));
    if (err == 0)
    {
        log_printf("%s doesn't exist", disk->name);
        return -1;
    }

    err = disk_wait_data(disk);
    if (err < 0)
    {
        log_printf("disk[%s]: read failed.", disk->name);
        return err;
    }

    uint16_t buf[256];
    disk_read_data(disk, buf, sizeof(buf));
    disk->sector_count = *(uint32_t *)(buf + 100);
    disk->sector_size = SECTOR_SIZE;
    return 0;
}

void disk_init(void)
{
    log_printf("Check disk...");

    kernel_memset(disk_buf, 0, sizeof(disk_buf));
    for (int i = 0; i < DISK_PER_CHANNEL; i++)
    {
        disk_t *disk = disk_buf + i;

        kernel_sprintf(disk->name, "sd%c", i + 'a');
        disk->drive = (i == 0) ? DISK_MASTER : DISK_SLAVE;
        disk->port_base = IOBASE_PRIMARY;

        int err = identify_disk(disk);
        if (err == 0)
        {
            print_disk_info(disk);
        }
    }
}