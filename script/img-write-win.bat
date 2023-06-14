if not exist "disk1.vhd" (
    echo "disk1.vhd not found in image directory"
    notepad win_error.txt
    exit -1
)

if not exist "disk2.vhd" (
    echo "disk2.vhd not found in image directory"
    notepad win_error.txt
    exit -1
)

set DISK1_NAME=disk1.vhd

dd if=boot.bin of=%DISK1_NAME% bs=512 conv=notrunc count=1

dd if=loader.bin of=%DISK1_NAME% bs=512 conv=notrunc seek=1

dd if=kernel.elf of=%DISK1_NAME% bs=512 conv=notrunc seek=100

@REM dd if=init.elf of=%DISK1_NAME% bs=512 conv=notrunc seek=5000
@REM dd if=shell.elf of=%DISK1_NAME% bs=512 conv=notrunc seek=5000

@REM set DISK2_NAME=disk2.vhd
@REM set TARGET_PATH=k
@REM echo select vdisk file="%cd%\%DISK2_NAME%" >a.txt
@REM echo attach vdisk >>a.txt
@REM echo select partition 1 >> a.txt
@REM echo assign letter=%TARGET_PATH% >> a.txt
@REM diskpart /s a.txt
@REM if %errorlevel% neq 0 (
@REM     echo "attach disk2 failed"
@REM     exit -1
@REM )
@REM del a.txt

@REM copy /Y init.elf %TARGET_PATH%:\init
@REM copy /Y shell.elf %TARGET_PATH%:\shell.elf
@REM copy /Y loop %TARGET_PATH%:\loop

@REM echo select vdisk file="%cd%\%DISK2_NAME%" >a.txt
@REM echo detach vdisk >>a.txt
@REM diskpart /s a.txt
@REM if %errorlevel% neq 0 (
@REM     echo "attach disk2 failed"
@REM     exit -1
@REM )
@REM del a.txt
