/* 应用程序 color2.c,在窗口指定区域填充指定颜色 */

/* 系统调用声明头文件 */
#include "apilib.h"

unsigned char rgb2pal(int r, int g, int b, int x, int y);

void HariMain(void)
{
    char *buf;
    int win, x, y;

    /* 初始化应用程序内存管理;分配144*164字节内存用作窗口画面信息缓冲区;
     * 打开144*164标题为color2无透明色的应用窗口。*/
    api_initmalloc();
    buf = api_malloc(144 * 164);
    win = api_openwin(buf, 144, 164, -1, "color2");

    /* 用 rgb2pal() 子程序指定颜色填充窗口[(8,28),(136,156)]区域画面信息 */
    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            buf[(x + 8) + (y + 28) * 144] = rgb2pal(x * 2, y * 2, 0, x, y);
        }
    }
    /* 刷新窗口[(8,28),(136,156)]区域画面信息,按任意键可结束本应用程序 */
    api_refreshwin(win, 8, 28, 136, 156);
    api_getkey(1);
    api_end();
}

/* 用于计算颜色号的子程序(不关注计算细节啦) */
unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
    static int table[4] = { 3, 1, 0, 2 };
    int i;
    x &= 1;
    y &= 1;
    i = table[x + y * 2];
    r = (r * 21) / 256;
    g = (g * 21) / 256;
    b = (b * 21) / 256;
    r = (r + i) / 4;
    g = (g + i) / 4;
    b = (b + i) / 4;
    return 16 + r + g * 6 + b * 36;
}
