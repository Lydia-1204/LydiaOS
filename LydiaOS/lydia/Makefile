# LydiaOS Enhanced Operating System Makefile
# Build configuration for kernel and applications

# ===== 目标文件定义 =====
OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
		int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
		mtask.obj window.obj console.obj file.obj scheduler.obj

# ===== 工具路径配置 =====
TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/lydia/

# ===== 编译工具定义 =====
MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../lydia.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

# ===== 默认构建目标 =====
default :
	$(MAKE) ipl10.bin
	$(MAKE) lydia.sys
	$(MAKE) lydia.img

# ===== 运行操作系统 =====
run : lydia.img
	@echo ========================================
	@echo  Starting LydiaOS Enhanced Edition
	@echo ========================================
	@echo.
	@echo System Features:
	@echo  * Priority-based Task Scheduling
	@echo  * Enhanced Process State Management
	@echo  * Semaphore and Message Queue IPC
	@echo  * Advanced Memory Management
	@echo    - Best Fit Algorithm
	@echo    - Buddy System
	@echo    - First Fit Algorithm
	@echo.
	@echo Available Console Commands:
	@echo   ps       - Show process list
	@echo   sched    - Show scheduler information  
	@echo   ipc      - Show IPC information
	@echo   priority - Set task priority
	@echo   sem      - Semaphore operations
	@echo   msg      - Message queue operations
	@echo   mem      - Show memory information
	@echo   memtest  - Run memory allocation tests
	@echo.
	@echo Press any key in QEMU to start...
	@echo ========================================
	$(TOOLPATH)qemu\qemu.exe -L $(TOOLPATH)qemu\ -m 32 -localtime -std-vga -fda lydia.img

# ===== 镜像构建规则 =====
lydia.img : ipl10.bin lydia.sys
	copy /B ipl10.bin+lydia.sys lydia.img

# ===== 启动扇区构建 =====
# 新的多阶段启动系统 (暂时禁用，保留用于future扩展)
bootloader.bin : bootloader.nas Makefile
	$(NASK) bootloader.nas bootloader.bin bootloader.lst

stage2.bin : stage2.nas Makefile
	$(NASK) stage2.nas stage2.bin stage2.lst

# 当前使用的启动程序
ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

lydia.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb lydia.sys


%.gas : %.c bootpack.h Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst


clean :
	-$(DEL) asmhead.bin
	-$(DEL) hankaku.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb

src_only :
	$(MAKE) clean
	-$(DEL) ipl10.bin
	-$(DEL) lydia.sys
