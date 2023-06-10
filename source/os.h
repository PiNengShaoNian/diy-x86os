#ifndef OS_H
#define OS_H

// 0-1:  RPL, 2: table indicator, 3-15: index

// 代码段选择子
#define KERNEL_CODE_SEG 8
// 数据段选择子
#define KERNEL_DATA_SEG 16

#define APP_CODE_SEG ((3 * 8) | 3)
#define APP_DATA_SEG ((4 * 8) | 3)

#define TASK0_TSS_SEL ((5 * 8))
#define TASK1_TSS_SEL ((6 * 8))
#define SYSCALL_SEL ((7 * 8))

#define TASK0_LDT_SEG (8 * 8) // 任务0 LDTs
#define TASK1_LDT_SEG (9 * 8) // 任务1 LDT

#define TASK_CODE_SEG ((0 * 8) | 0x4 | 0x3) // LDT, 任务代码段
#define TASK_DATA_SEG ((1 * 8) | 0x4 | 0x3) // LDT, 任务代码段

#endif // OS_H
