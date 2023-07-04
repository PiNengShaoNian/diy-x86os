#include "dev/kbd.h"
#include "cpu/irq.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "dev/tty.h"

static kbd_state_t kbd_stat;

/**
 * 键盘映射表，分3类
 * normal是没有shift键按下，或者没有numlock按下时默认的键值
 * func是按下shift或者numlock按下时的键值
 * esc是以esc开头的的键值
 */
static const key_map_t map_table[256] = {
    [0x2] = {'1', '!'},
    [0x3] = {'2', '@'},
    [0x4] = {'3', '#'},
    [0x5] = {'4', '$'},
    [0x6] = {'5', '%'},
    [0x7] = {'6', '^'},
    [0x08] = {'7', '&'},
    [0x09] = {'8', '*'},
    [0x0A] = {'9', '('},
    [0x0B] = {'0', ')'},
    [0x0C] = {'-', '_'},
    [0x0D] = {'=', '+'},
    [0x0E] = {0x7F, 0x7F},
    [0x0F] = {'\t', '\t'},
    [0x10] = {'q', 'Q'},
    [0x11] = {'w', 'W'},
    [0x12] = {'e', 'E'},
    [0x13] = {'r', 'R'},
    [0x14] = {'t', 'T'},
    [0x15] = {'y', 'Y'},
    [0x16] = {'u', 'U'},
    [0x17] = {'i', 'I'},
    [0x18] = {'o', 'O'},
    [0x19] = {'p', 'P'},
    [0x1A] = {'[', '{'},
    [0x1B] = {']', '}'},
    [0x1C] = {'\n', '\n'},
    [0x1E] = {'a', 'A'},
    [0x1F] = {'s', 'B'},
    [0x20] = {'d', 'D'},
    [0x21] = {'f', 'F'},
    [0x22] = {'g', 'G'},
    [0x23] = {'h', 'H'},
    [0x24] = {'j', 'J'},
    [0x25] = {'k', 'K'},
    [0x26] = {'l', 'L'},
    [0x27] = {';', ':'},
    [0x28] = {'\'', '"'},
    [0x29] = {'`', '~'},
    [0x2B] = {'\\', '|'},
    [0x2C] = {'z', 'Z'},
    [0x2D] = {'x', 'X'},
    [0x2E] = {'c', 'C'},
    [0x2F] = {'v', 'V'},
    [0x30] = {'b', 'B'},
    [0x31] = {'n', 'N'},
    [0x32] = {'m', 'M'},
    [0x33] = {',', '<'},
    [0x34] = {'.', '>'},
    [0x35] = {'/', '?'},
    [0x39] = {' ', ' '},
};

void kbd_init(void)
{
    static int initialized = 0;

    if (!initialized)
    {
        kernel_memset(&kbd_stat, 0, sizeof(kbd_stat));
        irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_kbd);
        irq_enable(IRQ1_KEYBOARD);
    }

    initialized = 1;
}

static inline int is_make_code(uint8_t key_code)
{
    return !(key_code & 0x80);
}

static inline char get_key(uint8_t key_code)
{
    return key_code & 0x7f;
}

static void do_normal_key(uint8_t raw_code)
{
    char key = get_key(raw_code);
    int is_make = is_make_code(raw_code);

    switch (key)
    {
    case KEY_RSHIFT:
        kbd_stat.rshift_press = is_make ? 1 : 0;
        break;
    case KEY_LSHIFT:
        kbd_stat.lshift_press = is_make ? 1 : 0;
        break;
    case KEY_CAPS:
        if (is_make)
            kbd_stat.caps_lock = ~kbd_stat.caps_lock;
        break;
    case KEY_ALT:
        kbd_stat.lalt_press = is_make;
        break;
    case KEY_CTRL:
        kbd_stat.lctrl_press = is_make;
        break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
        break;
    default:
        if (is_make)
        {
            if (kbd_stat.rshift_press || kbd_stat.lshift_press)
                key = map_table[key].func;
            else
                key = map_table[key].normal;

            if (kbd_stat.caps_lock)
            {
                if (key >= 'A' && key <= 'Z')
                    key = key - 'A' + 'a';
                else if (key >= 'a' && key <= 'z')
                    key = key - 'a' + 'A';
            }

            tty_in(0, key);
        }
        break;
    }
}

static void do_e0_key(uint8_t raw_code)
{
    char key = get_key(raw_code);
    int is_make = is_make_code(raw_code);

    switch (key)
    {
    case KEY_CTRL:
        kbd_stat.lctrl_press = is_make;
        break;
    case KEY_ALT:
        kbd_stat.lalt_press = is_make;
        break;
    default:
        break;
    }
}

void do_handler_kbd(exception_frame_t *frame)
{
    static enum {
        NORMAL,
        BEGIN_E0,
        BEGIN_E1,
    } recv_state = NORMAL;

    uint32_t status = inb(KBD_PORT_STAT);
    if (!(status & KBD_STAT_RECV_READY))
    {
        pic_send_eoi(IRQ1_KEYBOARD);
        return;
    }

    uint8_t raw_code = inb(KBD_PORT_DATA);
    pic_send_eoi(IRQ1_KEYBOARD);

    if (raw_code == KEY_E0)
        recv_state = BEGIN_E0;
    else if (raw_code == KEY_E1)
        recv_state = BEGIN_E1;
    else
    {
        switch (recv_state)
        {
        case NORMAL:
            do_normal_key(raw_code);
            break;
        case BEGIN_E0:
            do_e0_key(raw_code);
            recv_state = NORMAL;
            break;
        case BEGIN_E1:
            recv_state = NORMAL;
            break;
        default:
            break;
        }
    }
}