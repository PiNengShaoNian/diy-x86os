ENTRY(_start)
SECTIONS
{
    . = 0x81000000;
    .text : {
        *(*.text)
    }

    .rodata : {
        *(*.rodata)
    }

    .data : {
        *(*.data)
    }

    .bss : {
        PROVIDE(__bss_start__ = .);
        *(*.bss)
        PROVIDE(__bss_end__ = .);
    }
}