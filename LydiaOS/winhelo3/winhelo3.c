/**
 * 动态内存分配窗口程序
 * 展示动态内存分配API和窗口操作的结合使用
 */
#include "apilib.h"

void HariMain(void)
{
	char *buf;  /* 动态分配的窗口缓冲区 */
	int win;

	/* 初始化内存分配器 */
	api_initmalloc();
	
	/* 动态分配窗口缓冲区 */
	buf = api_malloc(150 * 50);
	
	/* 创建窗口 */
	win = api_openwin(buf, 150, 50, -1, "hello");
	
	/* 绘制背景和文本 */
	api_boxfilwin(win,  8, 36, 141, 43, 6);
	api_putstrwin(win, 28, 28, 0, 12, "hello, world");
	
	/* 等待用户按Enter键(0x0a)退出 */
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break; 
		}
	}
	
	/* 程序结束 */
	api_end();
}
