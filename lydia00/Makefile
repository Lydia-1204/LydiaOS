TOOLPATH = ../z_tools/
MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del


default :
	$(MAKE) img


ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

lydia.sys : lydia.nas Makefile
	$(NASK) lydia.nas lydia.sys lydia.lst

lydia.img : ipl10.bin lydia.sys Makefile
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:lydia.sys to:@: \
		imgout:lydia.img


img :
	$(MAKE) lydia.img

run :
	$(MAKE) img
	$(COPY) lydia.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: lydia.img

clean :
	-$(DEL) ipl10.bin
	-$(DEL) ipl10.lst
	-$(DEL) lydia.sys
	-$(DEL) lydia.lst

src_only :
	$(MAKE) clean
	-$(DEL) lydia.img
