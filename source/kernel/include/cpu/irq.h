#ifndef IRQ_H
#define IRQ_H

typedef struct _exception_frame_t
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
} exception_frame_t;

void irq_init(void);

#endif