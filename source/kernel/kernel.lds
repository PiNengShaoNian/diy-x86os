SECTIONS {
    PROVIDE(kernel_base = .);
    . = 0x00010000;

    PROVIDE(s_text = .);
    .text : {
        *(EXCLUDE_FILE(*first_task*) .text)
    }

    .rodata : {
        *(EXCLUDE_FILE(*first_task*) .rodata)
    }
    PROVIDE(e_text = .);

    . = ALIGN(4096);
    PROVIDE(s_data = .);
    .data : {
        *(EXCLUDE_FILE(*first_task*) .data)
    }

    .bss : {
        *(EXCLUDE_FILE(*first_task*) .bss)
    }
    e_data = .;

    . = 0x80000000;
    PROVIDE(s_first_task = LOADADDR(.first_task));
    .first_task : AT(e_data) {
        *first_task_entry*(.text .rodata .bss .data)
        *first_task*(.text .rodata .bss .data)
    }
    PROVIDE(e_first_task = LOADADDR(.first_task) + SIZEOF(.first_task));

    PROVIDE(mem_free_start = e_first_task);
}