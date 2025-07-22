/**
 * Noodle 时钟程序
 * 在窗口中显示实时时钟，展示定时器和窗口API的使用
 */
#include <stdio.h>
#include "apilib.h"

void HariMain(void)
{
	char *buf, s[12];
	int win, timer, sec = 0, min = 0, hou = 0;
	
	/* 初始化内存分配器 */
	api_initmalloc();
	
	/* 分配窗口缓冲区 */
	buf = api_malloc(150 * 50);
	
	/* 创建窗口 */
	win = api_openwin(buf, 150, 50, -1, "noodle");
	
	/* 分配定时器 */
	timer = api_alloctimer();
	api_inittimer(timer, 128);
	
	/* 主循环 - 更新时钟显示 */
	for (;;) {
		/* 格式化时间字符串 */
		sprintf(s, "%5d:%02d:%02d", hou, min, sec);
		
		/* 清除显示区域并更新时间 */
		api_boxfilwin(win, 28, 27, 115, 41, 7);
		api_putstrwin(win, 28, 27, 0 , 11, s);
		
		/* 设置100ms定时器 */
		api_settimer(timer, 100);
		
		/* 检查是否有按键退出 */
		if (api_getkey(1) != 128) {
			break;
		}
		
		/* 时间递增逻辑 */
		sec++;
		if (sec == 60) {
			sec = 0;
			min++;
			if (min == 60) {
				min = 0;
				hou++;
			}
		}
	}
	api_end();
}
