/**
 * LydiaOS 控制台系统
 * Console System Implementation
 * 
 * 提供命令行界面、字符串解析和系统命令处理功能
 */
#include "bootpack.h"
#include <stdio.h>
#include <string.h>

/* ===== 字符串解析辅助函数 ===== */

/**
 * 简单的字符串转整数函数
 * @param str 输入字符串
 * @return 转换后的整数值
 */
int str_to_int(char *str) 
{
    int result = 0;
    int i = 0;
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return result;
}

/**
 * 解析包含两个整数的字符串
 * @param str 输入字符串
 * @param pattern 模式字符串(未使用，保持兼容性)
 * @param val1 第一个整数的输出指针
 * @param val2 第二个整数的输出指针
 * @return 成功解析的整数个数
 */
int parse_two_ints(char *str, char *pattern, int *val1, int *val2)
{
    int count = 0;
    int i = 0, j = 0;
    char temp[10];
    
    /* 跳过命令名 */
    while (str[i] != ' ' && str[i] != 0) i++;
    while (str[i] == ' ') i++;
    
    /* 解析第一个整数 */
    j = 0;
    while (str[i] >= '0' && str[i] <= '9' && j < 9) {
        temp[j++] = str[i++];
    }
    if (j > 0) {
        temp[j] = 0;
        *val1 = str_to_int(temp);
        count++;
    }
    
    /* 跳过空格 */
    while (str[i] == ' ') i++;
    
    /* 解析第二个整数 */
    j = 0;
    while (str[i] >= '0' && str[i] <= '9' && j < 9) {
        temp[j++] = str[i++];
    }
    if (j > 0) {
        temp[j] = 0;
        *val2 = str_to_int(temp);
        count++;
    }
    
    return count;
}

/**
 * 解析命令字符串: cmd word1 word2
 * @param str 输入字符串
 * @param word1 第一个单词的输出缓冲区
 * @param word2 第二个单词的输出缓冲区
 * @return 成功解析的单词个数
 */
int parse_command_words(char *str, char *word1, char *word2)
{
    int count = 0;
    int i = 0, j = 0;
    
    /* 跳过命令名 */
    while (str[i] != ' ' && str[i] != 0) i++;
    while (str[i] == ' ') i++;
    
    /* 解析第一个单词 */
    j = 0;
    while (str[i] != ' ' && str[i] != 0 && j < 15) {
        word1[j++] = str[i++];
    }
    if (j > 0) {
        word1[j] = 0;
        count++;
    }
    
    /* 跳过空格 */
    while (str[i] == ' ') i++;
    
    /* 解析第二个单词 */
    j = 0;
    while (str[i] != ' ' && str[i] != 0 && j < 15) {
        word2[j++] = str[i++];
    }
    if (j > 0) {
        word2[j] = 0;
        count++;
    }
    
    return count;
}

/**
 * 解析字符串: cmd word1 int1
 * @param str 输入字符串
 * @param word 单词的输出缓冲区
 * @param val 整数的输出指针
 * @return 成功解析的项目个数
 */
int parse_word_int(char *str, char *word, int *val)
{
    int count = 0;
    int i = 0, j = 0;
    char temp[10];
    
    /* 跳过命令名 */
    while (str[i] != ' ' && str[i] != 0) i++;
    while (str[i] == ' ') i++;
    
    /* 解析单词 */
    j = 0;
    while (str[i] != ' ' && str[i] != 0 && j < 15) {
        word[j++] = str[i++];
    }
    if (j > 0) {
        word[j] = 0;
        count++;
    }
    
    /* 跳过空格 */
    while (str[i] == ' ') i++;
    
    /* 解析整数 */
    j = 0;
    while (str[i] >= '0' && str[i] <= '9' && j < 9) {
        temp[j++] = str[i++];
    }
    if (j > 0) {
        temp[j] = 0;
        *val = str_to_int(temp);
        count++;
    }
    
    return count;
}

/* 外部键盘表声明 */
extern char keytable0[0x80];

void console_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[30];

	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;  /* 初始光标隐藏，只有获得焦点时才显示 */
	task->cons = &cons;
	task->cmdline = cmdline;

	/* 初始化光标定时器，但不启动 */
	cons.timer = 0;  /* 初始化为0 */
	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		if (cons.timer != 0) {  /* 检查定时器分配是否成功 */
			timer_init(cons.timer, &task->fifo, 1);
			/* 不立即启动定时器，等待窗口获得焦点时再启动 */
		} else {
			/* 定时器分配失败，记录错误 */
			cons.cur_c = -1;  /* 禁用光标 */
		}
	}
	
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	
	}
	task->fhandle = fhandle;
	task->fat = fat;

	/* 显示提示符 */
	cons_putchar(&cons, '>', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			
			/* 处理光标闪烁定时器 - 优化性能 */
			if (i <= 1 && cons.sht != 0 && cons.timer != 0) { 
				/* 只有光标可见时才处理闪烁 */
				if (cons.cur_c >= 0) {
					static int refresh_counter = 0;
					/* 避免频繁重新初始化定时器 */
					if (i != 0) {
						cons.cur_c = COL8_FFFFFF;  /* 白色光标 */
						/* 只在必要时重新初始化 */
						if (refresh_counter == 0) {
							timer_init(cons.timer, &task->fifo, 0);
						}
					} else {
						cons.cur_c = COL8_000000;  /* 黑色光标 */
						/* 只在必要时重新初始化 */
						if (refresh_counter == 0) {
							timer_init(cons.timer, &task->fifo, 1);
						}
					}
					/* 绘制光标，减少但不过度延迟刷新 */
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
					/* 适度延迟刷新 - 更频繁的更新以保持流畅 */
					refresh_counter++;
					if (refresh_counter >= 1) { /* 每次都刷新，保证快速闪烁的流畅性 */
						sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
						refresh_counter = 0;
					}
					/* 重设定时器，更快的闪烁频率 */
					timer_settime(cons.timer, 50);  /* 调整间隔到50ms，更快的闪烁 */
				}
			}
			
			/* 光标激活 - 窗口获得焦点时启动光标闪烁 */
			if (i == 2) {	
				cons.cur_c = COL8_FFFFFF;
				/* 启动光标闪烁定时器 */
				if (cons.timer != 0) {
					timer_init(cons.timer, &task->fifo, 1);
					timer_settime(cons.timer, 50);  /* 快速闪烁间隔：50ms，接近Windows速度 */
				}
				/* 立即显示光标 */
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
					sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
				}
			}
			
			/* 光标隐藏 - 窗口失去焦点时隐藏光标 */
			if (i == 3) {	
				/* 停止光标闪烁定时器 */
				if (cons.timer != 0) {
					timer_cancel(cons.timer);
				}
				/* 清除光标显示 */
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
					sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
				}
				cons.cur_c = -1;  /* 设置为-1表示光标不显示 */
			}
			
			/* 处理×按钮关闭信号 */
			if (i == 4) {
				cmd_exit(&cons, fat);
			}
			
			/* 处理键盘输入 */
			if (256 <= i && i <= 511) { 
				if (i == 0x0E + 256) {  /* 退格键 (扫描码0x0E) */
					if (cons.cur_x > 16) {
						/* 退格时暂停光标闪烁 */
						if (cons.timer != 0) {
							timer_cancel(cons.timer);
						}
						
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
						
						/* 退格后重新启动光标闪烁 */
						if (cons.timer != 0 && cons.cur_c >= 0) {
							cons.cur_c = COL8_FFFFFF;
							timer_settime(cons.timer, 50);  /* 快速闪烁频率 */
						}
					}
				} else if (i == 0x1C + 256) {  /* 回车键 (扫描码0x1C) */
					/* 清除光标 */
					cons_putchar(&cons, ' ', 0);
					/* 获取命令行 */
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					/* 执行命令 */
					cons_runcmd(cmdline, &cons, fat, memtotal);	
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					/* 显示新的提示符 */
					if (cons.sht != 0) {
						cons_putchar(&cons, '>', 1);
					}
				} else {  /* 普通字符 */
					/* 将扫描码转换为ASCII */
					char ch = 0;
					int scancode = i - 256;
					if (scancode < 0x80) {
						ch = keytable0[scancode];  /* 使用外部键盘表 */
					}
					if (ch != 0 && cons.cur_x < 384) {
						/* 输入时暂停光标闪烁 */
						if (cons.timer != 0) {
							timer_cancel(cons.timer);
						}
						
						cmdline[cons.cur_x / 8 - 2] = ch;
						cons_putchar(&cons, ch, 1);
						
						/* 输入后重新启动光标闪烁 */
						if (cons.timer != 0 && cons.cur_c >= 0) {
							cons.cur_c = COL8_FFFFFF;  /* 显示白色光标 */
							timer_settime(cons.timer, 50);  /* 快速闪烁频率，接近Windows体验 */
						}
					}
				}
			}
		}
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 384) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	
			}
		}
	} else if (s[0] == 0x0a) {
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	
	} else {	
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 384) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	if (cons->cur_y < 28 + 174) {
		cons->cur_y += 16; 
	} else {
		if (sheet != 0) {
			for (y = 28; y < 28 + 174; y++) {
				for (x = 8; x < 8 + 384; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + 174; y < 28 + 190; y++) {
				for (x = 8; x < 8 + 384; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			sheet_refresh(sheet, 8, 28, 8 + 384, 28 + 190);
		}
	}
	cons->cur_x = 8;
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "memtest") == 0 && cons->sht != 0) {
		cmd_memtest(cons, memtotal);
	} else if (strcmp(cmdline, "firstfit") == 0 && cons->sht != 0) {
		cmd_set_strategy(cons, ALLOC_FIRST_FIT);
	} else if (strcmp(cmdline, "bestfit") == 0 && cons->sht != 0) {
		cmd_set_strategy(cons, ALLOC_BEST_FIT);
	} else if (strcmp(cmdline, "buddy") == 0 && cons->sht != 0) {
		cmd_set_strategy(cons, ALLOC_BUDDY);
	} else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (strcmp(cmdline, "ps") == 0 && cons->sht != 0) {
		cmd_ps(cons);
	} else if (strcmp(cmdline, "sched") == 0 && cons->sht != 0) {
		cmd_sched_info(cons);
	} else if (strcmp(cmdline, "ipc") == 0 && cons->sht != 0) {
		cmd_ipc_info(cons);
	} else if (strncmp(cmdline, "priority ", 9) == 0) {
		cmd_set_priority(cons, cmdline);
	} else if (strncmp(cmdline, "sem ", 4) == 0) {
		cmd_semaphore(cons, cmdline);
	} else if (strncmp(cmdline, "msg ", 4) == 0) {
		cmd_message(cons, cmdline);
	} else if (strcmp(cmdline, "exit") == 0) {
		/* 清空光标 */
		if (cons->sht != 0) {
			boxfill8(cons->sht->buf, cons->sht->bxsize, COL8_000000,
				cons->cur_x, cons->cur_y, cons->cur_x + 7, cons->cur_y + 15);
			sheet_refresh(cons->sht, cons->cur_x, cons->cur_y, cons->cur_x + 8, cons->cur_y + 16);
		}
		/* 使用特殊标记，让主循环检测并关闭窗口 */
		if (cons->sht != 0) {
			cons->sht->flags |= 0x40;  /* 设置关闭标记 */
		}
		/* 让任务睡眠 */
		for (;;) {
			task_sleep(task_now());
		}
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

void cmd_mem(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[80];
	unsigned int total_free = memman_total(memman);
	
	sprintf(s, "=== Enhanced Memory Management ===\n");
	cons_putstr0(cons, s);
	sprintf(s, "Total Memory: %dMB\n", memtotal / (1024 * 1024));
	cons_putstr0(cons, s);
	sprintf(s, "Free Memory:  %dKB\n", total_free / 1024);
	cons_putstr0(cons, s);
	sprintf(s, "Used Memory:  %dKB\n", (memtotal - total_free) / 1024);
	cons_putstr0(cons, s);
	sprintf(s, "Free Blocks:  %d (max: %d)\n", memman->frees, memman->maxfrees);
	cons_putstr0(cons, s);
	sprintf(s, "Lost Blocks:  %d (size: %dKB)\n", memman->losts, memman->lostsize / 1024);
	cons_putstr0(cons, s);
	
	/* 显示当前分配策略 */
	char *strategy_name;
	switch (memman->strategy) {
		case ALLOC_FIRST_FIT:
			strategy_name = "First Fit";
			break;
		case ALLOC_BEST_FIT:
			strategy_name = "Best Fit";
			break;
		case ALLOC_BUDDY:
			strategy_name = "Buddy System";
			break;
		default:
			strategy_name = "Unknown";
			break;
	}
	sprintf(s, "Strategy:     %s\n\n", strategy_name);
	cons_putstr0(cons, s);
	
	return;
}

void cmd_cls(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 190; y++) {
		for (x = 8; x < 8 + 384; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 384, 28 + 190);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct CONSOLE *cons)
{
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	
	/* 取消定时器 */
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
		timer_free(cons->timer);
	}
	
	/* 释放FAT内存 */
	memman_free_4k(memman, (int) fat, 4 * 2880);
	
	/* 清空光标 */
	if (cons->sht != 0) {
		boxfill8(cons->sht->buf, cons->sht->bxsize, COL8_000000,
			cons->cur_x, cons->cur_y, cons->cur_x + 7, cons->cur_y + 15);
		sheet_refresh(cons->sht, cons->cur_x, cons->cur_y, cons->cur_x + 8, cons->cur_y + 16);
		
		/* 设置关闭标记，让主循环处理 */
		cons->sht->flags |= 0x40;
	}
	
	/* 让任务永远睡眠等待被清理 */
	for (;;) {
		task_sleep(task);
	}
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb;
	struct SHTCTL *shtctl;
	struct SHEET *sht;

	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; 

	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			set_segmdesc(task->ldt + 0, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1,      (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					sheet_free(sht);	
				}
			}
			for (i = 0; i < 8; i++) {	
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}
		memman_free_4k(memman, (int) p, finfo->size);
		cons_newline(cons);
		return 1;
	}
	return 0;
}

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1;	
		/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
		/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	if (edx == 1) {
		cons_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {
		cons_putstr0(cons, (char *) ebx + ds_base);
	} else if (edx == 3) {
		cons_putstr1(cons, (char *) ebx + ds_base, ecx);
	} else if (edx == 4) {
		return &(task->tss.esp0);
	} else if (edx == 5) {
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
		sheet_updown(sht, shtctl->top); 
		reg[7] = (int) sht;
	} else if (edx == 6) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
	} else if (edx == 7) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 8) {
		memman_init((struct MEMMAN *) (ebx + ds_base));
		ecx &= 0xfffffff0;	
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {
		ecx = (ecx + 0x0f) & 0xfffffff0; 
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {
		ecx = (ecx + 0x0f) & 0xfffffff0; 
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 11) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
	} else if (edx == 12) {
		sht = (struct SHEET *) ebx;
		sheet_refresh(sht, eax, ecx, esi, edi);
	} else if (edx == 13) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {
		sheet_free((struct SHEET *) ebx);
	} else if (edx == 15) {
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { 
				timer_init(cons->timer, &task->fifo, 1); 
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	
				cons->cur_c = -1;
			}
			if (i == 4) {	
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);	
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256) { 
				/* 将扫描码转换为ASCII字符 */
				int scancode = i - 256;
				if (scancode < 0x80) {
					char ch = keytable0[scancode];  /* 使用键盘表转换 */
					if (ch != 0) {
						reg[7] = ch;  /* 返回ASCII字符 */
					} else {
						reg[7] = scancode;  /* 如果没有对应ASCII，返回原始扫描码 */
					}
				} else {
					reg[7] = scancode;  /* 超出范围，返回原始扫描码 */
				}
				return 0;
			}
		}
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;	
	} else if (edx == 17) {
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {
		timer_settime((struct TIMER *) ebx, eax);
	} else if (edx == 19) {
		timer_free((struct TIMER *) ebx);
	} else if (edx == 20) {
		if (eax == 0) {
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}
	} else if (edx == 21) {
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i < 8) {
			finfo = file_search((char *) ebx + ds_base,
					(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				reg[7] = (int) fh;
				fh->buf = (char *) memman_alloc_4k(memman, finfo->size);
				fh->size = finfo->size;
				fh->pos = 0;
				file_loadfile(finfo->clustno, finfo->size, fh->buf, task->fat, (char *) (ADR_DISKIMG + 0x003e00));
			}
		}
	} else if (edx == 22) {
		fh = (struct FILEHANDLE *) eax;
		memman_free_4k(memman, (int) fh->buf, fh->size);
		fh->buf = 0;
	} else if (edx == 23) {
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			fh->pos = ebx;
		} else if (ecx == 1) {
			fh->pos += ebx;
		} else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	} else if (edx == 24) {
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			reg[7] = fh->size;
		} else if (ecx == 1) {
			reg[7] = fh->pos;
		} else if (ecx == 2) {
			reg[7] = fh->pos - fh->size;
		}
	} else if (edx == 25) {
		fh = (struct FILEHANDLE *) eax;
		for (i = 0; i < ecx; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
	} else if (edx == 26) {
		i = 0;
		for (;;) {
			*((char *) ebx + ds_base + i) =  task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= ecx) {
				break;
			}
			i++;
		}
		reg[7] = i;
	}
	return 0;
}

int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	
}

int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}

/* 内存管理策略设置命令 */
void cmd_set_strategy(struct CONSOLE *cons, alloc_strategy_t strategy)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[60];
	
	memman_set_strategy(memman, strategy);
	
	char *strategy_name;
	switch (strategy) {
		case ALLOC_FIRST_FIT:
			strategy_name = "First Fit";
			break;
		case ALLOC_BEST_FIT:
			strategy_name = "Best Fit";
			break;
		case ALLOC_BUDDY:
			strategy_name = "Buddy System";
			break;
		default:
			strategy_name = "Unknown";
			break;
	}
	
	sprintf(s, "Memory allocation strategy set to: %s\n\n", strategy_name);
	cons_putstr0(cons, s);
	return;
}

/* 内存管理性能测试命令 */
void cmd_memtest(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[80];
	unsigned int test_addrs[10];
	int i, success_count = 0;
	
	cons_putstr0(cons, "=== Memory Allocation Test ===\n");
	
	/* 测试多次分配和释放 */
	for (i = 0; i < 10; i++) {
		test_addrs[i] = memman_alloc(memman, 4096 * (i + 1));
		if (test_addrs[i] != 0) {
			success_count++;
			sprintf(s, "Alloc %d: 0x%08X (%dKB)\n", i + 1, test_addrs[i], 4 * (i + 1));
			cons_putstr0(cons, s);
		} else {
			sprintf(s, "Alloc %d: FAILED\n", i + 1);
			cons_putstr0(cons, s);
		}
	}
	
	sprintf(s, "Successful allocations: %d/10\n", success_count);
	cons_putstr0(cons, s);
	
	/* 释放所有成功分配的内存 */
	for (i = 0; i < 10; i++) {
		if (test_addrs[i] != 0) {
			memman_free(memman, test_addrs[i], 4096 * (i + 1));
		}
	}
	
	cons_putstr0(cons, "All test memory freed.\n\n");
	return;
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}

/* 新增的任务管理和IPC命令 */

void cmd_ps(struct CONSOLE *cons)
{
	/* 显示进程列表 */
	char s[60];
	int i;
	struct TASK *task;
	
	cons_putstr0(cons, "=== Process List ===\n");
	cons_putstr0(cons, "ID  State    Priority Level Time\n");
	
	for (i = 0; i < MAX_TASKS; i++) {
		task = &taskctl->tasks0[i];
		if (task->flags != 0) {
			char *state_str;
			switch (task->state) {
				case TASK_STATE_UNUSED: state_str = "UNUSED"; break;
				case TASK_STATE_READY: state_str = "READY "; break;
				case TASK_STATE_RUNNING: state_str = "RUN   "; break;
				case TASK_STATE_BLOCKED: state_str = "BLOCK "; break;
				case TASK_STATE_SLEEPING: state_str = "SLEEP "; break;
				case TASK_STATE_WAITING: state_str = "WAIT  "; break;
				case TASK_STATE_ZOMBIE: state_str = "ZOMBIE"; break;
				default: state_str = "UNK   "; break;
			}
			sprintf(s, "%2d  %s   %d        %d     %d\n", 
				task->task_id, state_str, task->priority, task->level, task->run_time);
			cons_putstr0(cons, s);
		}
	}
	cons_putchar(cons, '\n', 1);
}

void cmd_sched_info(struct CONSOLE *cons)
{
	/* 显示调度器信息 */
	char s[60];
	struct TASK *current = task_now();
	
	cons_putstr0(cons, "=== Scheduler Info ===\n");
	sprintf(s, "Current Task ID: %d\n", current->task_id);
	cons_putstr0(cons, s);
	sprintf(s, "Current Priority: %d\n", current->priority);
	cons_putstr0(cons, s);
	sprintf(s, "Time Slice: %d\n", current->time_slice);
	cons_putstr0(cons, s);
	sprintf(s, "Remaining Time: %d\n", current->remaining_time);
	cons_putstr0(cons, s);
	sprintf(s, "Current Level: %d\n", taskctl->now_lv);
	cons_putstr0(cons, s);
	cons_putchar(cons, '\n', 1);
}

void cmd_ipc_info(struct CONSOLE *cons)
{
	/* 显示IPC信息 */
	char s[60];
	int i, sem_count = 0, queue_count = 0;
	
	/* 统计活跃的信号量 */
	for (i = 0; i < MAX_SEMAPHORES; i++) {
		if (taskctl->semaphores[i].flags != 0) {
			sem_count++;
		}
	}
	
	/* 统计活跃的消息队列 */
	for (i = 0; i < MAX_MESSAGE_QUEUES; i++) {
		if (taskctl->message_queues[i].flags != 0) {
			queue_count++;
		}
	}
	
	cons_putstr0(cons, "=== IPC Info ===\n");
	sprintf(s, "Active Semaphores: %d/%d\n", sem_count, MAX_SEMAPHORES);
	cons_putstr0(cons, s);
	sprintf(s, "Active Message Queues: %d/%d\n", queue_count, MAX_MESSAGE_QUEUES);
	cons_putstr0(cons, s);
	
	/* 显示信号量详情 */
	if (sem_count > 0) {
		cons_putstr0(cons, "\nSemaphores:\n");
		cons_putstr0(cons, "Name            Value Waiting\n");
		for (i = 0; i < MAX_SEMAPHORES; i++) {
			if (taskctl->semaphores[i].flags != 0) {
				sprintf(s, "%-15s %5d %7d\n", 
					taskctl->semaphores[i].name,
					taskctl->semaphores[i].value,
					taskctl->semaphores[i].waiting_count);
				cons_putstr0(cons, s);
			}
		}
	}
	
	/* 显示消息队列详情 */
	if (queue_count > 0) {
		cons_putstr0(cons, "\nMessage Queues:\n");
		cons_putstr0(cons, "Name            Count  Max Waiting\n");
		for (i = 0; i < MAX_MESSAGE_QUEUES; i++) {
			if (taskctl->message_queues[i].flags != 0) {
				sprintf(s, "%-15s %5d %4d %7d\n", 
					taskctl->message_queues[i].name,
					taskctl->message_queues[i].message_count,
					taskctl->message_queues[i].max_messages,
					taskctl->message_queues[i].waiting_count);
				cons_putstr0(cons, s);
			}
		}
	}
	
	cons_putchar(cons, '\n', 1);
}

void cmd_set_priority(struct CONSOLE *cons, char *cmdline)
{
	/* 设置任务优先级: priority <task_id> <priority> */
	int task_id, priority;
	char s[60];
	
	if (parse_two_ints(cmdline, "priority %d %d", &task_id, &priority) == 2) {
		if (task_id >= 1 && task_id < taskctl->next_task_id && 
		    priority >= PRIORITY_HIGH && priority <= PRIORITY_IDLE) {
			int i;
			for (i = 0; i < MAX_TASKS; i++) {
				if (taskctl->tasks0[i].task_id == task_id && taskctl->tasks0[i].flags != 0) {
					task_set_priority(&taskctl->tasks0[i], priority);
					sprintf(s, "Task %d priority set to %d\n", task_id, priority);
					cons_putstr0(cons, s);
					return;
				}
			}
			cons_putstr0(cons, "Task not found\n");
		} else {
			sprintf(s, "Invalid task ID or priority (1-%d, %d-%d)\n", 
				taskctl->next_task_id - 1, PRIORITY_HIGH, PRIORITY_IDLE);
			cons_putstr0(cons, s);
		}
	} else {
		cons_putstr0(cons, "Usage: priority <task_id> <priority>\n");
	}
}

void cmd_semaphore(struct CONSOLE *cons, char *cmdline)
{
	/* 信号量操作: sem create <name> <value> | sem wait <name> | sem signal <name> */
	char operation[16], name[16];
	int value;
	char s[60];
	
	if (parse_command_words(cmdline, operation, name) >= 2) {
		if (strcmp(operation, "create") == 0) {
			/* 尝试解析值 */
			if (parse_word_int(cmdline + 4, name, &value) == 2) {
				struct SEMAPHORE *sem = semaphore_alloc(name, value);
				if (sem != 0) {
					sprintf(s, "Semaphore '%s' created with value %d\n", name, value);
					cons_putstr0(cons, s);
				} else {
					cons_putstr0(cons, "Failed to create semaphore\n");
				}
			} else {
				cons_putstr0(cons, "Usage: sem create <name> <value>\n");
			}
		} else if (strcmp(operation, "wait") == 0) {
			/* 查找信号量 */
			int i;
			for (i = 0; i < MAX_SEMAPHORES; i++) {
				if (taskctl->semaphores[i].flags != 0 && 
				    strcmp(taskctl->semaphores[i].name, name) == 0) {
					int result = semaphore_wait(&taskctl->semaphores[i]);
					sprintf(s, "Wait on '%s': %s\n", name, 
						result == 0 ? "acquired" : "failed");
					cons_putstr0(cons, s);
					return;
				}
			}
			cons_putstr0(cons, "Semaphore not found\n");
		} else if (strcmp(operation, "signal") == 0) {
			/* 查找信号量 */
			int i;
			for (i = 0; i < MAX_SEMAPHORES; i++) {
				if (taskctl->semaphores[i].flags != 0 && 
				    strcmp(taskctl->semaphores[i].name, name) == 0) {
					semaphore_signal(&taskctl->semaphores[i]);
					sprintf(s, "Signaled '%s'\n", name);
					cons_putstr0(cons, s);
					return;
				}
			}
			cons_putstr0(cons, "Semaphore not found\n");
		} else if (strcmp(operation, "destroy") == 0) {
			/* 查找并销毁信号量 */
			int i;
			for (i = 0; i < MAX_SEMAPHORES; i++) {
				if (taskctl->semaphores[i].flags != 0 && 
				    strcmp(taskctl->semaphores[i].name, name) == 0) {
					semaphore_free(&taskctl->semaphores[i]);
					sprintf(s, "Semaphore '%s' destroyed\n", name);
					cons_putstr0(cons, s);
					return;
				}
			}
			cons_putstr0(cons, "Semaphore not found\n");
		} else {
			cons_putstr0(cons, "Unknown operation. Available: create, wait, signal, destroy\n");
		}
	} else {
		cons_putstr0(cons, "Usage: sem create <name> <value>\n");
		cons_putstr0(cons, "       sem wait <name>\n");
		cons_putstr0(cons, "       sem signal <name>\n");
	}
}

void cmd_message(struct CONSOLE *cons, char *cmdline)
{
	/* 消息队列操作: msg create <name> <max> | msg send <name> <data> | msg recv <name> */
	char operation[16], name[16];
	int max_msg;
	char s[60];
	
	if (parse_command_words(cmdline, operation, name) >= 2) {
		if (strcmp(operation, "create") == 0) {
			/* 尝试解析最大消息数 */
			if (parse_word_int(cmdline + 4, name, &max_msg) == 2) {
				struct MESSAGE_QUEUE *queue = msgqueue_alloc(name, max_msg);
				if (queue != 0) {
					sprintf(s, "Message queue '%s' created (max: %d)\n", name, max_msg);
					cons_putstr0(cons, s);
				} else {
					cons_putstr0(cons, "Failed to create message queue\n");
				}
			} else {
				cons_putstr0(cons, "Usage: msg create <name> <max>\n");
			}
		} else if (strcmp(operation, "send") == 0) {
			cons_putstr0(cons, "Message send: simplified implementation\n");
		} else if (strcmp(operation, "recv") == 0) {
			cons_putstr0(cons, "Message receive: simplified implementation\n");
		} else if (strcmp(operation, "destroy") == 0) {
			/* 查找并销毁消息队列 */
			int i;
			for (i = 0; i < MAX_MESSAGE_QUEUES; i++) {
				if (taskctl->message_queues[i].flags != 0 && 
				    strcmp(taskctl->message_queues[i].name, name) == 0) {
					msgqueue_free(&taskctl->message_queues[i]);
					sprintf(s, "Message queue '%s' destroyed\n", name);
					cons_putstr0(cons, s);
					return;
				}
			}
			cons_putstr0(cons, "Message queue not found\n");
		} else {
			cons_putstr0(cons, "Unknown operation. Available: create, send, recv, destroy\n");
		}
	} else {
		cons_putstr0(cons, "Usage: msg create <name> <max>\n");
		cons_putstr0(cons, "       msg send <name> <data>\n");
		cons_putstr0(cons, "       msg recv <name>\n");
	}
}
