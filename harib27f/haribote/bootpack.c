/* bootpack.c,haribote os C主程序入口 && haribote 主任务程序 */

#include "bootpack.h"
#include <stdio.h>

#define KEYCMD_LED 0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

/* HariMain,
 * haribote OS C主程序入口,从asmhead.nas中跳转而来。*/
void HariMain(void)
{
    /* 以 struct BOOTINFO 类型访问 ADR_BOOTINFO 
     * 内存段即获取显卡信息参数所在内存段基址。*/
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct SHTCTL *shtctl;
    char s[40];
    struct FIFO32 fifo, keycmd;
    int fifobuf[128], keycmd_buf[32];
    int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    /* [MEMMAN_ADDR, sizeof(struct MEMMAN))内存段用作管理内存的结构体 */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    unsigned char *buf_back, buf_mouse[256];
    struct SHEET *sht_back, *sht_mouse;
    struct TASK *task_a, *task;
    /* keytable0,keytable1 分别是没有按下和按下shift键时键盘按键扫描码与键盘按
     * 键编码值的映射表,即用键盘输入扫描码作为数组下标即得到键盘按键的编码值。*/
    static char keytable0[0x80] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    static char keytable1[0x80] = {
        0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
    };
    int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
    int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
    struct SHEET *sht = 0, *key_win, *sht2;
    int *fat;
    unsigned char *nihongo;
    struct FILEINFO *finfo;
    /* hankaku.txt中包含的符号信息被转换为字库后与haribote OS
     * 链接在一起,在链接层面的起始标号为hankaku。*/
    extern char hankaku[4096];

    /* init_gdtidt,初始化GDT和IDT;
     * init_pic,初始化可编程中断控制器(PIC);
     * io_sti,在初始化GDT,IDT以及中断控制器后,允许CPU处理
     * 中断, asmhead.nas在进入保护模式前曾禁止CPU处理中断。*/
    init_gdtidt();
    init_pic();
    io_sti();

    /* HarMain主程序(task_a所管理任务)循环队列:fifobuf 所指内存块为
     * 队列缓冲区,长度为128, 该循环队列将在后续与 task_a 所管理任务
     * 关联。将主程序循环队列结构体的内存基址存储在内存0x0fec处, 以
     * 供其他任务可间接通过该地址访问 fifobuf 循环队列。*/
    fifo32_init(&fifo, 128, fifobuf, 0);
    *((int *) 0x0fec) = (int) &fifo;

    /* 初始化定时器控制器及管理系统定时的数据结构体 */
    init_pit();

    /* 初始化键盘和鼠标控制器,初始化管理二者的数据结构体。
     * 将主程序中循环队列 fifo 与键盘和鼠标关联,以用 fifo
     * 接收键盘和鼠标数据。将键盘和鼠标数据标识(基数)分别
     * 设置为256和512,即 fifo 中[256,512)区间值为键盘数据,
     * [512,767]区间值为鼠标数据。
     *
     * 在初始化中断控制器和使能键盘和鼠标后,当键盘和鼠标有
     * 数据输入时将分别触发键盘和鼠标中断, 从而会让CPU分别
     * 执行键盘和鼠标中断入口处理程序 _asm_inthandler21 和
     * _asm_inthandler2c,从而再调用相应的中断C处理函数而将
     * 数据存入循环队列 fifo 中,主程序会处理 fifo中的数据。*/
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    
    /* 配置主PIC,允许日时钟(定时器),键盘,从PIC中断;
     * 配置从PIC允许实时钟(CMOS ROM)中断。*/
    io_out8(PIC0_IMR, 0xf8);
    io_out8(PIC1_IMR, 0xef);

    /* 设置键盘命令循环队列:keycmd_buf 所指内存段为键盘命令循环队列缓冲区,
     * 长度为32。键盘命令循环队列用于缓存主程序向键盘控制器发送的命令数据。*/
    fifo32_init(&keycmd, 32, keycmd_buf, 0);

    /* 检测扩展内存段[0x400000,0xbfffffff)中始于0x400000连续可用内存段;
     * 初始化内存管理结构体 memmam;记录实模式内存段[0x1000, 0x9efff)为
     * 空闲内存; 记录扩展内存段连续可用且空闲的内存段。*/
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    /* 初始化调色板 */
    init_palette();
    
    /* 初始化屏幕窗口管理结构体,参数中的显存基址和分辨率由BIOS获得(见asmhead.nas)。*/
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);

    /* 初始化任务管理。
     * task_a 将管理当前主程序任务, 将 task_a 与 fifo关联。
     * 调整当前任务任务层级(0->1), 当再次发生任务调度时会
     * 调度任务层1中的任务(任务层0已无任务),即约20ms后,内
     * 核程序HariMain将由 task_a 所指结构体管理。*/
    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 2);

    /*将系统窗口管理结构体内存基址存储在0x0fe4处,其他任务
     * 可通过0x0fe4访问到 shtctl。
     * 
     * 初始 task_a 所管理当前任务的语言模式为英文。*/
    *((int *) 0x0fe4) = (int) shtctl;
    task_a->langmode = 0;

    /* 为屏幕背景窗口画面信息分配管理结构体;为屏幕背景窗口画面信息分配缓冲区;
     * 将屏幕背景窗口画面信息缓存在 buf_back 所指缓冲区中。*/
    sht_back  = sheet_alloc(shtctl);
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    /* 描绘命令行窗口画面信息到指定缓冲区中;为命令行窗口关联任务。
     * 该命令行窗口任务被添加在任务层2中,由于任务层1中还有 task_a
     * 所管理任务, 所以任务层2中的命令行窗口任务还不会被调度运行。
     * 当任务层0和1中的任务都进入休眠且置任务层调度标志时,任务层2
     * 中的命令行窗口任务将会被调度运行。同时, 由于命令行窗口初始
     * 图层高度为-1, 所以命令行窗口也还不会被写入显存显示在屏幕上。
     *
     * key_win 将指向屏幕上被选中的命令行窗口。
     *
     * haribte启动后,该命令行窗口被默认创建并显示作为交互界面,所以
     * 最初的这个命令行窗口就是所谓的控制终端窗口。*/
    key_win = open_console(shtctl, memtotal);

    /* 分配管理鼠标画面信息的结构体(sht_mouse);
     * 描绘鼠标画面信息缓存于 sht_mouse 中;初始鼠标位置(约屏幕中间位置)。*/
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;

    /* 调整屏幕背景,命令行窗口,鼠标在屏幕上的初始位置 */
    sheet_slide(sht_back,  0,  0);
    sheet_slide(key_win,   32, 4);
    sheet_slide(sht_mouse, mx, my);

    /* 依次将屏幕背景,命令行窗口,鼠标画面信息写入显存显示(屏幕背
     * 景图层高度为最低值0,命令行窗口图层高度为1,鼠标图层高度为2)。*/
    sheet_updown(sht_back,  0);
    sheet_updown(key_win,   1);
    sheet_updown(sht_mouse, 2);
    /* 标识初始命令行窗口即中断被选中的状态 */
    keywin_on(key_win);
/* 程序执行到此处,屏幕就显示出了屏幕背景窗口,用于交互的命令行终端以及鼠标啦 */


    /* 将键盘初始状态写入键盘命令循环队列中,待 task_a 将键盘初
     * 始状态发送给键盘控制器,以指示键盘控制器如何显示状态灯。*/
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    /* 从软盘映像(见 asmhead.nas 中haribote内存的大体分布和 file.c)中读取FAT */
    fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
    /* 在FAT12文件信息区域(第20扇区)搜索 nihongo.fnt 字库文件,
     * 若软盘映像中包含 nihongo.fnt 字库文件则将其载入内存中。*/
    finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if (finfo != 0) {
        i = finfo->size;
        nihongo = file_loadfile2(finfo->clustno, &i, fat);
    /* 若无 nihongo.fnt 字库文件则使用已被编译链接到haribote os中的英文字库文件hankaku */
    } else {
        /* hankaku 中只有256个字符的画面信息,nihongo.fnt 除包含该256字符画面信息外还包含32*94*47
         * 个日文汉字画面信息,用0xff填充日文汉字画面信息对应的内存段,以表征当前字库不支持日文汉字。*/
        nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
        for (i = 0; i < 16 * 256; i++) {
            nihongo[i] = hankaku[i];
        }
        for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
            nihongo[i] = 0xff;
        }
    }
    /* 将包含字库文件内容的内存段基址写入0x0fe8处,以供其他任务通过
     * 0x0fe8地址能访问字库;并释放FAT所占内存(task_a 中不再使用FAT)。*/
    *((int *) 0x0fe8) = (int) nihongo;
    memman_free_4k(memman, (int) fat, 4 * 2880);

/* haribote C程序初始化部分代码到此完毕。
 * 大概是来自大神自由发挥的主程序代码都带些随意属性,以上代码有被进一步整理
 * 和简化的空间...在阅读后续代码之前,可预备代码更加随意(尤指嵌套)的心态。*/


    /* task_a 所所管理主任务的主循环程序,处理人机交互数据。
     * 主要显式涉及键盘和鼠标输入数据队列 fifo 中的键盘和鼠标数据;
     * 并将任务层1中无事可做的任务休眠以调度其他(任务层中)任务运行。*/
    for (;;) {
        /* 若键盘命令缓冲循环队列中有数据且键盘可正常接收命令的标志置位则发送键盘命
         * 令缓冲队列中的键盘命令给键盘键盘控制器以让键盘控制器正确标识状态灯状态。*/
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }

/* 处理 task_a 所管理任务循环队列中的数据,在访问 task_a 任务循环队列过
 * 程中应禁止CPU处理当前任务中断, 以防中断处理程序访问循环队列造成冲突。
 * 如键盘中断,鼠标中断,以及定时器中断。
 *
 * 主任务会将从I/O(键盘,鼠标)接收到数据发送给当前被选中的命令行窗口任务
 * 中,同时也会接收命令行窗口发送而来的数据。主程序区分数据来源的方式为
 * [256,511], 键盘输入数据(键盘数据基数为256);
 * [512, 767],鼠标输入数据(鼠标数据基数为512);
 * [768, 2279],命令行窗口所发送数据(含义为释放相应资源)。*/
        
        io_cli();
        /* 当前任务循环队列中无数据需处理时,再检查还有无其他事情可做,若鼠标位置
         * 有更新则更新鼠标画面的显示,若窗口有移动则更新sht所指窗口画面信息位置,
         * 否则表明 task_a 所管理的当前程序任务无事可做则休眠当前任务并切换到优
         * 先级最高的任务中运行(即调度任务层2中的命令行窗口任务运行)。*/
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

        /* 若 task_a 所指任务循环队列中有需处理的数据则读取处理 */
        } else {
            i = fifo32_get(&fifo);
            io_sti();

            /* 若 key_win 不为NULL且已没有再管理命令行窗口, */
            if (key_win != 0 && key_win->flags == 0) {
                /* top为1表明屏幕上只有背景窗口和鼠标已无命令行窗口,需将 key_win置空 */
                if (shtctl->top == 1) {
                    key_win = 0;
                /* 当top不为1(大于1)则将鼠标之下的处于最高图层的命令行窗口选中标亮 */
                } else {
                    key_win = shtctl->sheets[shtctl->top - 1];
                    keywin_on(key_win);
                }
            }

            /* 从 task_a 中读取到[256,511]范围数据为键盘数据,见 keyboard.c 中接口的调用 */
            if (256 <= i && i <= 511) {
                if (i < 0x80 + 256) { /* 键盘按键扫描码转换为键盘按键编码 */
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }

                /* 键盘码经转换后为字母,若大写锁定键和上档键同时没有
                 * 被按下或同时按下,则将字母编码转换为小写字母编码。*/
                if ('A' <= s[0] && s[0] <= 'Z') {
                    if (((key_leds & 4) == 0 && key_shift == 0) ||
                        ((key_leds & 4) != 0 && key_shift != 0)) {
                        s[0] += 0x20;
                    }
                }
                /* 将键盘常规输入数据写入当前被选中命令行窗口的任务循环队列中 */
                if (s[0] != 0 && key_win != 0) {
                    fifo32_put(&key_win->task->fifo, s[0] + 256);
                }

                /* 若键盘输入Tab则切换当前被选中窗口下一图层的命令行窗口高亮 */
                if (i == 256 + 0x0f && key_win != 0) { /* Tab */
                    keywin_off(key_win);
                    j = key_win->height - 1;
                    /* 若只有一个命令行窗口则j仍旧为该窗口图层高度 */
                    if (j == 0) {
                        j = shtctl->top - 1;
                    }
                    key_win = shtctl->sheets[j];
                    keywin_on(key_win);
                }

                /* 标识是否按下辅助按键 */
                if (i == 256 + 0x2a) { /* 按下左移键 */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) { /* 按下右移键 */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) { /* 左移键放开 */
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) { /* 右移键放开 */
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a) { /* 大写锁定,CapsLock */
                    /* 大写锁定状态翻转并将该状态发送给键盘指示LED灯的显示 */
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) { /* 数字锁定,NumLock */
                    /* 数字锁定状态翻转并将该状态发送给键盘指示LED灯显示 */
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) { /* ScrollLock */
                    /* 翻转ScrollLock按键状态并将该状态发送键盘控制器以指示LED灯显示 */
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                /* Shift+F1:终止被选中命令行窗口中的应用程序 */
                if (i == 256 + 0x3b && key_shift != 0 && key_win != 0) {
                    /* 当CPU执行以下程序时, key_win 所指命令行窗口任务肯定
                     * 没有被调度执行。若 key_win 所指命令行窗口曾启动运行
                     * 过应用程序,则该命令行窗口任务管理结构体将被用来管理
                     * 该应用程序的执行; 此处直接修改该应用程序TSS中寄存器
                     * eax和eip以让该应用程序被调度运行时跳转 _asm_end_app
                     * 子程序处执行而返回到命令行窗口启动应用程序处语句处,
                     * 见 console_task-->cons_runcmd-->cmd_app-->start_app 
                     * (如果 key_win 中没有执行过应用程序,栈结构似乎会被破坏)。*/
                    task = key_win->task;
                    if (task != 0 && task->tss.ss0 != 0) {
                        /* 在当前被选中命令行窗口中打印 Break(key) 信息 */
                        cons_putstr0(task->cons, "\nBreak(key) :\n");
                        io_cli();
                        task->tss.eax = (int) &(task->tss.esp0);
                        task->tss.eip = (int) asm_end_app;
                        io_sti();
                        /* 此处调用task_run主要是为了置任务层调度标志,以保证任务
                         * 层0,1中任务执行完毕后处于任务层2中的任务能得到调度,从
                         * 而执行 asm_end_app 以退出应用程序。*/
                        task_run(task, -1, 0);
                    }
                }

                /* Shift + F2: 在屏幕顶层新建命令行窗口 */
                if (i == 256 + 0x3c && key_shift != 0) { /* Shift+F2 */
                    /* 置灰原被选中命令行窗口 */
                    if (key_win != 0) {
                        keywin_off(key_win);
                    }
                    /* key_win 指向新创建命令行窗口,在屏幕(32,4)位置新创建
                     * 命令窗口起,将新建窗口置于屏幕顶层并高亮表征被选中。*/
                    key_win = open_console(shtctl, memtotal);
                    sheet_slide(key_win, 32, 4);
                    sheet_updown(key_win, shtctl->top);
                    keywin_on(key_win);
                }
                /* F11: 将图层1中的窗口移到屏幕顶层窗口之下并刷新窗口显示 */
                if (i == 256 + 0x57) { /* F11 */
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                }
                /* 键盘控制器所返回此键码表示键盘控制器已正确收键盘命令 */
                if (i == 256 + 0xfa) {
                    keycmd_wait = -1;
                }
                /* 若键盘控制器接收命令不正常则等待键盘输入缓冲器空后再向键盘发送个数据 */
                if (i == 256 + 0xfe) {
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }

            /* 从 task_a 中读取到[512,767]范围数据为鼠标数据,见 mouse.c 中接口的调用 */
            } else if (512 <= i && i <= 767) {
                /* 解析鼠标数据并刷新鼠标坐标 */
                if (mouse_decode(&mdec, i - 512) != 0) {
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
                    /* 鼠标左键按下, */
                    if ((mdec.btn & 0x01) != 0) {
                        if (mmx < 0) {
                            /* 检查是否在某个窗口上按下左键, 若是则将该窗口选至窗
                             * 口顶层并高亮窗口已被选中,top-1,top为鼠标所在图层。*/
                            for (j = shtctl->top - 1; j > 0; j--) {
                                sht = shtctl->sheets[j];
                                x = mx - sht->vx0;
                                y = my - sht->vy0;
                                if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                    /* 鼠标点击的不是窗口透明区域 */
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        /* 将被选中窗口移至窗口顶层显示 */
                                        sheet_updown(sht, shtctl->top - 1);
                                        /* 置灰原顶层窗口标题区域,高亮被选中窗口标题区域 */
                                        if (sht != key_win) {
                                            keywin_off(key_win);
                                            key_win = sht;
                                            keywin_on(key_win);
                                        }
                                        /* 鼠标在窗口移动区按下左键 */
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx = mx;
                                            mmy = my;
                                            mmx2 = sht->vx0;
                                            new_wy = sht->vy0;
                                        }
                                        /* 鼠标在窗口关闭按钮上按下左键 */
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                                            /* 若在应用程序窗口点击关闭按钮,则修改该应用程序窗口
                                             * 任务的eax和eip值,以当该应用程序被调度运行时退出。
                                             * 应用程序窗口在应用程序中通过系统调用创建,见 api_openwin()。*/
                                            if ((sht->flags & 0x10) != 0) {
                                                task = sht->task;
                                                cons_putstr0(task->cons, "\nBreak(mouse) :\n");
                                                io_cli();
                                                task->tss.eax = (int) &(task->tss.esp0);
                                                task->tss.eip = (int) asm_end_app;
                                                io_sti();
                                                task_run(task, -1, 0);

                                            /* 若点击了内核命令行窗口关闭按钮,则暂将控制窗口隐藏不显示, 其
                                             * 内存资源由命令行窗口任务发送[768, 2279]命令给主任务时再释放。*/
                                            } else {
                                                task = sht->task;
                                                sheet_updown(sht, -1);
                                                /* 高亮下一个命令行窗口标题区域 */
                                                keywin_off(key_win);
                                                key_win = shtctl->sheets[shtctl->top - 1];
                                                keywin_on(key_win);
                                                /* 往控制窗口发送退出窗口命令 */
                                                io_cli();
                                                fifo32_put(&task->fifo, 4);
                                                io_sti();
                                            }
                                        }
                                        break;
                                    }
                                }
                            }

                        /* 处理在窗口移动区域按下左键移动窗口的情况 */
                        } else {
                            /* 计算移动量,并更新窗口坐标 */
                            x = mx - mmx;
                            y = my - mmy;
                            new_wx = (mmx2 + x + 2) & ~3;
                            new_wy = new_wy + y;
                            mmy = my;
                        }

                    /* 处理没有按下左键的情况 */
                    } else {
                        mmx = -1; /* 记录鼠标没有按下左键 */
                        /* 将窗口移动到鼠标移动位置处,并标识窗口已移动过一次 */
                        if (new_wx != 0x7fffffff) {
                            sheet_slide(sht, new_wx, new_wy);
                            new_wx = 0x7fffffff;
                        }
                    }
                }
            /* task_a 任务循环队列中i=[768, 1023]范围数据段由命令行窗口发送而来, 见
             * cmd_exit()系列函数,他们的含义为释放第(i-768)窗口及其所关联任务的资源。*/
            } else if (768 <= i && i <= 1023) {
                close_console(shtctl->sheets0 + (i - 768));
            /* task_a 任务循环队列中i=[1024, 2023]范围数据段由命令行窗口发送
             * 而来,见cmd_exit()系列函数,他们的含义为关闭第(i-1024)个任务。*/
            } else if (1024 <= i && i <= 2023) {
                close_constask(taskctl->tasks0 + (i - 1024));
            /* 释放第(i-2024)的窗口画面信息缓冲区和管理该窗口结构体内存资源 */
            } else if (2024 <= i && i <= 2279) {
                sht2 = shtctl->sheets0 + (i - 2024);
                memman_free_4k(memman, (int) sht2->buf, 256 * 165);
                sheet_free(sht2);
            }
        }
    }
}

/* keywin_off,
 * 置灰key_win所指命令行窗口标题以表其不在屏幕顶层即
 * 当前没有被选中,并向该命令行窗口发送隐藏光标的命令。*/
void keywin_off(struct SHEET *key_win)
{
    change_wtitle8(key_win, 0);
    if ((key_win->flags & 0x20) != 0) {
        fifo32_put(&key_win->task->fifo, 3);
    }
    return;
}

/* keywin_on,
 * 高亮key_win所指命令行窗口标题以标识其在屏幕顶层即
 * 被选中的状态,并向该命令行窗口发送显示光标的命令。*/
void keywin_on(struct SHEET *key_win)
{
    change_wtitle8(key_win, 1);
    if ((key_win->flags & 0x20) != 0) {
        fifo32_put(&key_win->task->fifo, 2);
    }
    return;
}

/* open_constask，
 * 为sht对应内核态命令行窗口关联任务,关联循环队列。
 * 该函数将返回管理命令行窗口任务的结构体基址,命令行窗口任务运行在内核态。*/
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
    /* 获取管理内存结构体基址;获取用于管理任务的空闲
     * 结构体基址;分配内存用作命令行窗口任务循环队列。*/
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_alloc();
    int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);

    task->cons_stack = memman_alloc_4k(memman, 64 * 1024); /* 64Kb栈内存 */
    task->tss.esp = task->cons_stack + 64 * 1024 - 12; /* 设置栈顶位置 */
    task->tss.eip = (int) &console_task; /* 任务程序代码起始地址,任务函数 */
    task->tss.es = 1 * 8; /* 任务数据段选择符 */
    task->tss.cs = 2 * 8; /* 任务代码段选择符 */
    task->tss.ss = 1 * 8; /* 任务数据段选择符 */
    task->tss.ds = 1 * 8; /* 任务数据段选择符 */
    task->tss.fs = 1 * 8; /* 任务数据段选择符 */
    task->tss.gs = 1 * 8; /* 任务数据段选择符 */
    /* 在设置任务栈顶时预留了8字节,这8个字节分别用来存储管
     * 理命令行窗口画面信息的结构体基址和扩展可用内存大小。*/
    *((int *) (task->tss.esp + 4)) = (int) sht;
    *((int *) (task->tss.esp + 8)) = memtotal;
    /* 将命令行窗口任务加入到任务层级2中,当任务层
     * 0,1中无任务时任务层级2中任务将会被调度运行。*/
    task_run(task, 2, 2); /* level=2, priority=2 */
    /* 设置命令行窗口循环队列所关联任务和队列长度 */
    fifo32_init(&task->fifo, 128, cons_fifo, task);
    return task;
}

/* open_console,
 * 新建内核态命令行窗口。包括
 * 缓存命令行窗口画面信息,关联命令行窗口任务,关联窗口任务循环队列。
 *
 * 由 open_console() 创建的命令行窗口运行在内核态。
 * 用户态的命令行窗口需由应用程序通过系统调用创建。
 * =============================================== */
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
    /* 获取管理内存的结构体基址;获取管理窗口画面信息的
     * 空闲结构体基址;为命令行窗口画面信息分配缓冲区。*/
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHEET *sht = sheet_alloc(shtctl);
    unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 256 * 165);

    /* 初始化管理命令行窗口画面信息的结构体sht,命令行窗口为大小为(256 * 165)像素点 */
    sheet_setbuf(sht, buf, 256, 165, -1);

    /* 绘制命令行窗口画面信息并缓存在buf所指内存段中 */
    make_window8(buf, 256, 165, "console", 0);

    /* 命令行窗口文本框在窗口[(8,28), (8+240,28+128)]区域,
     * 将命令行窗口文本框的画面信息缓存在buf相应内存段中。*/
    make_textbox8(sht, 8, 28, 240, 128, COL8_000000);

/* 至此,整个命令行窗口的画面信息(即调色板中的色号)都缓存在sht->buf所指内存中了。
 * 将sht->buf所指内存从显存地址(vaddr + y * xsize + x)连续写入时,该命令行窗口就
 * 会从屏幕像素点位置(x,y)处开始显示。其中,vaddr为显存起始地址,xsize为屏幕x像素。*/

    /* 为命令行窗口关联任务 */
    sht->task = open_constask(sht, memtotal);
    sht->flags |= 0x20; /* 标识命令行窗口含光标 */
    return sht;
}

/* close_constask,
 * 释放task所指结构体所管理的内存资源。*/
void close_constask(struct TASK *task)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    task_sleep(task);
    memman_free_4k(memman, task->cons_stack, 64 * 1024);
    memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
    task->flags = 0; /* 置task元素空闲 */
    return;
}

/* close_console,
 * 释放sht所管理内核态窗口画面所占内存资源,释放sht所管理窗口任务内存资源。*/
void close_console(struct SHEET *sht)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = sht->task;
    memman_free_4k(memman, (int) sht->buf, 256 * 165);
    sheet_free(sht);
    close_constask(task);
    return;
}
