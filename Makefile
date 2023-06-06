# 工具链前缀，如果是windows和mac，使用x86_64-elf-
# 如果是linux，使用x86_64-linux-gnu-
# 工具链前缀，如果是windows和mac，使用x86_64-elf-
# 如果是linux，使用x86_64-linux-gnu-
ifeq ($(LANG),)
	TOOL_PREFIX = x86_64-linux-gnu-
else
	TOOL_PREFIX = x86_64-elf-
endif

# GCC编译参数
CFLAGS = -g -c -O0 -m32 -fno-pie -fno-stack-protector -nostdlib -nostdinc

# 目标创建:涉及编译、链接、二进制转换、反汇编、写磁盘映像
all: source/os.c source/os.h source/start.S
	$(TOOL_PREFIX)gcc $(CFLAGS) source/start.S
	$(TOOL_PREFIX)gcc $(CFLAGS) source/os.c	
	$(TOOL_PREFIX)ld -m elf_i386 -Ttext=0x7c00 start.o os.o -o os.elf
	${TOOL_PREFIX}objcopy -O binary os.elf os.bin
	${TOOL_PREFIX}objdump -x -d -S  os.elf > os_dis.txt	
	${TOOL_PREFIX}readelf -a  os.elf > os_elf.txt	
	dd if=os.bin of=../image/disk.img conv=notrunc

# 清理
clean:
	rm -f *.elf *.o
