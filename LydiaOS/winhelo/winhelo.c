/**
 * 窗口 Hello World 程序
 * 创建简单窗口并等待用户按Enter键退出
 */
#include "apilib.h"

void HariMain(void)
{
	int win;
	char buf[150 * 50];  /* 窗口缓冲区 */
	
	/* 创建窗口 */
	win = api_openwin(buf, 150, 50, -1, "hello");
	
	/* 等待用户按Enter键(0x0a)退出 */
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break; 
		}
	}
	
	/* 程序结束 */
	api_end();
}
