/**
 * 增强窗口 Hello World 程序
 * 创建窗口并在其中显示文本和背景
 */
#include "apilib.h"

void HariMain(void)
{
	int win;
	char buf[150 * 50];  /* 窗口缓冲区 */
	
	/* 创建窗口 */
	win = api_openwin(buf, 150, 50, -1, "hello");
	
	/* 绘制背景矩形 */
	api_boxfilwin(win,  8, 36, 141, 43, 3);
	
	/* 在窗口中显示文本 */
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
