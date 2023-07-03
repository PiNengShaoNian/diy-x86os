#ifndef CONSOLE_H
#define CONSOLE_H

#include "comm/types.h"

#define CONSOLE_DISP_ADDR 0xb8000
#define CONSOLE_DISP_END (0xb8000 + 32 * 1024)
#define CONSOLE_ROW_MAX 25
#define CONSOLE_COL_MAX 80

typedef struct _disp_char_t
{
    uint16_t v;
} disp_char_t;

typedef struct _console_t
{
    disp_char_t *disp_base;
    int display_rows;
    int display_cols;
} console_t;

int console_init(void);
int console_write(int console, char *data, int size);
void console_close(int console);

#endif