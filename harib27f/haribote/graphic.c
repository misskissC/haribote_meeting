/* graphic.c, 将像素点阵转换为显存中的画面信息(调色板色号)的程序接口 */

#include "bootpack.h"

/* 粗略理解DAC调色板,显存内存地址空间和屏幕像素点坐标对应关系。
 * 
 * 在asmhead.nas中通过VBE设置的SuperVGA显示模式(1024 x 768 x 8)支持DAC调色板。
 * 1024和768分别表示显卡在横向和纵向所支持的像素点数,8表示每个像素点支持8位色
 * (256种颜色)。依靠调色板,一种RGB颜色可用调色板号(色号)来指定。
 * PLT_nr    RGB_color
 *       |--------------|
 *   0   | R0 | G0 | B0 |
 *       |--------------|
 *   1   | R1 | G1 | B1 |
 *       |--------------|
 *  ...  | .. | .. | .. |
 *       |--------------|
 *  255  |R255|G255|B255|
 *       |--------------|
 * 往调色板中设定RGB颜色后,可用调色板中的色号指定RGB,如色号0将指定RGB
 * "R0G0B0".
 * 
 * 往1字节显存中写入色号nr,该字节显存所对应的屏幕像素点就会呈现色号
 * nr对应的RGB_color颜色。
 * |==========================================================|
 * |   |----|       ----------------------------------------  |
 * |   |----|       |(0,0)    | (0,1)    |(...) |(0, 767)  |  |
 * |   |----|       |--------------------------------------|  |
 * |   |----|       |(1,0)    | (1,1)    |(...) |(1, 767)  |  |
 * |   |----|       |--------------------------------------|  |
 * |   |----|       |(...,0)  | (...,1)  |(m,k) |(..., 767)|  |
 * |   |----|       |--------------------------------------|  |
 * |   |----|       |(1023,0) | (1023,1) |(...) |(1023,767)|  |
 * |   |----|       ----------------------------------------  |
 * |vram addr space                screen                     |
 * |==========================================================|
 * 屏幕像素点(m,k)对应的显存地址为 vram_base_addr + k * 768 + m.
 * 通过使用调色板,可以通过一字节显存指定屏幕像素点的RGB颜色。*/


/* 关于像素点阵的表示。
 * 
 * 往显存地址(vram_base_addr + y * xsize + x)写入颜色号cr,
 * 屏幕(x,y)处像素点会呈色号cr对应RGB颜色。若需在屏幕(x,y)
 * 像素点处以色号cr开始显示字符'A'
 * |——————————————————————|
 * |    (x,y)             |
 * |      ........        |
 * |      ...**...        |
 * |      ...**...        |
 * |      ...**...        |
 * |      ...**...        |
 * |      ..*..*..        |
 * |      ..*..*..        |
 * |      ..*..*..        |
 * |      ..*..*..        |
 * |      .******.        |
 * |      .*....*.        |
 * |      .*....*.        |
 * |      .*....*.        |
 * |      ***..***        |
 * |      ........        |
 * |      ........        |
 * |______________________| (假设屏幕就这么大^_^)
 * 
 * 则应往屏幕上'*'对应显存中写入色号cr即可显示'A'(cr与屏幕背景色号不同)。
 * 
 * 可用如下C程序实现该功能:
 * char font[16 * 8 + 1] = {
 *     "........"
 *     "...**..."
 *     "...**..."
 *     "...**..."
 *     "...**..."
 *     "..*..*.."
 *     "..*..*.."
 *     "..*..*.."
 *     "..*..*.."
 *     ".******."
 *     ".*....*."
 *     ".*....*."
 *     ".*....*."
 *     "***..***"
 *     "........"
 *     "........"
 * };
 * for (r = 0; r < 16; ++r)
 *     for (c = 0; c < 8; ++c)
 *         if (font[r * 8 + c] == '*')
 *             vaddr[(y + r) * xsize + x + c] = cr;
 * 如此,显存中就包含了起始于屏幕(x,y)像素点处的字符'A'画面信息(色号),
 * 即实现在屏幕(x,y)像素点处开始以cr色号对应RGB显示字符'A'。
 * 
 * 若需节约内存,可用二进制位表示字符'A'的像素点阵。
 * "........" -> 0 0 0 0 0 0 0 0 -> 0x00
 * "...**..." -> 0 0 0 1 1 0 0 0 -> 0x18
 * "...**..." -> 0 0 0 1 1 0 0 0 -> 0x18
 * "...**..." -> 0 0 0 1 1 0 0 0 -> 0x18
 * "...**..." -> 0 0 0 1 1 0 0 0 -> 0x18
 * "..*..*.." -> 0 0 1 0 0 1 0 0 -> 0x24
 * "..*..*.." -> 0 0 1 0 0 1 0 0 -> 0x24
 * "..*..*.." -> 0 0 1 0 0 1 0 0 -> 0x24
 * "..*..*.." -> 0 0 1 0 0 1 0 0 -> 0x24
 * ".******." -> 0 1 1 1 1 1 1 0 -> 0x7e
 * ".*....*." -> 0 1 0 0 0 0 1 0 -> 0x42
 * ".*....*." -> 0 1 0 0 0 0 1 0 -> 0x42
 * ".*....*." -> 0 1 0 0 0 0 1 0 -> 0x42
 * "***..***" -> 1 1 1 0 0 1 1 1 -> 0xe7
 * "........" -> 0 0 0 0 0 0 0 0 -> 0x00
 * "........" -> 0 0 0 0 0 0 0 0 -> 0x00
 * 
 * 如此,实现在屏幕(x,y)像素点处显示'A'的C程序可变为
 * char font[16] = {
 *     0x00,0x18,0x18,0x18,0x18,0x24,0x24,0x24
 *     0x24,0x7e,0x42,0x42,0x42,0xe7,0x00,0x00
 * };
 * for (r = 0; r < 16; ++r)
 *     if (font[r] & 0x80)
 *         vram[(y + r) * xsize + 0] = cl;
 *     if (font[r] & 0x40)
 *         vram[(y + r) * xsize + 1] = cl; *     
 *     // ...to check every bit of the current byte...
 *
 *     if (font[r] & 0x01)
 *         vram[(y + r) * xsize + 7] = cl;
 *
 * 两个程序往显存中写入的画面信息是相同的,后者节约了内存, 按照16*8的像素点阵来算,
 * 后者在一个字符中能节约(16 * 8 + 1) - 16 = 113个字节。字库中各字符的像素点阵就
 * 是采用的二进制信息表示方法。在使用字库时,需要用字符编码值去字库中索引其像素点阵。*/



/* init_palette,
 * 为DAC调色板指定RGB颜色。*/
void init_palette(void)
{
    /* 抽取16种基础RGB颜色(可以不用static修饰) */
    static unsigned char table_rgb[16 * 3] = {
        /*Red,Green,Blue*/
        0x00, 0x00, 0x00, /*  0:黑色 */
        0xff, 0x00, 0x00, /*  1:红色 */
        0x00, 0xff, 0x00, /*  2:绿色 */
        0xff, 0xff, 0x00, /*  3:黄色 */
        0x00, 0x00, 0xff, /*  4:蓝色 */
        0xff, 0x00, 0xff, /*  5:紫红 */
        0x00, 0xff, 0xff, /*  6:青色 */
        0xff, 0xff, 0xff, /*  7:白色 */
        0xc6, 0xc6, 0xc6, /*  8:亮灰 */
        0x84, 0x00, 0x00, /*  9:暗红 */
        0x00, 0x84, 0x00, /* 10:暗绿 */
        0x84, 0x84, 0x00, /* 11:暗黄 */
        0x00, 0x00, 0x84, /* 12:暗蓝 */
        0x84, 0x00, 0x84, /* 13:暗紫 */
        0x00, 0x84, 0x84, /* 14:暗白 */
        0x84, 0x84, 0x84  /* 15:暗灰 */
    };
    /* 指定调色板前16种颜色;0-黑色,... */
    set_palette(0, 15, table_rgb);

    /* 将RGB中的每种颜色都划分成6个色阶即6等份[0, 1*51,..., 5*51],
     * 然后依次将他们设置在色号[16,231]中。如此又得到216种RGB所描
     * 述的颜色, 加上调色板中前16种基础色,现调色板中一共有232种RGB颜色。*/
    unsigned char table2[216 * 3];
    int r, g, b;
    for (b = 0; b < 6; b++) {
        for (g = 0; g < 6; g++) {
            for (r = 0; r < 6; r++) {
                table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
                table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
                table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
            }
        }
    }
    set_palette(16, 231, table2);
    return;
}

/* set_palette,
 * 用rgb所指内存中包含的RGB颜色依次设置在调色板号[start, end]中。*/
void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;

    /* 备份EFLAG到eflags变量中,然后设置EFLAG禁止CPU处理本进程中断 */
    eflags = io_load_eflags();
    io_cli();

    /* 将起始调色板号写入调色板3c8h端口, 然后将每一组rgb依次写入调色板
     * 3c9h端口即完成一次调色板号的设置,除以4只是将RGB颜色变浅/深了一点。*/
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    /* 恢复EFLAG允许CPU处理当前进程中断 */
    io_store_eflags(eflags);
    return;
}

/* boxfill8,
 * 用色号c充当窗口[(x0,y0),(x1,y1)]区域画面信息,窗口x方向像素点数为xsize。
 * |===========================|
 * | (x0,y0)|-------|          |
 * |        |ccccccc|          |
 * |        |-------|(x1,y1)   |
 * |                           |
 * |===========================|
 * |<-----window xsize-------->|
 * 
 * vram, 用于缓存窗口的画面信息;
 * [(x0,y0),(x1,y1)], 基于窗口左上角的窗口区域;
 * c, 填充窗口[(x0,y0),(x1,y1)]区域的色号。
 *
 * 当vram为显存基址,xsize为屏幕x箱数点时,屏幕[(x0,y0),(x1,y1)]区域将会直接显示色号c对应的RGB颜色。*/
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;

    /* 将色号c写入窗口像素(x,y)处所对应缓存中 */
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    }
    return;
}

/* init_screen8,
 * 将自制屏幕背景窗口画面信息缓存到vram所指内存中。
 * x,y分别为屏幕横向(x)和纵向(y)的像素点数。*/
void init_screen8(char *vram, int x, int y)
{
    boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
    boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
    boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
    boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

    boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
    boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
    boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
    boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
    boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
    boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
    boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
    boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);

/* 自制屏幕背景窗口画面大体如下(每个十六进制数代表像素点的色号,':'表像素点省略部分)。
 * |——————————————————————————————————————————————————————————————————|
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE |
 * |88888888888888888888888888888888888888888888888888888888888888888 |
 * |77777777777777777777777777777777777777777777777777777777777777777 |
 * |8FFFFFFFFFF08888888888888888888888888888888888888FFFFFFFFF7888888 |
 * |8FFFFFFFFFF08888888888888888888888888888888888888FFFFFFFFF7888888 |
 * |8FFFFFFFFFF08888888888888888888888888888888888888FFFFFFFFF7888888 |
 * |80000000000088888888888888888888888888888888888887777777777888888 |
 * |88888888888888888888888888888888888888888888888888888888888888888 |
 * |                                                                  |
 * |——————————————————————————————————————————————————————————————————| */

    return;
}

/* putfont8,
 * 将font所指像素点阵用c充当画面信息,
 * 并将画面信息从vram所指画面的(x,y)像素点位置处开始缓存。
 * 
 * font所指向的16字节是一个字符的像素点阵。
 * 如字符'A'的像素点阵
 * 0 0 0 0 0 0 0 0 -> 0x00
 * 0 0 0 1 1 0 0 0 -> 0x18
 * 0 0 0 1 1 0 0 0 -> 0x18
 * 0 0 0 1 1 0 0 0 -> 0x18
 * 0 0 0 1 1 0 0 0 -> 0x18
 * 0 0 1 0 0 1 0 0 -> 0x24
 * 0 0 1 0 0 1 0 0 -> 0x24
 * 0 0 1 0 0 1 0 0 -> 0x24
 * 0 0 1 0 0 1 0 0 -> 0x24
 * 0 1 1 1 1 1 1 0 -> 0x7e
 * 0 1 0 0 0 0 1 0 -> 0x42
 * 0 1 0 0 0 0 1 0 -> 0x42
 * 0 1 0 0 0 0 1 0 -> 0x42
 * 1 1 1 0 0 1 1 1 -> 0xe7
 * 0 0 0 0 0 0 0 0 -> 0x00
 * 0 0 0 0 0 0 0 0 -> 0x00
 * 即font中的内容为
 * char font[] = {
 *      0x00,0x18,0x18,0x18,0x18,0x24,0x24,0x24,
 *      0x24,0x7e,0x42,0x42,0x42,0xe7,0x00,0x00
 * };
 * 
 * putfont8依次遍历font中的每一位,当遍历到bit位为1时,
 * 则将指定颜色号c写入vram所指内存的对应字节上。*/
void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
    int i;
    char *p, d /* data */;

    /* 见"关于像素点阵的表示" */
    for (i = 0; i < 16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0) { p[0] = c; }
        if ((d & 0x40) != 0) { p[1] = c; }
        if ((d & 0x20) != 0) { p[2] = c; }
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }
    return;
}

/* putfonts8_asc,
 * 按照当前进程的语言模式(英文,日文等),根据s所指字符编码值
 * 从相应字库中取出字符的像素点阵, 将其转换为画面信息并从
 * vram所指画面(x,y)像素点处开始缓存;直到s所指字符编码值为
 * 0时结束。
 *
 * 如'A'编码值 --> 字符'A'在字库中像素点阵
 * 65  --------->  0x00    0 0 0 0 0 0 0 0
 *                 0x18    0 0 0 1 1 0 0 0
 *                 0x18    0 0 0 1 1 0 0 0
 *                 0x18    0 0 0 1 1 0 0 0
 *                 0x18    0 0 0 1 1 0 0 0
 *                 0x24    0 0 1 0 0 1 0 0
 *                 0x24    0 0 1 0 0 1 0 0
 *                 0x24    0 0 1 0 0 1 0 0
 *                 0x24    0 0 1 0 0 1 0 0
 *                 0x7e    0 1 1 1 1 1 1 0
 *                 0x42    0 1 0 0 0 0 1 0
 *                 0x42    0 1 0 0 0 0 1 0
 *                 0x42    0 1 0 0 0 0 1 0
 *                 0xe7    1 1 1 0 0 1 1 1
 *                 0x00    0 0 0 0 0 0 0 0
 *                 0x00    0 0 0 0 0 0 0 0
 * 首先根据字符编码在字库中找到字符的像素点阵,然后根据字符像素点阵得到字符
 * 画面信息(色号c),最后将字符画面信息从vram所指画面(x,y)像素点处开始缓存。*/
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
    /* hankaku是hankaku.txt中各符号像素点阵开始处标号(_hankaku)。由于hankaku.txt中只有
     * 256个英文字符,其对应的像素点阵所占内存比较小(256 x 16 bytes),所以该部分像素点阵
     * 被链接到haribote.sys中成为操作系统的一部分。欲在程序中使用hankaku.txt中字符的像
     * 素点阵时,先将hankaku当做全局变量标识符先声明一下,再根据字符编码值索引其像素点阵。*/
    extern char hankaku[4096];
    /* 当程序运行到此处时,所获取到任务结构体为管理当前程序任务的结构体 */
    struct TASK *task = task_now();

    /* 内存地址0xfe8处存储了 nihongo.fnt 字库文件在内存中的基址(见HariMain)。FAT12映像
     * 软盘除了包含haribote.sys操作系统程序文件外,还包含一些其他文件,如字库文件,应用程
     * 序文件等。字库文件nihongo由操作系统启动后从FAT12映像盘的缓冲区读入,内存段
     * [100000h, 267fffh]缓存了映像软盘内容(见asmhead.nas)。*/
    char *nihongo = (char *) *((int *) 0x0fe8), *font;
    int k, t;

/* 从字符库中获取s所指字符(编码值)的像素点阵信息,并将其画面信息缓存在vram所指画面的
 * (x,y)像素点处。如果不关心字符像素点阵在字符库中的组织方式(编码方式),可略过计算各
 * 字符像素点阵信息在字符库中位置的代码,如字符面,区,点的计算。*/

    /* 从英文字库中获取s所指字符的像素点阵,
     * 并将其对应的画面信息缓存在vram所指画面的(x,y)像素点位置处 */
    if (task->langmode == 0) {
        /* hankaku按照字符编码值[0, 255]升序顺序依次包含了各字符的像素点阵。
         * 因为每个字符像素点阵占16字节,所以(*s * 16)得到字符(*s)在hankaku
         * 内存段中的像素点阵。putfont8将(*s)像素点阵的画面信息存于vram所指
         * 画面的(x,y)像素点位置处。*/
        for (; *s != 0x00; s++) {
            putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
            x += 8;
        }
    }

/* 日文字符(文字)分半角字符和全角字符,半角字符指其像素点阵为16 x 8,其字符编码值
 * 范围为[0x00,0x7f],其编码值可直接作为字符在字符库中像素点阵的索引(同hankaku);
 * 全角字符指其像素点阵信息为16 x 16,其字符编码值占2字节,这两字节以面,区,点定位
 * 字符像素点阵在字符库中的位置,第一个字节编码值用于描述字符像素点阵在字符库中的
 * 面和区,第二字节编码值用于描述字符像素点阵在字符库中的区和点。*/

    /* 获取s所指字符在日文字库中的画面信息 */
    if (task->langmode == 1) {
        for (; *s != 0x00; s++) {
            /* langbyte1=0: 刚解析完上一个字符编码或还未处理当前字符编码 */
            if (task->langbyte1 == 0) {
                /* 根据第1字节编码判断该字符为全角字符 */
                if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
                    task->langbyte1 = *s; /* 记录全角字符像素点阵的面和区 */

                /* 根据半角字符编码值在字库中索引到像素点阵,
                 * 将其转换为画面信息存于vram所指画面的(x,y像素点处) */
                } else {
                    putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
                }
            } else { /* 全角字符第2字节编码 */
                /* 根据记录在langbyte1中的第1字节编码值计算字符像素点阵的面和区号 */
                if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) { /* 1面 */
                    k = (task->langbyte1 - 0x81) * 2; /* k=1面中区号 */
                } else {/* 2面 */
                    k = (task->langbyte1 - 0xe0) * 2 + 62; /* k=2面中区号 */
                }
                /* 根据字符第2字节编码值计算字符像素点阵所在区的点号 */
                if (0x40 <= *s && *s <= 0x7e) {
                    t = *s - 0x40;
                } else if (0x80 <= *s && *s <= 0x9e) {
                    t = *s - 0x80 + 63;
                } else {
                    t = *s - 0x9f;
                    k++; /* 较大区号 */
                }
                /* 获取到1个全角字符像素点阵面区点信息后复位0 */
                task->langbyte1 = 0;
                
                /* 256 * 16 为半角字符像素点阵在字库中所占字节数;
                 * k为全角字符像素点阵区号,每个区共有94个全角字符,
                 * t为全角字符像素点阵在k区中的点号,字库前面字符像
                 * 素点阵所占字节数为(k * 94 + t) * 32。font最终指
                 * 向当前字符在字库中的像素点阵。*/
                font = nihongo + 256 * 16 + (k * 94 + t) * 32;
                putfont8(vram, xsize, x - 8, y, c, font     );
                putfont8(vram, xsize, x    , y, c, font + 16);
            }
            /* 当获取一个半角字符的像素点阵后,x方向像素点将增长8个像素点,
             * 当获取一个全角字符的像素点阵号,由于全角字符有两个字节编码
             * 值,所以该语句会被执行两次即x方向像素点会增长16个像素点。*/
            x += 8;
        }
    }

    /* s所指字符在日文EUC模式下的画面信息 */
    if (task->langmode == 2) {
        for (; *s != 0x00; s++) {
            if (task->langbyte1 == 0) { /* 已处理全角或半角字符编码值 */
                if (0x81 <= *s && *s <= 0xfe) {
                    task->langbyte1 = *s; /* 记录全角字符第1编码值 */
                } else { /* 获取半角字符像素点阵并写入vram显示 */
                    putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
                }
            } else { /* 处理全角字符第2编码字节 */
                /* 计算全角字符像素点阵的区号和点号 */
                k = task->langbyte1 - 0xa1;
                t = *s - 0xa1;
                task->langbyte1 = 0; /* 已获取全角字符像素点阵 */

                /* 根据全角字符的像素点阵将其对应的画面
                 * 信息写入vram所指画面的(x,y)像素点处。*/
                font = nihongo + 256 * 16 + (k * 94 + t) * 32;
                putfont8(vram, xsize, x - 8, y, c, font     );
                putfont8(vram, xsize, x    , y, c, font + 16);
            }
            x += 8;
        }
    }
    return;
}
/* 结合本文件开始处的粗略知识,粗略理解hankaku.txt中各字符图标
 * 被工具软件(makefont.exe)转换为像素点阵并组合成字符库过程。
 * (nihongo.fnt字库原理同hankaku)。
 * 
 * char 0x41
 * ........                   0 0 0 0 0 0 0 0     0x00
 * ...**...                   0 0 0 1 1 0 0 0     0x18
 * ...**...                   0 0 0 1 1 0 0 0     0x18
 * ...**...                   0 0 0 1 1 0 0 0     0x18
 * ...**...   makefont.ext    0 0 0 1 1 0 0 0     0x18
 * ..*..*.. --------------->  0 0 1 0 0 1 0 0     0x24
 * ..*..*..                   0 0 1 0 0 1 0 0     0x24
 * ..*..*..                   0 0 1 0 0 1 0 0     0x24
 * ..*..*..                   0 0 1 0 0 1 0 0     0x24
 * .******.                   0 1 1 1 1 1 1 0     0x7e
 * .*....*.                   0 1 0 0 0 0 1 0     0x42
 * .*....*.                   0 1 0 0 0 0 1 0     0x42
 * .*....*.                   0 1 0 0 0 0 1 0     0x42
 * ***..***                   1 1 1 0 0 1 1 1     0xe7
 * ........                   0 0 0 0 0 0 0 0     0x00
 * ........                   0 0 0 0 0 0 0 0     0x00
 * hankaku.txt--------------->(16 * 8bit)像素点阵
 * 在得到字符像素点阵后,调用putfont8即可将字符A显示vram
 * 所指画面(x,y)像素点处。'A'编码值0x41用于在hankaku字库
 * 中索引其像素点阵。*/


/* init_mouse_cursor8,
 * 将鼠标画面信息(色号)缓存在mouse所指内存段中,
 * bc为鼠标所在背景的背景色号。*/
void init_mouse_cursor8(char *mouse, char bc)
{
    /* 鼠标像素点阵将为16(行) x 16(列), 这里可不用static */
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };
    int x, y;

    /* 鼠标边缘(*部分)使用偏黑色的色号,
     * 鼠标形状(O部分)使用偏白色的色号,
     * 鼠标剩余部分(.部分)使用背景色号。*/
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (cursor[y][x] == '*') {
                mouse[y * 16 + x] = COL8_000000;
            }
            if (cursor[y][x] == 'O') {
                mouse[y * 16 + x] = COL8_FFFFFF;
            }
            if (cursor[y][x] == '.') {
                mouse[y * 16 + x] = bc;
            }
        }
    }
    return;
}

/* putblock8_8,
 * 将buf所指内存段中的画面信息(如鼠标图标的色号)
 * 缓存在vram所指画面的[(px0, py0),(px0+pxsize, py0+pysize)]区域。*/
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize)
{
    int x, y;
    for (y = 0; y < pysize; y++) {
        for (x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}

/* 单从接口上看,graphic.c中的接口有 无参数检查的缺陷。
 * 
 * 回看之前所粗略阅读的程序接口, 他们都没有涉及参数合
 * 理性的检查。作者可能是为了减少接口代码量和提升程序
 * 性能(省CPU分支预测错误风险), 再加上无他人懵懂调用,
 * 所以免去了参数检查部分代码。
 *
 * 不过,graphic.c中的形参名vram真够具迷惑性的......尤
 * 其是在还没有阅读sheet.c,window.c以及bootpack.c的情
 * 况下。摊开这些小缺点, haribote仍旧是此文心中优秀的
 * 高综合性程序作品。*/
