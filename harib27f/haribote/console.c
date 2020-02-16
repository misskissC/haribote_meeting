/* console.c,命令行(控制台)窗口管理程序接口 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

/* console_task,
 * 命令行窗口任务程序代码。*/
void console_task(struct SHEET *sheet, int memtotal)
{
    /* 获取管理当前正运行任务结构体即本正运行任务结构体基址;
     * 管理内存的结构体基址;分配内存用作存储FAT。*/
    struct TASK *task = task_now();
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    struct CONSOLE cons;
    struct FILEHANDLE fhandle[8];
    char cmdline[30];
    /* nihongo字库内存基址被存在0x0fe8处(见HarMain) */
    unsigned char *nihongo = (char *) *((int *) 0x0fe8);

    /* 初始化管理命令行窗口(终端)的结构体 cons。
     * 此文将 struct SHEET 和 struct CONSOLE 分别解释为
     * "管理窗口画面信息的结构体"和"管理命令行窗口的结构体"类型。*/
    cons.sht = sheet;   /* 命令行窗口画面信息管理结构体 */
    cons.cur_x =  8;    /* 光标x方向起始位置 */
    cons.cur_y = 28;    /* 光标y方向起始位置 */
    cons.cur_c = -1;    /* 光标初始色号-1不显示 */
    task->cons = &cons; /* 为当前任务关联终端 */
    task->cmdline = cmdline; /* 用于保存终端输入命令 */

    /* 设置用于命令行窗口光标闪烁的定时器,初始
     * 超时数据为1,超时时间为50个时间片(500ms)。*/
    if (cons.sht != 0) {
        cons.timer = timer_alloc();
        timer_init(cons.timer, &task->fifo, 1);
        timer_settime(cons.timer, 50);
    }
    /* 从软盘映像读取FAT到fat所指内存;初始化任务用于管理打开文件的指针数组 */
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
    for (i = 0; i < 8; i++) {
        fhandle[i].buf = 0;
    }
    task->fhandle = fhandle;
    task->fat = fat;

    /* 设置命令行窗口任务所支持的语言模式。若nihongo内存段
     * 末端为0xff则表示不支持日文汉字,见 HariMain()。*/
    if (nihongo[4096] != 0xff) {
        task->langmode = 1;
    } else {
        task->langmode = 0;
    }
    task->langbyte1 = 0;

    /* 在命令行窗口中加入输入提示符 */
    cons_putchar(&cons, '>', 1);

    /* 命令行窗口任务主循环,处理任务循环队列中的数据 */
    for (;;) {
        /* 在访问循环队列过程中应禁止CPU处理当前任
         * 务中断,以防中断程序访问循环队列造成冲突。
         * 如定时器中断发生任务调度。*/
        io_cli();
        
        /* 若命令行窗口任务队列中无数据需处理则让该任务睡眠,
         * 并调度优先级最高的任务运行。*/
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            /* 从命令行窗口任务队列中读取数据 */
            i = fifo32_get(&task->fifo);
            io_sti();

            /* 判断所读数据含义 */
            
            /* 来自定时器的数据,则根据数据标识光标显示颜色的数据;
             * 0-光标为白色,1-光标为黑色;光标每500ms变化一次颜色
             * 以造光标闪烁之感。*/
            if (i <= 1 && cons.sht != 0) {
                if (i != 0) {
                    /* 置命令行窗口定时器下一次超时数据为0,以标识光标应显示黑
                     * 色, 此文觉得能提供一个改变定时器超时数据的接口更好。*/
                    timer_init(cons.timer, &task->fifo, 0);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    /* 下一个超时数据应为1,以标识光标的白色 */
                    timer_init(cons.timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                /* 重新设置光标定时器500ms超时 */
                timer_settime(cons.timer, 50);
            }

            /* 以下数据来自主程序任务HariMain;主程序任务中的数据大多来自键盘和鼠标
             * I/O --> 主程序循环队列 --> 当前被选中命令行窗口任务循环队列。*/
            
            /* 命令行窗口任务循环队列数据 2-显示光标;3-隐藏光标 */
            if (i == 2) {
                cons.cur_c = COL8_FFFFFF;
            }
            if (i == 3) { /* 隐藏光标时刷新光标区域画面信息 */
                if (cons.sht != 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
                        cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                }
                cons.cur_c = -1;
            }

            /* 4-退出命令行窗口 */
            if (i == 4) {
                cmd_exit(&cons, fat);
            }

            /* [256,511], 键盘向被选中命令行窗口中输入的键盘数据 */
            if (256 <= i && i <= 511) {
                if (i == 8 + 256) { /* 退格键 */
                    if (cons.cur_x > 16) {
                        /* 收到退格键时,用黑色空格填充当前字符,光标向前移动 */
                        cons_putchar(&cons, ' ', 0);
                        cons.cur_x -= 8;
                    }
                } else if (i == 10 + 256) { /* 回车键 */
                    /* 用黑色空格填充光标然后换行 */
                    cons_putchar(&cons, ' ', 0);
                    cmdline[cons.cur_x / 8 - 2] = 0; /* 命令以0结尾 */
                    cons_newline(&cons); /* 换行 */
                    /* 当在被选中命令行中接收到回车键时,处理 cmdline 中的键盘数据 */
                    cons_runcmd(cmdline, &cons, fat, memtotal); /* 执行命令 */
                    /* 若命令行管理窗口画面指针为空则退出命令行窗口 */
                    if (cons.sht == 0) {
                        cmd_exit(&cons, fat);
                    }
                    /* 在命令行窗口当前位置显示输入提示符 */
                    cons_putchar(&cons, '>', 1);
                } else { /* 键盘常规数据输入 */
                    if (cons.cur_x < 240) {
                        /* 减去队列中键盘数据标识得到其编码并
                         * 存于cmdline 数组中,同时移动光标。*/
                        cmdline[cons.cur_x / 8 - 2] = i - 256;
                        cons_putchar(&cons, i - 256, 1);
                    }
                }
            }
            
            /* 刷新命令行窗口中光标的显示 */
            if (cons.sht != 0) {
                /* 刷新画面信息 */
                if (cons.cur_c >= 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
                        cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                }
                /* 刷新光标的显示 */
                sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
            }
        }
    }
}

/* cons_putchar,
 * 在cons所指命令行窗口光标当前位置显示chr字符,move标识光标是否移动。*/
void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
    /* 将chr转换为字符串存储在s中 */
    char s[2];
    s[0] = chr;
    s[1] = 0;

    if (s[0] == 0x09) { /* chr为水平制表符 */
        /* 刷新水平制表符的显示,水平制表符的画面用空格填充,将光标后移到正确位置 */
        for (;;) {
            if (cons->sht != 0) {
                putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
            }
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) { /* 到行尾时换行 */
                cons_newline(cons);
            }
            if (((cons->cur_x - 8) & 0x1f) == 0) { /* 1水平制表符占4字符位置 */
                break;
            }
        }
    } else if (s[0] == 0x0a) { /* chr为换行符 */
        cons_newline(cons);
    } else if (s[0] == 0x0d) { /* chr为回车 */
        /* 什么也不做 */
    } else { /* chr为键盘普通字符 */
        /* 显示该字符 */
        if (cons->sht != 0) {
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
        }
        if (move != 0) {
            /* move不为0标识光标随着移动 */
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
        }
    }
    return;
}

/* cons_newline,
 * 在cons所指命令行窗口中换行。*/
void cons_newline(struct CONSOLE *cons)
{
    int x, y;
    /* cons所指命令行窗口画面信息 */
    struct SHEET *sheet = cons->sht;
    /* 管理当前正运行任务即本程序的结构体基址 */
    struct TASK *task = task_now();
    
    /* 命令行窗口未满屏时,将光标直接移动到下一行 */
    if (cons->cur_y < 28 + 112) { 
        cons->cur_y += 16;
    } else { /* 否则窗口将发生滚动 */
        if (sheet != 0) {
            /* 将窗口中内容依次上移一行 */
            for (y = 28; y < 28 + 112; y++) {
                for (x = 8; x < 8 + 240; x++) {
                    sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
                }
            }
            /* 将命令行窗口往下其余部分都以黑色涂满 */
            for (y = 28 + 112; y < 28 + 128; y++) {
                for (x = 8; x < 8 + 240; x++) {
                    sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                }
            }
            /* 刷新窗口刚刚被更改过画面信息的显示 */
            sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
        }
    }
    /* 根据当前字库模式将光标移动行首 */
    cons->cur_x = 8;
    if (task->langmode == 1 && task->langbyte1 != 0) {
        cons->cur_x = 16;
    }
    return;
}

/* cons_putstr0,
 * 在cons所指命令行窗口中显示s所指字符串(0结尾)。*/
void cons_putstr0(struct CONSOLE *cons, char *s)
{
    for (; *s != 0; s++) {
        cons_putchar(cons, *s, 1);
    }
    return;
}

/* cons_putstr1,
 * 在cons所指命令行窗口中显示s所指字符的前l个。*/
void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
    int i;
    for (i = 0; i < l; i++) {
        cons_putchar(cons, s[i], 1);
    }
    return;
}

/* cons_runcmd,
 * 在cons窗口中执行cmdline中所包含的命令(或可执行程序)。*/
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
    /* 判断 cmdline 是否为haribote所支持的某个命令 */
    if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
        cmd_mem(cons, memtotal); /* mem, 显示内存容量命令 */
    } else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
        cmd_cls(cons); /* cls, 清屏命令 */
    } else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
        cmd_dir(cons); /* dir, 显示文件信息命令 */
    } else if (strcmp(cmdline, "exit") == 0) {
        cmd_exit(cons, fat); /* exit, 退出窗口命令 */
    } else if (strncmp(cmdline, "start ", 6) == 0) {
        cmd_start(cons, cmdline, memtotal); /* start cmd,新建命令行窗口解析执行 cmd */
    } else if (strncmp(cmdline, "ncst ", 5) == 0) {
        cmd_ncst(cons, cmdline, memtotal); /* ncst cmd, 新建任务解析执行cmd */
    } else if (strncmp(cmdline, "langmode ", 9) == 0) {
        cmd_langmode(cons, cmdline); /* 设置当前语言模式 */
    
    /* 若cmdline不为haribote所支持命令,则判断其是否为可执行程序 */
    } else if (cmdline[0] != 0) {
        if (cmd_app(cons, fat, cmdline) == 0) {
            /* 若既非haribote支持命令也非可执行程序则提示不能识别键盘输入 */
            cons_putstr0(cons, "Bad command.\n\n");
        }
    }
    return;
}

/* cmd_mem,
 * mem 命令对应子程序,
 * 在cons所指窗口显示内存使用情况。*/
void cmd_mem(struct CONSOLE *cons, int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[60];
    sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    cons_putstr0(cons, s);
    return;
}

/* cmd_cls,
 * cls 命令对应子程序,将cons所指命令行窗口清屏。*/
void cmd_cls(struct CONSOLE *cons)
{
    int x, y;
    struct SHEET *sheet = cons->sht;
    /* 用黑色画面信息填充窗口 */
    for (y = 28; y < 28 + 128; y++) {
        for (x = 8; x < 8 + 240; x++) {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    /* 刷新屏幕画面的显示,将光标置在最后一行 */
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

/* cmd_dir,
 * dir 命令对应子程序,在cons所指窗口中列表显示当前系统的文件信息。*/
void cmd_dir(struct CONSOLE *cons)
{
    /* 获取软盘映像文件信息区域基址 */
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int i, j;
    char s[30];
    
    for (i = 0; i < 224; i++) {
        /* 跳过软盘映像文件信息区域无文件信息的项 */
        if (finfo[i].name[0] == 0x00) {
            break;
        }
        /* 0xe5应该是0x05?获取有效的文件信息并依次显示 */
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

/* cmd_exit,
 * eixt 命令或命令4对应子程序,退出cons所指命令行窗口。*/
void cmd_exit(struct CONSOLE *cons, int *fat)
{
    /* 管理内存结构体基址 */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_now(); /* 当前正运行任务 */
    /* 0xfe4中存储了屏幕画面管理结构体地址,见 HarMain() */
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    /* 0x0fec中存储了主程序循环队列首地址,见 HarMain() */
    struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);

    /* 取消cons所指窗口用于光标闪烁的定时器 */
    if (cons->sht != 0) {
        timer_cancel(cons->timer);
    }
    /* 释放FAT所占内存 */
    memman_free_4k(memman, (int) fat, 4 * 2880);

    /* 向主任务 HariMain() 循环队列发送当前命令行窗口已退出数据,
     * 已让主任务 HariMain() 回收cons所指窗口相关内存资源。*/
    io_cli();
    if (cons->sht != 0) {
        fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768); /* 768-1023 */
    } else {
        fifo32_put(fifo, task - taskctl->tasks0 + 1024); /* 1024-2023 */
    }
    io_sti();

    /* 将当前任务从其任务层中移除而进入睡眠 */
    for (;;) {
        task_sleep(task);
    }
}

/* cmd_start,
 * "start cmd"命令对应子程序,打开新的命令行窗口并执行cmd。*/
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
    /* 获取系统画面管理结构体指针 */
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    /* 新建命令行窗口画面等相关信息,见 bootpack.c */
    struct SHEET *sht = open_console(shtctl, memtotal);
    struct FIFO32 *fifo = &sht->task->fifo;
    int i;

    /* 将新建命令行窗口画面移到屏幕(32,4)坐标处,并在顶层显示该窗口 */
    sheet_slide(sht, 32, 4);
    sheet_updown(sht, shtctl->top);
    
    /* 将 start 命令的参数发送给新建命令行窗口中运行 */
    for (i = 6; cmdline[i] != 0; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); /* Enter */
    cons_newline(cons);
    return;
}

/* cmd_ncst,
 * "ncst cmd"命令对应子程序,在cons所指窗口中执行cmd命令。与"start cmd"不同
 * 的是,该命令不创建新窗口只创建新的任务执行cmd命令,相当于一个后台程序。*/
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
    /* 创建一个新的任务 */
    struct TASK *task = open_constask(0, memtotal);
    struct FIFO32 *fifo = &task->fifo;
    int i;
    
    /* 将ncst命令的参数发送给新建任务解析执行 */
    for (i = 5; cmdline[i] != 0; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); /* Enter */
    cons_newline(cons);
    return;
}

/* cmd_langmode,
 * "langmode x"设置当前任务(cons所指命令行窗口任务)的语言模式为x
 * 0-英文模式;1-日文汉字模式;2-日文EUC模式。*/
void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
    struct TASK *task = task_now();
    unsigned char mode = cmdline[9] - '0';
    if (mode <= 2) {
        task->langmode = mode;
    } else {
        cons_putstr0(cons, "mode number error.\n");
    }
    cons_newline(cons);
    return;
}

/* cmd_app,
 * 在cons所指命令行窗口中运行可执行文件的处理程序。*/
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
    /* 获取管理内存结构体基址 */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo;
    char name[18], *p, *q;
    /* 管理当前正运行任务即本程序任务机构体基址,
     * 用于管理应用程序的运行,相当于在本任务程序中插入一段应用程序。*/
    struct TASK *task = task_now();
    int i, segsiz, datsiz, esp, dathrb, appsiz;
    struct SHTCTL *shtctl;
    struct SHEET *sht;

    /* 从命令行窗口输入中获取可执行文件名 */
    for (i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0; /* 让可执行文件名以0结尾(字符串方式) */

    /* 在软盘映像文件信息区域搜索name文件的文件信息,见 file.c */
    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    /* 若搜索name文件失败则为name文件补上".HRB"后缀再搜索 */
    if (finfo == 0 && name[i - 1] != '.') {
        name[i    ] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }

    /* 若获取到name文件信息 */
    if (finfo != 0) {
        /* 则将其内容载入到p所指内存段中 */
        appsiz = finfo->size;
        p = file_loadfile2(finfo->clustno, &appsiz, fat);

        /* 由作者改编编译链接器所编译链接得到的可执行文件含36字节头部信息,
         * 偏移0x00处4字节为请求haribote为其预分配的数据段大小;
         * 偏移0x04处4字节为haribote应用程序标识'H','a',r','i';
         * 偏移0x0c处4字节为栈顶在数据段中的偏移;
         * 偏移0x10处4字节为可执行文件数据段(可执行文件层面概念)大小;
         * 偏移0x14处4字节为可执行文件数据段在可执行文件中起始偏移地址。*/
        if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            segsiz = *((int *) (p + 0x0000));
            esp    = *((int *) (p + 0x000c));
            datsiz = *((int *) (p + 0x0010));
            dathrb = *((int *) (p + 0x0014));
            /* 为可执行程序分配数据内存段并记录于 ds_base 成员中 */
            q = (char *) memman_alloc_4k(memman, segsiz);
            task->ds_base = (int) q;
            /* 为可执行应用程序设置代码段和数据段描述符 */
            set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
            set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
            /* 将可执行程序中的数据拷贝到栈顶之后(应用程序数据段前部分为栈,后部分用于数据) */
            for (i = 0; i < datsiz; i++) {
                q[esp + i] = p[dathrb + i];
            }
/* 从此处可看出haribote app在内存中布局大体如下。
 * 
 * ldt[0]-可执行程序代码段
 * |--------------|
 * |可执行程序内容|
 * |--------------|
 * p
 * 
 * ldt[1]-可执行程序数据段
 * |---------------------------------|
 * |stack memory|data zone (and heap)|
 * |---------------------------------|
 * q           esp                     */
 
            /* 跳转执行可执行应用程序。
             * 
             * + 4将标识cs.TI=1即标识cs为LDT选择符,0*8+4和1*8+4分别为当前
             * 任务LDT[0]和LDT[1]选择符。
             *
             * 可执行程序偏移0x1b处1字节为jmp指令,偏移0x1c处4字节内容为jmp
             * 指令操作数,通过执行偏移0x1b处指令可跳转到可执行程序入口处。*/
            start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));

            /* 在可执行应用程序执行完毕后将返回到此处。
             * 
             * 应用程序是通过系统调用 api_end 返回到此处的。
             * 届时挑个应用程序了解 api_end 返回到此处原理。
             * 
             * 当应用程序运行结束返回后,为其释放相关资源。*/
            shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
            /* 若应用程序开启了窗口,则释放其画面缓冲区 */
            for (i = 0; i < MAX_SHEETS; i++) {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
                    sheet_free(sht);
                }
            }
            /* 释放应用程序所打开文件所占内存资源 */
            for (i = 0; i < 8; i++) {
                if (task->fhandle[i].buf != 0) {
                    memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
                    task->fhandle[i].buf = 0;
                }
            }
            /* 释放应用程序所用的所有定时器 */
            timer_cancelall(&task->fifo);
            /* 释放应用程序数据段内存 */
            memman_free_4k(memman, (int) q, segsiz);
            task->langbyte1 = 0;
        } else {
            /* 不能识别可执行文件格式 */
            cons_putstr0(cons, ".hrb file format error.\n");
        }
        /* 释放可执行程序代码段内存 */
        memman_free_4k(memman, (int) p, appsiz);
        cons_newline(cons);
        return 1;
    }
    /* 软盘映像中无name命名文件返回0 */
    return 0;
}

/* hrb_api,
 * 系统调用C处理函数,由系统调用入口程序 _asm_hrb_api 调用(见dsctbl.c/init_gdtidt)。
 *
 * edx为应用程序中所传递的系统调用号;
 * 其余参数充当对应地充当各系统调用的参数。*/
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
    /* 当前运行任务结构体 */
    struct TASK *task = task_now();
    /* 当前任务数据段基址 */
    int ds_base = task->ds_base;
    /* 当前任务命令关联的命令行窗口结构体 */
    struct CONSOLE *cons = task->cons;
    /* 管理系统画面的结构体指针 */
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct SHEET *sht;
    /* 主程序循环队列指针 */
    struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
    /* 根据_asm_hrb_api在栈中为hrb_api准备的参数,
     * reg指向PUSHAD往栈中所备份的用户程序寄存器,
     * reg[0]: EDI, reg[1]:ESI, ..., reg[7]:EAX。*/
    int *reg = &eax + 1;
    int i;
    struct FILEINFO *finfo;
    struct FILEHANDLE *fh;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

/* 根据edx寄存器携带的系统调用号执行相应的系统调用 */

    /* api_putchar(), 打印字符的系统调用,eax携带字符 */
    if (edx == 1) {
        cons_putchar(cons, eax & 0xff, 1);
        
    /* api_putstr0(), 打印字符串的系统调用,
     * ebx为字符串在可执行程序数据段中的偏移地址。*/
    } else if (edx == 2) {
        cons_putstr0(cons, (char *) ebx + ds_base);
        
    /* api_putstr1(), 打印指定个数字符的系统调用,ebx为字符在可执行
     * 程序数据段中的偏移地址,ecx为欲打印字符个数。*/
    } else if (edx == 3) {
        cons_putstr1(cons, (char *) ebx + ds_base, ecx);
        
    /* api_end(), 应用程序返回系统调用,即返回到调用 start_app 处的系统调用 */
    } else if (edx == 4) {
        return &(task->tss.esp0);
    
    /* api_openwin(), 打开应用程序命令行窗口,
     * ebx,窗口画面信息在数据数据段中的偏移地址;
     * esi,edi,eax分别为窗口x,y方向大小和是否包含透明色的标识;
     * ecx,窗口标题在数据段起始地址。
     * 
     * 将所绘制窗口的管理结构体地址写往用户在栈中备份的EAX中,
     * 以作为系统调用返回到应用程序中返回值。*/
    } else if (edx == 5) {
        sht = sheet_alloc(shtctl);
        sht->task = task;
        sht->flags |= 0x10; /* 标识sht所指窗口为应用程序窗口 */
        sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
        make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
        sheet_updown(sht, shtctl->top); /* 顶层显示 */
        reg[7] = (int) sht;

    /* api_putstrwin(), 往ebx所指画面中(esi,edi)坐标起始处写入
     * ebp处的字符串画面信息,eax为颜色号。*/
    } else if (edx == 6) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        }

    /* api_boxfilwin(),
     * 在ebx所标识窗口中的(eax,ecx),(esi, edi)区域绘制色号为ebp的矩形 */
    } else if (edx == 7) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }

    /* api_initmalloc(),
     * 管理可执行程序数据段偏移ebx处内存段,初始[eax, eax+ecx)内存段空闲 */
    } else if (edx == 8) {
        memman_init((struct MEMMAN *) (ebx + ds_base));
        ecx &= 0xfffffff0; /* 16字节对齐 */
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);

    /* api_malloc(), 分配内存ecx字节内存,返回所分被内存首地址 */
    } else if (edx == 9) {
        ecx = (ecx + 0x0f) & 0xfffffff0; /* 16字节对齐 */
        reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);

    /* api_free(), 释放内存段[eax, eax+ecx) */
    } else if (edx == 10) {
        ecx = (ecx + 0x0f) & 0xfffffff0; /* 16字节对齐 */
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);

    /* api_point(), 在ebx所指窗口中(esi, edi)位置写入色号eax */
    } else if (edx == 11) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        sht->buf[sht->bxsize * edi + esi] = eax;
        /* ebx最低位为0时刷新该区域 */
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
        }
    
    /* api_refreshwin(),
     * 刷新ebx所指窗口[(eax, ecx),(esi, edi)]区域画面信息 */
    } else if (edx == 12) {
        sht = (struct SHEET *) ebx;
        sheet_refresh(sht, eax, ecx, esi, edi);

    /* api_linewin(),
     * 在ebx所指窗口中绘制线条[(eax,ecx),(esi,edi)],线条色号为ebp */
    } else if (edx == 13) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
        if ((ebx & 1) == 0) {
            if (eax > esi) {
                i = eax;
                eax = esi;
                esi = i;
            }
            if (ecx > edi) {
                i = ecx;
                ecx = edi;
                edi = i;
            }
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }

    /* api_closewin(), 释放ebx所指窗口 */
    } else if (edx == 14) {
        sheet_free((struct SHEET *) ebx);

    /* api_getkey(),
     * 从内核循环队列中读取数据,eax=0在无数
     * 据时直接返回,否则睡眠等待直到有数据。*/
    } else if (edx == 15) {
        for (;;) {
            io_cli();
            if (fifo32_status(&task->fifo) == 0) {
                if (eax != 0) { /* eax不为0时睡眠等待直到有数据 */
                    task_sleep(task);
                } else {
                    io_sti();
                    reg[7] = -1; /* 返回-1给应用程序 */
                    return 0;
                }
            }
            /* 读取内核循环队列数据 */
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1 && cons->sht != 0) { /* 来自光标定时器的数据 */
                /* 应用程序中无光标,所以光标定时数据为1以标识光标为白色 */
                timer_init(cons->timer, &task->fifo, 1);
                timer_settime(cons->timer, 50);
            }
            if (i == 2) { /* 光标为白色 */
                cons->cur_c = COL8_FFFFFF;
            }
            if (i == 3) { /* 关闭光标 */
                cons->cur_c = -1;
            }
            if (i == 4) { /* 关闭窗口,向主程序队列发送关闭窗口数据标识2024-2279 */
                timer_cancel(cons->timer);
                io_cli();
                fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);
                cons->sht = 0;
                io_sti();
            }
            if (i >= 256) { /* 键盘数据由EAX返回 */
                reg[7] = i - 256; /* 返回键盘输入编码 */
                return 0;
            }
        }

    /* api_alloctimer(),
     * 为应用程序分配一个定时器,并返回定时器地址 */
    } else if (edx == 16) {
        reg[7] = (int) timer_alloc();
        /* 置定时器自动取消标志 */
        ((struct TIMER *) reg[7])->flags2 = 1;

    /* api_inittimer(),
     * 初始化ebx指向的定时器的循环队列和标识数据 */
    } else if (edx == 17) {
        timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);

    /* api_settimer(), 设置ebx所指定时器超时时间eax */
    } else if (edx == 18) {
        timer_settime((struct TIMER *) ebx, eax);

    /* api_freetimer(), 释放ebx所指定时器 */
    } else if (edx == 19) {
        timer_free((struct TIMER *) ebx);

    /* api_beep(), 蜂鸣设置 */
    } else if (edx == 20) {
        if (eax == 0) {
            /* 蜂鸣开关设置 */
            i = io_in8(0x61);
            io_out8(0x61, i & 0x0d);
        } else {
            /* 音高设置 */
            i = 1193180000 / eax;
            io_out8(0x43, 0xb6);     /* 设置音高命令 */
            io_out8(0x42, i & 0xff); /* 设定值低8位 */
            io_out8(0x42, i >> 8);   /* 设定值高8位 */
            
            /* 蜂鸣开关设置 */
            i = io_in8(0x61);
            io_out8(0x61, (i | 0x03) & 0x0f);
        }
    
    /* api_fopen(), 打开ebx所指命名的文件,返回所打开文件指针 */
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
                fh->size = finfo->size;
                fh->pos = 0;
                fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
            }
        }
    
    /* api_fclose(), 释放eax所指打开文件 */
    } else if (edx == 22) {
        fh = (struct FILEHANDLE *) eax;
        memman_free_4k(memman, (int) fh->buf, fh->size);
        fh->buf = 0;

    /* api_fseek(),
     * 设置eax所指文件内容当前位置,
     * ecx=0,基于文件开始处偏移ebx;
     * ecx=1,基于文件当前位置偏移ebx;
     * ecx=2,基于文件末尾偏移ebx。*/
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

    /* api_fsize(),
     * 获取eax所指文件信息并返回,
     * ecx=0,返回文件大小;
     * ecx=1,返回文件当前位置;
     * ecx=2,返回文件当前位置与文件末尾的偏移。*/
    } else if (edx == 24) {
        fh = (struct FILEHANDLE *) eax;
        if (ecx == 0) {
            reg[7] = fh->size;
        } else if (ecx == 1) {
            reg[7] = fh->pos;
        } else if (ecx == 2) {
            reg[7] = fh->pos - fh->size;
        }

    /* api_fread(),
     * 从eax所指文件当前位置开始读取文件内
     * 容到ebx所指内存段中直到文件读取完毕。*/
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

    /* api_cmdline(),
     * 获取内核命令行输入,最多获取ecx字节,
     * 返回所获取命令行输入的字节数。*/
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

    /* api_getlang(), 获取当前任务语言模式 */
    } else if (edx == 27) {
        reg[7] = task->langmode;
    }
    return 0;
}

/* 在应用程序中引发栈异常和保护异常时,
 * 运行到 inthandler0c() 和 inthandler0d()处后栈中内容为
 * |SS   |
 * |ESP  |
 * |EFLAG|
 * |CS   |
 * |EIP  | <-- esp[11]
 * |er   | <-- esp[10]
 * |ES   | <-- esp[9]
 * |DS   | <-- esp[8]
 * |EAX  | <-- esp[7]
 * |ECX  | <-- esp[6]
 * |EDX  | <-- esp[5]
 * |EBX  | <-- esp[4]
 * |ESP  | <-- esp[3]
 * |EBP  | <-- esp[2]
 * |ESI  | <-- esp[1]
 * |EDI  | <-- esp[0]
 * |esp  | 
 * |EIP  | <-- ESP */
 
/* inthandler0c,
 * 栈异常C处理程序: 提示栈异常后结束应用程序。
 * 在发生栈异常时由栈异常处理入口程序 _asm_inthandler0c 调用。*/
int *inthandler0c(int *esp)
{
    /* 获取管理正运行即本程序任务结构体基址 */
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    /* 根据 _asm_inthandler0c() 栈中内容,esp[11]为应用程序发生栈异常处eip值,
     * 即打印引起栈异常语句在其代码段中的(偏移)地址。*/
    cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr0(cons, s);
    /* 返回内核栈顶指针的地址以结束应用程序,见 naskfunc.nas */
    return &(task->tss.esp0);
}

/* inthandler0d,
 * 保护异常C处理函数:提示保护异常并结束应用程序。
 * 在发生保护异常时由保护异常入口程序_asm_inthandler0d调用。*/
int *inthandler0d(int *esp)
{
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    /* 根据 _asm_inthandler0c() 栈中内容,esp[11]为应用程序发生栈异常处eip值,
     * 即打印引起栈异常语句在其代码段中的(偏移)地址。*/
    cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); /* 结束应用程序(见naskfunc.nas) */
}

/* hrb_api_linewin,
 * 在sht所指画面中描绘(x0,y0)和(x1,y1)两点之间线段的画面信息。*/
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

/* 用以下简图理解以上代码段的含义。
 * 
 * 如描述由(x0,y0)和(x1,y1)两点形成的线段line的画面信息。
 * (x0,y0)
 *    ............
 *    \          .
 *     \         .
 *      \        .
 *       \       .
 *        \      .
 *         \     .
 *          \    .
 *           \   .
 *            \  .
 *             \ .
 *              \.
 *            (x1,y1)
 * xlen = x1 - x0 + 1
 * ylen = y1 - y0 + 1
 * 基于(x0, y0)描绘线段line,当y方向增加dy时,x方向应成比例的增加dx=(dy * xlen) / ylen。
 * 此处dy=1024;1024为屏幕x向最大像素点数,将dy或dx(当x1 - x0 >= y1 - y0时)设置为1024是
 * 为了保证描绘线段的步长不为0。
 *
 * 其它方向和长度的线段也是这个过程哦。*/

    return;
}
