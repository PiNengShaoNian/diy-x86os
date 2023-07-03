#ifndef KBD_H
#define KBD_H

#include "comm/types.h"

#define KEY_RSHIFT 0x36
#define KEY_LSHFIT 0x2A

typedef struct _key_map_t
{
    uint8_t normal;
    uint8_t func;
} key_map_t;

typedef struct _kbd_state_t
{
    int lshift_press : 1;
    int rshift_press : 1;
} kbd_state_t;

#define KBD_PORT_DATA 0x60
#define KBD_PORT_STAT 0x64
#define KBD_PORT_CMD 0x64

#define KBD_STAT_RECV_READY (1 << 0)

void kbd_init(void);
void exception_handler_kbd(void);

#endif