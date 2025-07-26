/**
 * LydiaOS 内核主程序
 * Enhanced Operating System Kernel Main
 * 
 * 负责系统初始化、设备管理、窗口系统和多任务调度
 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

/* ===== 全局变量 ===== */
static struct SHEET *key_win = 0;       /* 当前键盘焦点窗口 */
static int console_creation_busy = 0;   /* 防止快速重复创建控制台 */

/* ===== 函数声明 ===== */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void console_task(struct SHEET *sheet, int memtotal);
void close_constask(struct TASK *task);
void close_console(struct SHEET *sht);
struct SHEET *create_new_console(struct SHTCTL *shtctl, int memtotal);

/**
 * 内核主函数 - 系统初始化和事件循环
 * 负责硬件初始化、内存管理、窗口系统和多任务管理
 */
void HariMain(void)
{
    /* ===== 变量声明 ===== */
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_win_b[3];
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_win_b;
    struct TIMER *timer;
    struct FIFO32 fifo, keycmd;
    int fifobuf[128], keycmd_buf[32];
    int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    unsigned char s[32];
    struct TASK *task_a, *task;
    int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
    int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
    struct SHEET *sht = 0, *sht2;
    int *fat;
    unsigned char *nihongo;

    /* ===== 系统初始化 ===== */
    /* 初始化GDT和IDT */
    init_gdtidt();
    /* 初始化可编程中断控制器 */
    init_pic();
    /* 开启中断 */
    io_sti(); 
    /* 初始化FIFO缓冲区 */
    fifo32_init(&fifo, 128, fifobuf, 0);
    /* 初始化可编程间隔定时器 */
    init_pit();
    /* 初始化键盘 */
    init_keyboard(&fifo, 256);
    /* 启用鼠标 */
    enable_mouse(&fifo, 512, &mdec);
    /* 设置中断屏蔽 */
    io_out8(PIC0_IMR, 0xf8); /* 允许PIT、PIC1和键盘中断 */
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标中断 */
    /* 初始化键盘命令FIFO */
    fifo32_init(&keycmd, 32, keycmd_buf, 0);

    /* ===== 内存管理初始化 ===== */
    /* 内存测试，获取总内存大小 */
    memtotal = memtest(0x00400000, 0xbfffffff);
    /* 初始化内存管理器 */
    memman_init(memman);
    /* 添加可用内存区域 */
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    /* 初始化伙伴系统，固定区间4MB~32MB */
    buddy_init(memman);

    /* ===== 内存管理功能测试 ===== */
    sprintf(s, "Total Memory: %dMB", memtotal / (1024 * 1024));
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 96, COL8_FFFFFF, s);
    
    /* 测试不同的内存分配策略 */
    unsigned int test_addr1, test_addr2, test_addr3;
    
    /* 测试首次适配算法 */
    memman_set_strategy(memman, ALLOC_FIRST_FIT);
    test_addr1 = memman_alloc(memman, 1024);
    sprintf(s, "First Fit: 0x%08X", test_addr1);
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 112, COL8_FFFFFF, s);
    
    /* 测试最佳适配算法 */
    memman_set_strategy(memman, ALLOC_BEST_FIT);
    test_addr2 = memman_alloc(memman, 1024);
    sprintf(s, "Best Fit: 0x%08X", test_addr2);
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 128, COL8_FFFFFF, s);
    
    /* 测试伙伴系统算法 */
    memman_set_strategy(memman, ALLOC_BUDDY);
    test_addr3 = memman_alloc(memman, 1024);
    sprintf(s, "Buddy System: 0x%08X", test_addr3);
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 144, COL8_FFFFFF, s);
    
    /* 释放测试内存 */
    if (test_addr1) memman_free(memman, test_addr1, 1024);
    if (test_addr2) memman_free(memman, test_addr2, 1024);
    if (test_addr3) memman_free(memman, test_addr3, 1024);
    
    /* 显示当前内存管理策略 */
    sprintf(s, "Memory Strategy: Best Fit");
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 160, COL8_FFFFFF, s);
    
    /* 设置为最佳适配策略 */
    memman_set_strategy(memman, ALLOC_BEST_FIT);

    init_palette();
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, PRIORITY_NORMAL);  /* 主任务使用正常优先级，平衡响应性和系统稳定性 */
    *((int *) 0x0fe4) = (int) shtctl;

    /* sht_back */
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 没有透明色 */
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    /* sht_cons */
    key_win = open_console(shtctl, memtotal);

    /* sht_mouse */
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    mx = (binfo->scrnx - 16) / 2; /* 画面中央坐标 */
    my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back, 0, 0);
    sheet_slide(key_win, (binfo->scrnx - 400) / 2, (binfo->scrny - 250) / 2);  /* 控制台窗口居中 */
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(key_win, 1);
    sheet_updown(sht_mouse, 2);
    keywin_on(key_win);  /* 激活控制台窗口，这会启动光标 */

    /* 为了避免和task_a的FIFO冲突，事先进行设置 */
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    for (;;) {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            /* 如果存在向键盘控制器发送的数据，则发送它 */
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            if (new_mx >= 0) {
                io_sti();
                sheet_slide(sht_mouse, new_mx, new_my);
                new_mx = -1;
            } else if (new_wx != 0x7fffffff) {
                io_sti();
                sheet_slide(sht, new_wx, new_wy);
                new_wx = 0x7fffffff;
            } else {
                task_sleep(task_a);
                io_sti();
            }
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            
            /* 检查是否有控制台窗口需要通过exit命令关闭 */
            int j;
            for (j = shtctl->top - 1; j >= 1; j--) {
                struct SHEET *check_sht = shtctl->sheets[j];
                if ((check_sht->flags & 0x40) != 0) { /* 检测exit标记 */
                    check_sht->flags &= ~0x40; /* 清除标记 */
                    close_console(check_sht);
                    break; /* 一次只处理一个 */
                }
            }
            
            if (key_win != 0 && key_win->flags == 0) { /* 输入窗口被关闭 */
                if (shtctl->top == 1) { /* 当没有窗口时 */
                    key_win = 0;
                } else {
                    /* 寻找有效的控制台窗口 */
                    key_win = 0;
                    int i;
                    for (i = shtctl->top - 1; i >= 1; i--) {
                        if (shtctl->sheets[i] != 0 && shtctl->sheets[i]->task != 0 && 
                            (shtctl->sheets[i]->flags & 0x20) != 0) { /* 确保是控制台窗口 */
                            key_win = shtctl->sheets[i];
                            keywin_on(key_win);
                            break;
                        }
                    }
                }
            }
            if (256 <= i && i <= 511) { /* 键盘数据 */
                if (i < 0x80 + 256) { /* 将按键编码转换为字符编码 */
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }
                if ('A' <= s[0] && s[0] <= 'Z') { /* 输入的是英文字母 */
                    if (((key_leds & 4) == 0 && key_shift == 0) ||
                        ((key_leds & 4) != 0 && key_shift != 0)) {
                        s[0] += 0x20; /* 将大写字母转换为小写字母 */
                    }
                }
                /* 处理特殊按键和一般字符 */
                if ((s[0] != 0 && key_win != 0) || 
                    (i == 256 + 0x0E && key_win != 0) ||  /* 退格键 */
                    (i == 256 + 0x1C && key_win != 0)) { /* 回车键 */
                    fifo32_put(&key_win->task->fifo, i - 256 + 256);  /* 发送原始扫描码 */
                }
                if (i == 256 + 0x0f && key_win != 0) {  /* Tab键 */
                    keywin_off(key_win);
                    j = key_win->height - 1;
                    if (j == 0) {
                        j = shtctl->top - 1;
                    }
                    key_win = shtctl->sheets[j];
                    keywin_on(key_win);
                }
                if (i == 256 + 0x3c) { /* F2键 - 创建新控制台 */
                    if (console_creation_busy == 0) {  /* 防止快速重复创建 */
                        console_creation_busy = 1;
                        /* 先关闭当前焦点窗口的光标 */
                        if (key_win != 0) {
                            keywin_off(key_win);
                        }
                        /* 创建新控制台 */
                        key_win = create_new_console(shtctl, memtotal);
                        if (key_win != 0) {
                            keywin_on(key_win);  /* 激活新窗口的光标 */
                        }
                        console_creation_busy = 0;
                    }
                }
                if (i == 256 + 0x2a) { /* 左Shift ON */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) { /* 右Shift ON */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) { /* 左Shift OFF */
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) { /* 右Shift OFF */
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a) { /* CapsLock */
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) { /* NumLock */
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) { /* ScrollLock */
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x3b && key_shift != 0 && key_win != 0) { /* Shift+F1 */
                    task = key_win->task;
                    if (task != 0 && task->tss.ss0 != 0) {
                        cons_putstr0(task->cons, "\nBreak(key) :\n");
                        io_cli(); /* 不能在改变寄存器值时切换到其他任务 */
                        task->tss.eax = (int) &(task->tss.esp0);
                        task->tss.eip = (int) asm_end_app;
                        io_sti();
                    }
                }
                if (i == 256 + 0x57 && shtctl->top > 2) { /* F11 */
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                }
                if (i == 256 + 0xfa) { /* 键盘成功接收到数据 */
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe) { /* 键盘没有成功接收到数据 */
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
            } else if (512 <= i && i <= 767) { /* 鼠标数据 */
                if (mouse_decode(&mdec, i - 512) != 0) {
                    /* 已搜集齐3字节的数据，移动光标 */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    new_mx = mx;
                    new_my = my;
                    if ((mdec.btn & 0x01) != 0) {
                        /* 按下左键、寻找鼠标所指向的图层 */
                        if (mmx < 0) {
                            /* 如果处于通常模式 */
                            for (j = shtctl->top - 1; j > 0; j--) {
                                sht = shtctl->sheets[j];
                                x = mx - sht->vx0;
                                y = my - sht->vy0;
                                if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        sheet_updown(sht, shtctl->top - 1);
                                        if (sht != key_win) {
                                            keywin_off(key_win);
                                            key_win = sht;
                                            keywin_on(key_win);
                                        }
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx = mx; /* 进入窗口移动模式 */
                                            mmy = my;
                                            mmx2 = sht->vx0;
                                            new_wy = sht->vy0;
                                        }
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                                            /* 点击"×"按钮 */
                                            if ((sht->flags & 0x10) != 0) { /* 是否为应用程序窗口？ */
                                                task = sht->task;
                                                cons_putstr0(task->cons, "\nBreak(mouse) :\n");
                                                io_cli(); /* 不能在改变寄存器值时切换到其他任务 */
                                                task->tss.eax = (int) &(task->tss.esp0);
                                                task->tss.eip = (int) asm_end_app;
                                                io_sti();
                                            } else if ((sht->flags & 0x20) != 0) { /* 控制台窗口 */
                                                /* 向控制台任务发送关闭信号，让任务自己执行exit流程 */
                                                task = sht->task;
                                                if (task != 0) {
                                                    fifo32_put(&task->fifo, 4); /* 发送关闭信号 */
                                                }
                                            } else { /* 其他类型窗口 */
                                                task = sht->task;
                                                sheet_updown(sht, -1); /* 暂且隐藏 */
                                                keywin_off(key_win);
                                                key_win = shtctl->sheets[shtctl->top - 1];
                                                keywin_on(key_win);
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            /* 如果处于窗口移动模式 */
                            x = mx - mmx; /* 计算鼠标的移动距离 */
                            y = my - mmy;
                            new_wx = (mmx2 + x + 2) & ~3; /* 适度修正 */
                            new_wy = new_wy + y;
                            mmy = my; /* 更新为移动后的坐标 */
                        }
                    } else {
                        /* 没有按下左键 */
                        mmx = -1; /* 返回通常模式 */
                        if (new_wx != 0x7fffffff) {
                            sheet_slide(sht, new_wx, new_wy); /* 确定移动 */
                            new_wx = 0x7fffffff;
                        }
                    }
                }
            } else if (768 <= i && i <= 1023) { /* 控制台终止处理 */
                close_console(shtctl->sheets0 + (i - 768));
            } else if (1024 <= i && i <= 2023) { /* 关闭控制台窗口 */
                close_constask(taskctl->tasks0 + (i - 1024));
            } else if (2024 <= i && i <= 2279) { /* 只关闭应用程序窗口 */
                sht2 = shtctl->sheets0 + (i - 2024);
                memman_free_4k(memman, (int) sht2->buf, 400 * 250);
                sheet_free(sht2);
            }
        }
    }
}

/* 窗口输入控制函数 */
void keywin_off(struct SHEET *key_win)
{
    change_wtitle8(key_win, 0);
    if ((key_win->flags & 0x20) != 0) { /* 如果是控制台窗口 */
        fifo32_put(&key_win->task->fifo, 3); /* 发送光标隐藏信号 */
    }
}

void keywin_on(struct SHEET *key_win)
{
    change_wtitle8(key_win, 1);
    if ((key_win->flags & 0x20) != 0) { /* 如果是控制台窗口 */
        fifo32_put(&key_win->task->fifo, 2); /* 发送光标显示信号 */
    }
}

/* 控制台相关函数 */
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHEET *sht = sheet_alloc(shtctl);
    unsigned char *buf;
    
    /* 检查图层分配是否成功 */
    if (sht == 0) {
        return 0;
    }
    
    /* 分配窗口缓存内存并检查 */
    buf = (unsigned char *) memman_alloc_4k(memman, 400 * 250);
    if (buf == 0) {
        sheet_free(sht);
        return 0;
    }
    
    sheet_setbuf(sht, buf, 400, 250, -1); /* 没有透明色 */
    make_window8(buf, 400, 250, "console", 0);
    make_textbox8(sht, 8, 28, 384, 190, COL8_000000);
    
    /* 创建控制台任务并检查 */
    sht->task = open_constask(sht, memtotal);
    if (sht->task == 0) {
        /* 任务创建失败，清理资源 */
        memman_free_4k(memman, (int) buf, 400 * 250);
        sheet_free(sht);
        return 0;
    }
    
    sht->flags |= 0x20;    /* 有光标 */
    return sht;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_alloc();
    int *cons_fifo;
    
    /* 检查任务分配是否成功 */
    if (task == 0) {
        return 0;
    }
    
    /* 分配FIFO缓存并检查 */
    cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
    if (cons_fifo == 0) {
        task->flags = 0; /* 释放任务 */
        return 0;
    }
    
    /* 分配堆栈内存并检查 */
    task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
    if (task->cons_stack == 0) {
        memman_free_4k(memman, (int) cons_fifo, 128 * 4);
        task->flags = 0; /* 释放任务 */
        return 0;
    }
    
    task->tss.esp = task->cons_stack + 64 * 1024 - 12;
    task->tss.eip = (int) &console_task;
    task->tss.es = 1 * 8;
    task->tss.cs = 2 * 8;
    task->tss.ss = 1 * 8;
    task->tss.ds = 1 * 8;
    task->tss.fs = 1 * 8;
    task->tss.gs = 1 * 8;
    *((int *) (task->tss.esp + 4)) = (int) sht;
    *((int *) (task->tss.esp + 8)) = memtotal;
    
    /* 初始化FIFO */
    fifo32_init(&task->fifo, 128, cons_fifo, task);
    
    /* 启动任务 */
    task_run(task, 2, PRIORITY_NORMAL); /* level=2, priority=PRIORITY_NORMAL - 避免与主任务冲突 */
    
    return task;
}

void close_constask(struct TASK *task)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    task_sleep(task);
    memman_free_4k(memman, task->cons_stack, 64 * 1024);
    memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
    task->flags = 0; /* task_free(task); */
}

void close_console(struct SHEET *sht)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = sht->task;
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    
    /* 如果关闭的是当前键盘焦点窗口，需要切换焦点 */
    if (key_win == sht) {
        keywin_off(key_win);
        /* 寻找下一个可用的控制台窗口 */
        key_win = 0;
        if (shtctl->top > 1) {
            int i;
            for (i = shtctl->top - 1; i >= 1; i--) {
                if (shtctl->sheets[i] != sht && shtctl->sheets[i]->task != 0 && 
                    (shtctl->sheets[i]->flags & 0x20) != 0) { /* 确保是控制台窗口 */
                    key_win = shtctl->sheets[i];
                    break;
                }
            }
        }
        if (key_win != 0) {
            keywin_on(key_win);
        }
    }
    
    /* 关闭任务 */
    if (task != 0) {
        close_constask(task);
    }
    
    /* 释放窗口缓存和图层 */
    memman_free_4k(memman, (int) sht->buf, 400 * 250);
    sheet_free(sht);
}

struct SHEET *create_new_console(struct SHTCTL *shtctl, int memtotal)
{
    struct SHEET *sht;
    int x = 32, y = 4;
    int console_count = 0;
    int i;
    
    /* 计算当前控制台窗口数量 */
    for (i = 1; i < shtctl->top; i++) {
        if (shtctl->sheets[i] != 0 && (shtctl->sheets[i]->flags & 0x20) != 0) {
            console_count++;
        }
    }
    
    /* 限制最大控制台数量，避免资源耗尽 */
    if (console_count >= 8) {
        return 0;  /* 超过最大数量，拒绝创建 */
    }
    
    /* 创建新控制台 */
    sht = open_console(shtctl, memtotal);
    if (sht != 0) {
        /* 基于控制台数量调整位置，避免重叠和超出屏幕 */
        x = 32 + (console_count % 8) * 32;  /* 限制水平偏移 */
        y = 4 + (console_count % 6) * 24;   /* 限制垂直偏移 */
        
        /* 确保窗口在屏幕范围内 */
        if (x + 400 > 640) x = 32;  /* 窗口宽度约400 */
        if (y + 250 > 480) y = 4;   /* 窗口高度约250 */
        
        sheet_slide(sht, x, y);
        sheet_updown(sht, shtctl->top);
    }
    return sht;
}
