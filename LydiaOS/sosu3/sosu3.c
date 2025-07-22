/**
 * 素数计算程序
 * Prime Number Calculator
 * 
 * 使用埃拉托斯特尼筛法计算并输出指定范围内的所有素数
 * 展示动态内存分配和算法实现
 */
#include <stdio.h>
#include "apilib.h"

#define MAX		10000  /* 计算范围上限 */

void HariMain(void)
{
	char *flag, s[8];
	int i, j;
	
	/* 初始化内存分配器 */
	api_initmalloc();
	
	/* 动态分配标记数组 */
	flag = api_malloc(MAX);
	
	/* 初始化标记数组，所有数字初始标记为素数候选 */
	for (i = 0; i < MAX; i++) {
		flag[i] = 0;
	}
	
	/* 埃拉托斯特尼筛法计算素数 */
	for (i = 2; i < MAX; i++) {
		if (flag[i] == 0) {  /* 如果i还没有被标记，则为素数 */
			sprintf(s, "%d ", i);
			api_putstr0(s);
			
			/* 标记i的所有倍数为合数 */
			for (j = i * 2; j < MAX; j += i) {
				flag[j] = 1;	
			}
		}
	}
	
	/* 程序结束 */
	api_end();
}
