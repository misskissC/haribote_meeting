/* struct BOOTINFO,
 * 描述asmhead.nas在[0xff0,0xffc)内存段所存储显卡信息的结构体类型。*/
struct BOOTINFO {
    char cyls;          /* haribote os所占柱面数 */
    char leds;          /* 键盘状态标志 */
    char vmode;         /* 色号位数 */
    char reserve;       /* 用于填充色号位数的高字节 */
    short scrnx, scrny; /* x,y分辨率(像素点数) */
    char *vram;         /* 显存起始地址 */
};
/* 见asmhead.nas,
 * 0xff0为显存相关参数所在内存基址;0x100000为软盘映像内存基址。*/
#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG  0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);

/* fifo.c */
/* struct FIFO32,
 * 循环队列结构体类型。*/
struct FIFO32 {
    int *buf; /* 所指内存段为队列数据缓冲区 */
/* p,buf队尾数据后一个位置(索引),用于向队尾写入数据;
 * q,buf队头位置,用于读队头数据;
 * size,buf队列大小;free,队列空闲大小;
 * flags,队列状态标志,0-还有空闲;1-队列满。*/
    int p, q, size, free, flags;
/* task,循环队列所关联的任务(拥有该循环队列的任务) */
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

/* 调色板前16个调色板号,各调色板
 * 号对应RGB颜色见init_palette中的table_rgb数组。*/
#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15


/* dsctbl.c */
/* struct SEGMENT_DESCRIPTOR,
 * 描述GDT段描述符的结构体类型。
 * GDT段描述符位格式参见x86CPU手册。*/
struct SEGMENT_DESCRIPTOR {
/* limit_high&&limit_low,
 * 所描述内存段基于其基址的最大偏移(内存段长度);
 * base_high&&base_mid&&base_low,所描述内存段的基址;
 * access_right,段描述符有效位,特权级(DPL),类型(TYPE)等属性。*/
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
/* struct GATE_DESCRIPTOR,
 * 描述IDT描述符的结构体类型。
 * IDT描述符位格式参见x86CPU手册。*/
struct GATE_DESCRIPTOR {
/* offset_high&&offset_low,
 * 中断或异常处理程序在其所在内存段中的偏移;
 * selector,处理程序所在内存段的段选择符;
 * dw_count,保留未用;
 * access_right,IDT描述符有效位,特权级(DPL),类型(TYPE)等属性。*/
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT         0x0026f800  /* IDT内存段基址 */
#define LIMIT_IDT       0x000007ff  /* IDT限长 */
#define ADR_GDT         0x00270000  /* GDT内存段基址 */
#define LIMIT_GDT       0x0000ffff  /* GDT限长 */
#define ADR_BOTPAK      0x00280000  /* haribote os C部分内存地址起始地址, 见asmhead.nas */
#define LIMIT_BOTPAK    0x0007ffff  /* haribote os C部分大小, 64Kb */
#define AR_DATA32_RW    0x4092  /* 用于设定GDT段描述符所描述内存段为可读写数据内存段,DPL=0 */
#define AR_CODE32_ER    0x409a  /* 用于设定GDT段描述符所描述内存段为可读可执行内存段,DPL=0 */
#define AR_LDT          0x0082  /* 用于表征GDT段描述符所描述内存段为LDT */
#define AR_TSS32        0x0089  /* 用于表征GDT段描述符所描述内存段为TSS */
#define AR_INTGATE32    0x008e  /* 用于设定IDT描述符为中断门描述符,DPL=0 */

/* int.c */
void init_pic(void);
/* 8259A-1提供20h和21h两个端口地址供编程配置,
 * 8259A-2提供a0h和a1两个端口地址供编程配置。
 *
 * 需先通过8259A端口地址下发ICW初始化命令字组,
 * 然后才能通过8259A端口地址下发OCW操作命令字组,
 * 同一个端口地址在不同阶段配置不同的功能。*/
/* 在8259A正确的配置顺序下,
 * 以下端口地址的在不同阶段的作用分别是, */
#define PIC0_ICW1 0x0020 /* 写8259A-1 ICW1的端口地址 */
#define PIC0_OCW2 0x0020 /* 写8259A-1 OCW2的端口地址 */
#define PIC0_IMR  0x0021 /* 写8259A-1 OCW1的端口地址 */
#define PIC0_ICW2 0x0021 /* 写8259A-1 ICW2的端口地址 */
#define PIC0_ICW3 0x0021 /* 写8259A-1 ICW3的端口地址 */
#define PIC0_ICW4 0x0021 /* 写8259A-1 ICW4的端口地址 */
#define PIC1_ICW1 0x00a0 /* 写8259A-2 ICW1的端口地址 */
#define PIC1_OCW2 0x00a0 /* 写8259A-2 OCW2的端口地址 */
#define PIC1_IMR  0x00a1 /* 写8259A-2 OCW1的端口地址 */
#define PIC1_ICW2 0x00a1 /* 写8259A-2 ICW2的端口地址 */
#define PIC1_ICW3 0x00a1 /* 写8259A-2 ICW3的端口地址 */
#define PIC1_ICW4 0x00a1 /* 写8259A-2 ICW4的端口地址 */

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
/* 8042 端口地址,
 * in  al, 60h, 读8042输出缓冲寄存器内容到al;
 * in  al, 64h, 读8042状态寄存器内容到al;
 * out 60h, al, 写数据al到8042输入缓冲寄存器;
 * out 64h, al, 写命令al到8042输入缓冲寄存器. */
#define PORT_KEYDAT 0x0060
#define PORT_KEYCMD 0x0064

/* mouse.c */
/* struct MOUSE_DEC,
 * 用于存储鼠标输入数据及其解析结果的结构体类型。*/
struct MOUSE_DEC {
/* buf,用于保存鼠标的一组数据,共3字节;
 * phase,标识一组鼠标数据已输入多少字节;
 * 
 * 后面3个成员用于存储buf中鼠标数据的解析结果。
 * x和y,分别存储鼠标左右和上下移动的位移量;
 * btn,存储鼠标点击事件。*/
    unsigned char buf[3], phase;
    int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c */
/* MEMMAN_FREES,内存管理中空闲且不连续内存块的最大数目,
 * MEMMAN_ADDR,内存管理结构体首地址。*/
#define MEMMAN_FREES 4090
#define MEMMAN_ADDR  0x003c0000

/* struct FREEINFO,
 * 用于记录一块空闲内存块的结构体类型。*/
struct FREEINFO {
    /* 内存块首地址及大小 */
    unsigned int addr, size;
};

/* struct MEMMAN,
 * 用于内存管理的结构体类型。*/
struct MEMMAN {
/* frees,当前内存空闲块数;
 * maxfrees,记录空闲内存块曾出现过的最大次数(记录最严重的碎片化);
 * lostsize,内存释放失败总大小;
 * losts,内存释放失败总次数。*/
    int frees, maxfrees, lostsize, losts;
/* 按照内存地址升序记录空闲内存块的结构体数组 */
    struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS 256 /* haribote支持管理的窗口数 */
/* struct SHEET,
 * 管理一个窗口画面信息(调色板色号)的结构体类型。*/
    struct SHEET {
/* buf所指内存段用于缓存窗口的画面信息;
 * bxsize,窗口x方向像素点数;
 * bysize,窗口y方向像素点数。*/
    unsigned char *buf;
/* (vx0,vy0),窗口画面信息在屏幕上显示的起始位置;
 * col_inv,标识窗口画面信息中是否包含透明色(-1表不含透明色号);
 * height,窗口画面信息在屏幕上的图层高度;
 * flags,标识本结构体管理管理窗口画面信息属性,
 * 0,未用;1,已使用;bit[5]=1,命令行窗口含光标;bit[4]=1,应用程序任务窗口;
 * ctl,指向管理屏幕所有窗口显示的结构体;
 * task,窗口所关联任务管理结构体(拥有该窗口的任务)。*/
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    struct SHTCTL *ctl;
    struct TASK *task;
};
/* struct SHTCTL,
 * 管理屏幕画面(窗口)显示的结构体类型。*/
struct SHTCTL {
/* vram,指向显存基址;
 * map所指内存与显存大小相同,用于记录屏幕所显示画面所在图层。*/
    unsigned char *vram, *map;
/* xsize,屏幕x方向像素点数;
 * ysize,屏幕y方向像素点数;
 * top,屏幕顶层窗口图层高度。*/
    int xsize, ysize, top;
/* sheets, 以窗口图层高度的升序顺序依次指向sheets0中的元素;
 * sheets0,管理屏幕窗口画面信息的结构体数组,每个数组元素对应管理一个窗口的画面信息。*/
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
/* haribote os所支持定时器最大数 */
#define MAX_TIMER 500

/* struct TIMER,
 * 管理定时器的结构体类型。*/
struct TIMER {
/* 指向下一个定时器,下一个定时器是
 * 超时值刚好大于(或等于)自身超时值的定时器。*/
    struct TIMER *next;
    
/* 定时器超时值(以10ms为单位) */
    unsigned int timeout;

/* flags=0/1/2,本定时器未使用/已分配/正使用;
 * flags2=1,定时器自动取消。*/
    char flags, flags2;

/* 定时器超时队列,
 * 用于缓存定时器超时后的超时信号data。
 *
 * 任务定时器超时后,则可直接执行任务切换;
 * 其他类型定时器超时后,则将相应的超时信号
 * data写入fifo指向的缓存队列中,当其他任务
 * 从该队列中读出data时,便知道相应的定时器超时了。*/
    struct FIFO32 *fifo;
    int data;
};
/* struct TIMERCTL,
 * 定时器管理结构体类型。*/
struct TIMERCTL {
/* count,当前系统已计时值(单位为10ms);
 * next,即将超时定时器的超时值(单位10ms)。*/
    unsigned int count, next;

/* 指向timers0内存段中超时值最小的定时器 */
    struct TIMER *t0;

/* 定时器内存空间 */
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
/* task - 任务,即正在运行的进程(或线程等),此处指进程 */
#define MAX_TASKS       1000 /* haribote能同时运行的总任务数 */
#define TASK_GDT0       3    /* GDT中首个TSS表项索引 */
#define MAX_TASKS_LV    100  /* 各任务层中所允许任务的最大数量 */
#define MAX_TASKLEVELS  10   /* 任务层级数 */
/* struct TSS32,
 * 描述x86 32位 TSS内存位格式(共104字节)结构体类型。
 * TSS用于CPU切换任务时备份原任务运行上下文。*/
struct TSS32 {
/* backlink,前一任务TSS选择符;
 * ss*:esp*,用于维护0-2特权级程序的栈内存;
 * cr3,备份当前任务cr3寄存器值,即页目录基址; */
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
/* 用于备份本任务所使用的各寄存器当前值 */
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
/* ldtr,本任务LDT的选择符;
 * iomap,I/O许可位图偏移地址(相对于本TSS起始处) */
    int ldtr, iomap;
/* 粗略理解I/O许可位图。
 * 当前任务在访问I/O端口时,
 * 当CPL<=EFLAGS.IOPL时,当前任务可访问所有I/O端口;
 * 当CPL>EFLAGS.IOPL时,当前任务需检查I/O许可位图,
 * 若端口在I/O许可位图中对应bit位为0则可访问,否则
 * 不可访问。I/O许可位图中的0-65535位对应端口号0-6553. */
};
/* struct TASK,
 * 管理1个(应用程序和内核)任务的结构体类型。*/
struct TASK {
/* sel,任务TSS选择符;
 * flags,2-任务处于运行状态(结构体位于任务层),1-任务处于休眠状态(结构体不在任务层中);
 * level,任务所处任务层级(数字越小表任务层被调度优先级越高,任务层级0的优先级最高);
 * priority,任务优先级(运行时间片); */
    int sel, flags;
    int level, priority;
/* fifo,任务循环队列;
 * tss,任务TSS;
 * ldt,应用程序任务LDT,ldt[0]-应用程序任务代码段描述符,ldt[1]-应用程序任务数据段描述符;
 * cons,指向本任务关联的命令行窗口(控制终端)的结构体指针; */
    struct FIFO32 fifo;
    struct TSS32 tss;
    struct SEGMENT_DESCRIPTOR ldt[2];
    struct CONSOLE *cons;
/* ds_base,任务数据段起始地址(对于应用程序,该成员指向ldt[1]所描述内存段起始地址);
 * cons_stack,任务栈内存起始地址(对于内核态任务,栈内存为64Kb;对于应用程序任务,栈为数据段一部分);
 * fhandle,管理任务所打开文件的结构体指针;
 * fat,指向(解压缩后的)FAT;
 * cmdline,指向任务所关联命令行窗口的(命令行)输入(如命令"dir");
 * langmode, 当前任务语言模式,0-英文,1-日文,2-日文EUC;
 * langbyte1,辅助日文全角字符的处理(0-还未获取全角字符编码;1-已获取全角第1字节编码)。*/
    int ds_base, cons_stack;
    struct FILEHANDLE *fhandle;
    int *fat;
    char *cmdline;
    unsigned char langmode, langbyte1;
};
/* struct TASKLEVEL,
 * 管理任务层级的结构体类型。*/
struct TASKLEVEL {
    int running; /* 当前任务层处于运行状态的任务数 */
    int now;     /* 当前任务层正运行任务在tasks数组中的下标 */
    /* 指向处于本层级任务的指针数组 */
    struct TASK *tasks[MAX_TASKS_LV];
};
/* struct TASKCTL,
 * 管理系统所有任务的结构体类型。*/
struct TASKCTL {
/* now_lv,当前正运行任务的任务层;
 * lv_change,任务层调度标志(0-否;1-是)。*/
    int now_lv;
    char lv_change;

/* level, 管理任务层级分配的结构体数组;
 * tasks0,最多管理 MAX_TASKS 个任务的结构体数组。*/
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK tasks0[MAX_TASKS];
};

extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);

/* console.c */
/* struct CONSOLE,
 * 管理命令行窗口的结构体类型。*/
struct CONSOLE {
/* sht,管理命令行窗口画面的结构体指针;
 * cur_x,cur_y,cur_c,光标坐标和色号;
 * timer,命令行窗口定时器指针。*/
    struct SHEET *sht;
    int cur_x, cur_y, cur_c;
    struct TIMER *timer;
};
/* struct FILEHANDLE,
 * 管理打开文件的结构体类型。*/
struct FILEHANDLE {
    char *buf; /* 缓存文件内容 */
    int size;  /* 文件大小 */
    int pos;   /* 文件当前位置 */
};
void console_task(struct SHEET *sheet, int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_exit(struct CONSOLE *cons, int *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_langmode(struct CONSOLE *cons, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);

/* file.c */
/* struct FILEINFO,
 * 描述(FAT文件系统)文件信息的结构体。 */
struct FILEINFO {
/* name, 文件名,name[0]=0x00时表无文件名,
 * name[0]=0x05时表明文件已被删除,文件名
 * 不足8字节用空格补齐;
 * ext,文件扩展名,不足3字节用空格补齐;
 * type为文件属性,0x20-普通文件,0x01-只读文件,
 * 0x02-隐藏文件,0x04-系统文件,0x10-目录。*/
    unsigned char name[8], ext[3], type;
    char reserve[10]; /* 保留未用 */
/* time,文件时间;date,文件日期;clustno,文
 * 件内容起始簇号(扇区号);size,文件大小。*/
    unsigned short time, date, clustno;
    unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
char *file_loadfile2(int clustno, int *psize, int *fat);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);

/* haribote OS中部分数据结构体类型有耦合(嵌套互指)现象,勿轻易模仿。*/
