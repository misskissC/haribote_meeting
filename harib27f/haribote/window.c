/* window.c, 自制窗口(画面信息)的程序接口 */

#include "bootpack.h"

/* make_window8所绘制窗口大体如下。
 * |———————————————————————————————————————————————————————————————————| - ---
 * |888888888888888888888888888888888888888...8888888888888888888888888 | ^  ^
 * |877777777777777777777777777777777777777...7777777777777777777777770 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB...BBBBBBBBBBBBBBBBBBBBBB8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBBBBBBBBBBBBBBBBBBB8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBBBBBBBBBBBBBBBBBBB8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | | 窗口标题,
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | | 关闭按钮区域
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  |
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | |  V
 * |878BBBBBBBBBBBBBBBBBBBBBTITLETITLETITLE...BBBBCCCCCCCCCCCCCCCCCC8F0 | | ---
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |....................................................................| ysize
 * |....................................................................| |
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |878888888888888888888888888888888888888...88888888888888888888888F0 | |
 * |87FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF...FFFFFFFFFFFFFFFFFFFFFFFF0 | |
 * |000000000000000000000000000000000000000...0000000000000000000000000 | |
 * |                                       ...                          | |
 * |————————————————————————————————————————————————————————————————————| |
 *                                                                        V
 * | <----------------------------------- xsize ----------------------> | - 
 * 
 * B区域,标题背景色区域[(3,3), (xsize-4,20)];
 * TITLE区域,标题区域,其像素点起始位置为(24,4);
 * C区域,关闭按钮图标区域,其像素点起始位置(xsize - 21, 5);
 * 这些区域中的色号见make_wtitle8。
 *
 * [(0,0),(xsize,ysize)]剩余部分是窗口其余部分像素点,所对应显存由调色板号填充,
 * 各调色板号对应RGB颜色见init_palette中的table_rgb数组。*/

/* make_window8,
 * 将自制窗口即x方向像素点数为xsize,
 * y方向像素点数为ysize,包含title所指标题的窗口画面信息(色号)缓存到buf所指内存中。*/
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);
    return;
}

/* make_wtitle8,
 * 将title所指标题和关闭按钮部分的画面信息(色号)缓存到buf所指画面中。act用于
 * 标识标题字体颜色及标题背景色;act=0,背景色暗灰,字体亮灰;act!=0,背景色暗蓝,字体白色。*/
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
{
    /* 用字符标识 窗口关闭按钮图标 的数组(可以不用static) */
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c, tc, tbc;

    /* 根据act选择窗口标题色号和标题背景色号;
     * 在buf所指画面的[(3,3), (xsize-4,20)]区
     * 域显示标题背景,从(24,4)处开始显示标题。*/
    if (act != 0) {
        tc = COL8_FFFFFF;
        tbc = COL8_000084;
    } else {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfonts8_asc(buf, xsize, 24, 4, tc, title);

    /* 根据窗口关闭按钮的(14 * 16)像素点阵从buf所指
     * 画面(xsize - 21, 5)位置处开始显示窗口关闭按钮。*/
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if (c == '@') {
                c = COL8_000000;
            } else if (c == '$') {
                c = COL8_848484;
            } else if (c == 'Q') {
                c = COL8_C6C6C6;
            } else {
                c = COL8_FFFFFF;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

/* putfonts8_asc_sht,
 * 在sht所指结构体管理窗口中的(x,y)处显示s所指字符,共l(变量l)个。*/
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l)
{
    struct TASK *task = task_now();

    /* 在sht->buf所指画面的区域[(x,y), (x + l * 8 - 1, y + 15)]绘制一个色号为b的窗口 */
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);

    /* 根据当前设置的语言模式,在sht->buf所指画面的(x,y)处开始写入
     * s所指向的文字的画面信息,并刷新sht->buf文字区域的画面显示。*/
    if (task->langmode != 0 && task->langbyte1 != 0) {
        putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
        sheet_refresh(sht, x - 8, y, x + l * 8, y + 16);
    } else {
        putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
        sheet_refresh(sht, x, y, x + l * 8, y + 16);
    }
    return;
}

/* make_textbox8,
 * 将自制文本框的画面信息(色号)缓存到sht->buf成员所指内存段中,
 * 使该文本框在sht->buf所指画面(窗口)的[(x0,y0), (x0+sx,y0+sy)]区域。*/
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}

/* change_wtitle8,
 * 改变sht所指结构体管理窗口的标题。
 * act=1标识sht所指窗口被选中位于屏幕最上层。*/
void change_wtitle8(struct SHEET *sht, char act)
{
    int x, y, xsize = sht->bxsize;
    char c, tc_new, tbc_new, tc_old, tbc_old, *buf = sht->buf;

    /* 窗口被选择位于屏幕最上层时窗口标题高亮 */
    if (act != 0) {
        tc_new  = COL8_FFFFFF;
        tbc_new = COL8_000084;
        tc_old  = COL8_C6C6C6;
        tbc_old = COL8_848484;
    } else {
        tc_new  = COL8_C6C6C6;
        tbc_new = COL8_848484;
        tc_old  = COL8_FFFFFF;
        tbc_old = COL8_000084;
    }

    /* 用新颜色号作为字体及字体背景区域的画面信息 */
    for (y = 3; y <= 20; y++) {
        for (x = 3; x <= xsize - 4; x++) {
            c = buf[y * xsize + x];
            if (c == tc_old && x <= xsize - 22) {
                c = tc_new;
            } else if (c == tbc_old) {
                c = tbc_new;
            }
            buf[y * xsize + x] = c;
        }
    }
    /* 刷新sht所指窗口中[(3,3),(xsize,21)]区域即标题所在区域的画面显示 */
    sheet_refresh(sht, 3, 3, xsize, 21);
    return;
}
