/* 应用程序 color.c,在窗口指定区域填充指定颜色 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    char *buf;
    int win, x, y, r, g, b;

    /* 内存分配初始化;分配144*164字节内存作为窗口画面信息缓冲区;
     * 打开144*164大小标题为color无透明色的窗口。*/
    api_initmalloc();
    buf = api_malloc(144 * 164);
    win = api_openwin(buf, 144, 164, -1, "color");

    /* 用指定颜色填充win所管理窗口[(8,28),(136,156)]区域画面信息 */
    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            r = x * 2;
            g = y * 2;
            b = 0;
            buf[(x + 8) + (y + 28) * 144] = 16 + (r / 43) + (g / 43) * 6 + (b / 43) * 36;
        }
    }
    /* 刷新win所管理窗口[(8,28),(136,156)]   区域画面信息 */
    api_refreshwin(win, 8, 28, 136, 156);

    /* 敲任意键退出本应用程序 */
    api_getkey(1);
    api_end();
}
