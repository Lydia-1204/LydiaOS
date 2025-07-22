/**
 * 下降音调演示程序
 * Descending Tone Demo
 * 
 * 播放从高音到低音的下降音调序列
 * 演示音频API和定时器的使用
 */
#include "apilib.h"

void HariMain(void)
{
	int i, timer;
	
	/* 分配定时器 */
	timer = api_alloctimer();
	api_inittimer(timer, 128);
	
	/* 播放下降音调序列 */
	for (i = 20000000; i >= 20000; i -= i / 100) {
		api_beep(i);        /* 播放指定频率的声音 */
		api_settimer(timer, 1);  /* 设置短暂延时 */
		
		/* 检查是否有按键中断播放 */
		if (api_getkey(1) != 128) {
			break;
		}
	}
	
	/* 停止播放 */
	api_beep(0);
	
	/* 程序结束 */
	api_end();
}
