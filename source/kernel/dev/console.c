#include "dev/console.h"
#include "tools/klib.h"

#define CONSOLE_NR 1
static console_t console_buf[CONSOLE_NR];

static void erase_rows(console_t *console, int start, int end)
{
    disp_char_t *disp_start = console->disp_base + console->display_cols * start;
    disp_char_t *disp_end = console->disp_base + console->display_cols * end;

    while (disp_start < disp_end)
    {
        disp_start->c = ' ';
        disp_start->foreground = console->foreground;
        disp_start->background = console->background;
        disp_start++;
    }
}

static void scroll_up(console_t *console, int lines)
{
    disp_char_t *dest = console->disp_base;
    disp_char_t *src = console->disp_base + console->display_cols * lines;

    uint32_t size = (console->display_rows - lines) * console->display_cols * sizeof(disp_char_t);
    kernel_memcpy(dest, src, size);

    erase_rows(console, console->display_rows - lines, console->display_rows);
    console->cursor_row -= lines;
}

static void move_to_col0(console_t *console)
{
    console->cursor_col = 0;
}

static void move_next_line(console_t *console)
{
    console->cursor_row++;
    if (console->cursor_row >= console->display_rows)
        scroll_up(console, 1);
}

static void move_forward(console_t *console, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (++console->cursor_col >= console->display_cols)
        {
            console->cursor_row++;
            console->cursor_col = 0;

            if (console->cursor_row >= console->display_rows)
                scroll_up(console, 1);
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

static void clear_display(console_t *console)
{
    int size = console->display_cols * console->display_rows;

    disp_char_t *start = console->disp_base;
    for (int i = 0; i < size; i++)
    {
        start->c = ' ';
        start->foreground = console->foreground;
        start->background = console->background;
        start++;
    }
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

        clear_display(console);
    }

    return 0;
}

int console_write(int console_idx, char *data, int size)
{
    console_t *console = console_buf + console_idx;
    int len;

    for (len = 0; len < size; len++)
    {
        char ch = *data++;
        switch (ch)
        {
        case '\n':
            move_to_col0(console);
            move_next_line(console);
            break;
        default:
            show_char(console, ch);
        }
    }

    return len;
}

void console_close(int console);