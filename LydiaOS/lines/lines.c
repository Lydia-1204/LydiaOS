/**
 * 线条绘制测试程序
 * 展示窗口中的线条绘制API功能
 */
#include "apilib.h"

void HariMain(void)
{
	char *buf;  /* 动态分配的窗口缓冲区 */
	int win, i;
	
	/* 初始化内存分配器 */
	api_initmalloc();
	
	/* 分配窗口缓冲区 */
	buf = api_malloc(160 * 100);
	
	/* 创建窗口 */
	win = api_openwin(buf, 160, 100, -1, "lines");
	
	/* 绘制不同颜色的线条 */
	for (i = 0; i < 8; i++) {
		/* 绘制水平线条 */
		api_linewin(win + 1,  8, 26, 77, i * 9 + 26, i);
		/* 绘制倾斜线条 */
		api_linewin(win + 1, 88, 26, i * 9 + 88, 89, i);
	}
	
	/* 刷新窗口显示区域 */
	api_refreshwin(win,  6, 26, 154, 90);
	
	/* 等待用户按Enter键(0x0a)退出 */
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break;
		}
	}
	
	/* 关闭窗口 */
	api_closewin(win);
	
	/* 程序结束 */
	api_end();
}
