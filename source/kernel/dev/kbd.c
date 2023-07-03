#include "dev/kbd.h"
#include "cpu/irq.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"

void kbd_init(void)
{
    irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_kbd);
    irq_enable(IRQ1_KEYBOARD);
}

void do_handler_kbd(exception_frame_t *frame)
{
    uint32_t status = inb(KBD_PORT_STAT);
    if (!(status & KBD_STAT_RECV_READY))
    {
        pic_send_eoi(IRQ1_KEYBOARD);
        return;
    }

    uint8_t raw_code = inb(KBD_PORT_DATA);
    log_printf("key: %d", raw_code);
    pic_send_eoi(IRQ1_KEYBOARD);
}