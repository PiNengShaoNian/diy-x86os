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
        *(*.bss)
    }
}