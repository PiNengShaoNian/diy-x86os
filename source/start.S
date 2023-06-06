#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start

	// 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
  	.code16

	// 以下是代码区
 	.text
_start: 
    mov $0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %gs
	mov %ax, %fs
	mov $_start, %esp

// 调用bios中断加载一扇区后面的代码到内存中
read_self_all:
    mov $0x7E00, %bx  // 要加载到的位置
	mov $0x2, %cx  // 起始扇区
	mov $0x240, %ax // 0x02 读操作, 0x40 读取多少个扇区
	mov $0x80, %dx // 读取哪一个硬盘(0x80是第一个硬盘)
	int $0x13 // 发起中断
	jc read_self_all

// 进入保护模式
//  1) 关中断
//  2) 加载gdt表的大小以及位置到gdtr寄存器中
//  3) 将cr0寄存器的protection位置为1
//  4) 发起一个长跳转清空流水线
	cli
	lgdt gdt_desc
	mov $1, %eax
	lmsw %ax
	jmp $KERNEL_CODE_SEG, $_start_32

	.org 0x1fe // 510
	.byte 0x55, 0xaa

    .code32
	.text
_start_32:
    mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %gs
	mov %ax, %fs
	mov $_start, %esp
	jmp .

gdt_desc:
    .word (256*8)-1
	.long gdt_table