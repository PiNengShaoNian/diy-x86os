#ifndef OS_H
#define OS_H


// 0-1:  RPL, 2: table indicator, 3-15: index

// 代码段选择子
#define KERNEL_CODE_SEG 8
// 数据段选择子
#define KERNEL_DATA_SEG 16

#define APP_CODE_SEG ((3 * 8) | 3)
#define APP_DATA_SEG ((4 * 8) | 3)

#endif // OS_H
