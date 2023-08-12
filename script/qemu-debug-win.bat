@REM 适用于windows
start qemu-system-i386  -m 128M -s -S -netdev tap,id=mynet0,ifname=tap -device rtl8139,netdev=mynet0,mac=52:54:00:c9:18:27 -serial stdio  -drive file=disk1.vhd,index=0,media=disk,format=raw -drive file=disk2.vhd,index=1,media=disk,format=raw -d pcall,page,mmu,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation
