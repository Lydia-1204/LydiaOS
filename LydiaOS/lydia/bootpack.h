/**
 * LydiaOS 内核头文件
 * Enhanced Operating System Headers
 * 
 * 包含系统核心数据结构、函数声明和常量定义
 */
#ifndef BOOTPACK_H
#define BOOTPACK_H

#include <stdarg.h>

/* ===== 系统启动信息结构 ===== */
struct BOOTINFO { /* 0x0ff0-0x0fff 启动信息结构体 */
	char cyls;      /* 柱面数 */
	char leds;      /* 键盘LED状态 */
	char vmode;     /* 显示模式 */
	char reserve;   /* 保留字节 */
	short scrnx, scrny;  /* 屏幕分辨率 */
	char *vram;     /* 显存地址 */
};
#define ADR_BOOTINFO	0x00000ff0  /* 启动信息地址 */
#define ADR_DISKIMG		0x00100000  /* 磁盘镜像地址 */

/* ===== 核心函数声明 ===== */
void HariMain(void);  /* 内核主函数 */

/* ===== 底层汇编函数接口 (naskfunc.nas) ===== */

/* CPU控制函数 */
void io_hlt(void);          /* CPU休眠等待中断 */
void io_cli(void);          /* 禁用中断 */
void io_sti(void);          /* 启用中断 */
void io_stihlt(void);       /* 启用中断并休眠 */

/* 端口I/O函数 */
int io_in8(int port);                    /* 读取8位端口数据 */
void io_out8(int port, int data);       /* 写入8位端口数据 */

/* 系统寄存器操作 */
int io_load_eflags(void);               /* 读取标志寄存器 */
void io_store_eflags(int eflags);       /* 设置标志寄存器 */
void load_gdtr(int limit, int addr);    /* 加载全局描述符表寄存器 */
void load_idtr(int limit, int addr);    /* 加载中断描述符表寄存器 */
int load_cr0(void);                     /* 读取控制寄存器CR0 */
void store_cr0(int cr0);                /* 设置控制寄存器CR0 */
void load_tr(int tr);                   /* 加载任务寄存器 */

/* 中断处理程序包装器 */
void asm_inthandler0c(void);            /* 栈段错误异常处理 */
void asm_inthandler0d(void);            /* 一般保护错误异常处理 */
void asm_inthandler20(void);            /* 定时器中断处理(IRQ0) */
void asm_inthandler21(void);            /* 键盘中断处理(IRQ1) */
void asm_inthandler2c(void);            /* 鼠标中断处理(IRQ12) */

/* 内存和应用程序管理 */
unsigned int memtest_sub(unsigned int start, unsigned int end);  /* 内存测试子程序 */
void farjmp(int eip, int cs);           /* 远程跳转 */
void farcall(int eip, int cs);          /* 远程调用 */
void asm_hrb_api(void);                 /* HRB应用程序API调用包装器 */
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);  /* 启动用户应用程序 */
void asm_end_app(void);                 /* 应用程序结束处理 */

/* fifo.c */
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

/* dsctbl.c */
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT			0x0082
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

/* mouse.c */
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c - Enhanced Memory Management */
#define MEMMAN_FREES		4090	
#define MEMMAN_ADDR			0x003c0000
#define PAGE_SIZE			4096		/* 4KB页面大小 */
#define MAX_PAGES			0x100000	/* 最大页面数 (4GB / 4KB) */
#define BUDDY_MAX_ORDER		10			/* 伙伴系统最大阶数 (2^10 = 1024页) */
#define VIRT_BASE			0x80000000	/* 虚拟内存起始地址 */
// 固定伙伴系统管理区间4MB~32MB
#define MEM_BUDDY_START   0x00400000  /* 4MB，通常内核和BIOS保留区之后 */
#define MEM_BUDDY_END     0x02000000  /* 32MB，实际可用物理内存末尾 */

/* 内存分配策略 */
typedef enum {
	ALLOC_FIRST_FIT,	/* 首次适配 */
	ALLOC_BEST_FIT,		/* 最佳适配 */
	ALLOC_BUDDY			/* 伙伴系统 */
} alloc_strategy_t;

/* 页面状态 */
typedef enum {
	PAGE_FREE,		/* 空闲页面 */
	PAGE_USED,		/* 已使用页面 */
	PAGE_RESERVED	/* 保留页面 */
} page_state_t;

/* 页面描述符 */
struct PAGE_DESC {
	unsigned int phys_addr;		/* 物理地址 */
	unsigned int virt_addr;		/* 虚拟地址 */
	page_state_t state;			/* 页面状态 */
	int ref_count;				/* 引用计数 */
	struct PAGE_DESC *next;		/* 链表指针 */
};

/* 伙伴系统块 */
struct BUDDY_BLOCK {
	unsigned int addr;			/* 块起始地址 */
	int order;					/* 块大小的阶数 (2^order页) */
	struct BUDDY_BLOCK *next;	/* 下一个同阶块 */
};

/* 传统内存块信息 */
struct FREEINFO {
	unsigned int addr, size;
};

/* 增强内存管理器 */
struct MEMMAN {		
	/* 传统链表管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
	
	/* 页面管理 */
	struct PAGE_DESC *page_table;		/* 页面表 */
	unsigned int total_pages;			/* 总页面数 */
	unsigned int free_pages;			/* 空闲页面数 */
	struct PAGE_DESC *free_list;		/* 空闲页面链表 */
	
	/* 伙伴系统 */
	struct BUDDY_BLOCK *buddy_free[BUDDY_MAX_ORDER + 1];  /* 各阶空闲块链表 */
	unsigned int buddy_bitmap[(MAX_PAGES / 32) + 1];	  /* 伙伴位图 */
	
	/* 虚拟内存 */
	unsigned int *page_dir;				/* 页目录 */
	unsigned int virt_next;				/* 下一个可用虚拟地址 */
	
	/* 配置 */
	alloc_strategy_t strategy;			/* 分配策略 */
};

/* 内存管理函数 */
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
void memman_set_strategy(struct MEMMAN *man, alloc_strategy_t strategy);

/* 传统接口 (向后兼容) */
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* 页面管理接口 */
void page_init(struct MEMMAN *man, unsigned int start_addr, unsigned int end_addr);
struct PAGE_DESC *page_alloc(struct MEMMAN *man);
void page_free(struct MEMMAN *man, struct PAGE_DESC *page);
unsigned int page_to_addr(struct PAGE_DESC *page);
struct PAGE_DESC *addr_to_page(struct MEMMAN *man, unsigned int addr);

/* 伙伴系统接口 */
void buddy_init(struct MEMMAN *man);
unsigned int buddy_alloc(struct MEMMAN *man, int order);
void buddy_free(struct MEMMAN *man, unsigned int addr, int order);
int buddy_get_order(unsigned int size);

/* 虚拟内存接口 */
void vmem_init(struct MEMMAN *man);
unsigned int vmem_alloc(struct MEMMAN *man, unsigned int size);
void vmem_free(struct MEMMAN *man, unsigned int virt_addr, unsigned int size);
int vmem_map(struct MEMMAN *man, unsigned int virt_addr, unsigned int phys_addr);
void vmem_unmap(struct MEMMAN *man, unsigned int virt_addr);

/* 最佳适配算法 */
unsigned int bestfit_alloc(struct MEMMAN *man, unsigned int size);
void bestfit_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS		256
struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL *ctl;
	struct TASK *task;
};
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER		500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout;
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);

/* mtask.c */
#define MAX_TASKS		1000	
#define TASK_GDT0		3	
#define MAX_TASKS_LV	100
#define MAX_TASKLEVELS	10

/* 任务状态定义 */
#define TASK_STATE_UNUSED	0	/* 未使用 */
#define TASK_STATE_READY	1	/* 就绪 */
#define TASK_STATE_RUNNING	2	/* 运行中 */
#define TASK_STATE_BLOCKED	3	/* 阻塞 */
#define TASK_STATE_SLEEPING	4	/* 睡眠 */
#define TASK_STATE_WAITING	5	/* 等待 */
#define TASK_STATE_ZOMBIE	6	/* 僵尸 */

/* 任务优先级定义 */
#define PRIORITY_HIGH		1	/* 高优先级 */
#define PRIORITY_NORMAL		5	/* 正常优先级 */
#define PRIORITY_LOW		9	/* 低优先级 */
#define PRIORITY_IDLE		10	/* 空闲优先级 */

/* IPC 定义 */
#define MAX_SEMAPHORES		100
#define MAX_MESSAGE_QUEUES	50
#define MAX_MESSAGES		1000
#define MESSAGE_SIZE		256

struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};

/* 信号量结构 */
struct SEMAPHORE {
	int value;
	int flags;
	struct TASK *waiting_tasks[MAX_TASKS_LV];
	int waiting_count;
	char name[16];
};

/* 消息结构 */
struct MESSAGE {
	int sender_id;
	int type;
	char data[MESSAGE_SIZE];
	int size;
	struct MESSAGE *next;
};

/* 消息队列结构 */
struct MESSAGE_QUEUE {
	int flags;
	int max_messages;
	int message_count;
	struct MESSAGE *head;
	struct MESSAGE *tail;
	struct TASK *waiting_tasks[MAX_TASKS_LV];
	int waiting_count;
	char name[16];
};

struct TASK {
	int sel, flags, state;
	int level, priority, base_priority;
	int time_slice, remaining_time;
	int task_id;
	struct FIFO32 fifo;
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[2];
	struct CONSOLE *cons;
	int ds_base, cons_stack;
	struct FILEHANDLE *fhandle;
	int *fat;
	char *cmdline;
	
	/* 等待相关 */
	struct SEMAPHORE *waiting_semaphore;
	struct MESSAGE_QUEUE *waiting_msgqueue;
	int wait_result;
	
	/* 调度统计 */
	unsigned int run_time;
	unsigned int sleep_time;
	unsigned int created_time;
};

struct TASKLEVEL {
	int running;
	int now;
	struct TASK *tasks[MAX_TASKS_LV];
};

struct TASKCTL {
	int now_lv;
	char lv_change; 
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
	int next_task_id;
	
	/* IPC管理 */
	struct SEMAPHORE semaphores[MAX_SEMAPHORES];
	struct MESSAGE_QUEUE message_queues[MAX_MESSAGE_QUEUES];
	struct MESSAGE messages[MAX_MESSAGES];
	int next_message_id;
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
void task_set_priority(struct TASK *task, int priority);
void task_wake_up(struct TASK *task);
void task_block(struct TASK *task);
void task_unblock(struct TASK *task);
int task_get_state(struct TASK *task);
void task_set_state(struct TASK *task, int state);

/* IPC 函数 */
struct SEMAPHORE *semaphore_alloc(char *name, int initial_value);
void semaphore_free(struct SEMAPHORE *sem);
int semaphore_wait(struct SEMAPHORE *sem);
int semaphore_signal(struct SEMAPHORE *sem);
struct MESSAGE_QUEUE *msgqueue_alloc(char *name, int max_messages);
void msgqueue_free(struct MESSAGE_QUEUE *queue);
int msgqueue_send(struct MESSAGE_QUEUE *queue, int type, char *data, int size);
int msgqueue_receive(struct MESSAGE_QUEUE *queue, int *type, char *data, int *size);

/* 调度器函数 */
void scheduler_init(void);
struct TASK *scheduler_select_next(void);
void scheduler_update_priorities(void);

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);

/* console.c */
struct CONSOLE {
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
	struct TIMER *timer;
};
struct FILEHANDLE {
	char *buf;
	int size;
	int pos;
};
void console_task(struct SHEET *sheet, int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_memtest(struct CONSOLE *cons, int memtotal);
void cmd_set_strategy(struct CONSOLE *cons, alloc_strategy_t strategy);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_exit(struct CONSOLE *cons, int *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);
void cmd_ps(struct CONSOLE *cons);
void cmd_sched_info(struct CONSOLE *cons);
void cmd_ipc_info(struct CONSOLE *cons);
void cmd_set_priority(struct CONSOLE *cons, char *cmdline);
void cmd_semaphore(struct CONSOLE *cons, char *cmdline);
void cmd_message(struct CONSOLE *cons, char *cmdline);

/* file.c */
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);
void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

/* 全局变量声明 */
extern struct MEMMAN *memman;
extern char keytable0[0x80];
extern char keytable1[0x80];

/* 键盘命令常量 */
#define KEYCMD_LED		0xed

#endif /* BOOTPACK_H */
