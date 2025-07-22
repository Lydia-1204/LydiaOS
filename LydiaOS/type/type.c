/**
 * 文件显示程序
 * File Display Program
 * 
 * 类似于Unix的cat命令，显示文件内容
 * 支持命令行参数指定文件名
 */
#include "apilib.h"

void HariMain(void)
{
	int fh;
	char c, cmdline[30], *p;

	/* 获取命令行参数 */
	api_cmdline(cmdline, 30);
	
	/* 跳过程序名，找到文件名参数 */
	for (p = cmdline; *p > ' '; p++) { }	/* 跳过程序名 */
	for (; *p == ' '; p++) { }	/* 跳过空格 */
	
	/* 打开文件 */
	fh = api_fopen(p);
	if (fh != 0) {
		/* 逐字符读取并显示文件内容 */
		for (;;) {
			if (api_fread(&c, 1, fh) == 0) {
				break;  /* 文件结束 */
			}
			api_putchar(c);
		}
	} else {
		/* 文件不存在或无法打开 */
		api_putstr0("File not found.\n");
	}
	api_end();
}
