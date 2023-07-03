#include "dev/console.h"

#define CONSOLE_NR 1
static console_t console_buf[CONSOLE_NR];

static void move_forward(console_t *console, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (++console->cursor_col >= console->display_cols)
        {
            console->cursor_row++;
            console->cursor_col = 0;
        }
    }
}

static void show_char(console_t *console, char c)
{
    int offset = console->cursor_col + console->cursor_row * console->display_cols;
    disp_char_t *p = console->disp_base + offset;
    p->c = c;
    p->foreground = console->foreground;
    p->background = console->background;
    move_forward(console, 1);
}

int console_init(void)
{
    for (int i = 0; i < CONSOLE_NR; i++)
    {
        console_t *console = console_buf + i;

        console->cursor_row = console->cursor_col = 0;
        console->display_rows = CONSOLE_ROW_MAX;
        console->display_cols = CONSOLE_COL_MAX;
        console->disp_base = (disp_char_t *)CONSOLE_DISP_ADDR +
                             i * CONSOLE_ROW_MAX * CONSOLE_COL_MAX;
        console->foreground = COLOR_White;
        console->background = COLOR_Black;
    }

    return 0;
}

int console_write(int console, char *data, int size)
{
    console_t *c = console_buf + console;
    int len;

    for (len = 0; len < size; len++)
    {
        char ch = *data++;
        show_char(c, ch);
    }

    return len;
}

void console_close(int console);