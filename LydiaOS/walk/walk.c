/**
 * 移动星号游戏
 * Walking Star Game
 * 
 * 使用数字键2,4,6,8控制星号在窗口中移动
 * 演示键盘输入处理和图形更新
 */
#include "apilib.h"

void HariMain(void)
{
	char *buf;
	int win, i, x, y;
	
	/* 初始化内存分配器 */
	api_initmalloc();
	
	/* 分配窗口缓冲区 */
	buf = api_malloc(160 * 100);
	
	/* 创建窗口 */
	win = api_openwin(buf, 160, 100, -1, "walk");
	
	/* 绘制黑色背景 */
	api_boxfilwin(win, 4, 24, 155, 95, 0);
	
	/* 初始位置 */
	x = 76;
	y = 56;
	
	/* 绘制初始星号 */
	api_putstrwin(win, x, y, 3, 1, "*");
	
	/* 主循环：处理键盘输入并移动星号 */
	for (;;) {
		i = api_getkey(1);
		
		/* 清除当前位置的星号 */
		api_putstrwin(win, x, y, 0, 1, "*"); 
		
		/* 根据按键移动位置 */
		if (i == '4' && x >   4) { x -= 8; }  /* 左移 */
		if (i == '6' && x < 148) { x += 8; }  /* 右移 */
		if (i == '8' && y >  24) { y -= 8; }  /* 上移 */
		if (i == '2' && y <  80) { y += 8; }  /* 下移 */
		if (i == 0x0a) { break; } 
		api_putstrwin(win, x, y, 3, 1, "*");
	}	
	api_closewin(win);
	api_end();
}
