#include "dev/console.h"

#define CONSOLE_NR 1
static console_t console_buf[CONSOLE_NR];

int console_init(void)
{
    for (int i = 0; i < CONSOLE_NR; i++)
    {
        console_t *console = console_buf + i;
        console->display_rows = CONSOLE_ROW_MAX;
        console->display_cols = CONSOLE_COL_MAX;
        console->disp_base = (disp_char_t *)CONSOLE_DISP_ADDR +
                             i * CONSOLE_ROW_MAX * CONSOLE_COL_MAX;
    }

    return 0;
}

int console_write(int console, char *data, int size)
{
    console_t *c = console_buf + console;
    int len;

    for (len = 0; len < size; len++)
    {
        char c = *data++;
    }

    return len;
}

void console_close(int console);