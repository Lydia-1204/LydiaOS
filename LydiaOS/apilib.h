/**
 * LydiaOS Application Programming Interface Library
 * 应用程序编程接口库
 * 
 * 为用户应用程序提供系统调用接口
 */

/* ===== 基本输出函数 ===== */
void api_putchar(int c);                    /* 输出单个字符 */
void api_putstr0(char *s);                  /* 输出以NULL结尾的字符串 */
void api_putstr1(char *s, int l);           /* 输出指定长度的字符串 */
void api_end(void);                         /* 结束应用程序 */

/* ===== 窗口管理函数 ===== */
int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);  /* 创建窗口 */
void api_putstrwin(int win, int x, int y, int col, int len, char *str);     /* 在窗口中输出字符串 */
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);       /* 在窗口中绘制填充矩形 */
void api_point(int win, int x, int y, int col);                             /* 在窗口中绘制点 */
void api_refreshwin(int win, int x0, int y0, int x1, int y1);               /* 刷新窗口指定区域 */
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);         /* 在窗口中绘制线条 */
void api_closewin(int win);                                                 /* 关闭窗口 */

/* ===== 内存管理函数 ===== */
void api_initmalloc(void);                  /* 初始化内存分配器 */
char *api_malloc(int size);                 /* 分配内存 */
void api_free(char *addr, int size);        /* 释放内存 */

/* ===== 输入处理函数 ===== */
int api_getkey(int mode);                   /* 获取键盘输入 */

/* ===== 定时器函数 ===== */
int api_alloctimer(void);                   /* 分配定时器 */
void api_inittimer(int timer, int data);    /* 初始化定时器 */
void api_settimer(int timer, int time);     /* 设置定时器时间 */
void api_freetimer(int timer);              /* 释放定时器 */

/* ===== 音频函数 ===== */
void api_beep(int tone);                    /* 发出蜂鸣声 */

/* ===== 文件操作函数 ===== */
int api_fopen(char *fname);                 /* 打开文件 */
void api_fclose(int fhandle);               /* 关闭文件 */
void api_fseek(int fhandle, int offset, int mode);  /* 设置文件指针位置 */
int api_fsize(int fhandle, int mode);       /* 获取文件大小 */
int api_fread(char *buf, int maxsize, int fhandle); /* 读取文件内容 */

/* ===== 系统信息函数 ===== */
int api_cmdline(char *buf, int maxsize);    /* 获取命令行参数 */
