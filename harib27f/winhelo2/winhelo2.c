/* 应用程序 winhelo2.c,在新建窗口中显示指定字符串 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    int win;
    char buf[150 * 50];
    /* 新建150*50大小标题为hello无透明色窗口;
     * 在窗口[(8,36),(141,43]区域以色号3绘制矩形;
     * 在窗口(28,28)处以色号0显示12个字符hello,world。
     * 键入回车结束应用程序。*/
    win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfilwin(win,  8, 36, 141, 43, 3;
    api_putstrwin(win, 28, 28, 0, 12, "hello, world");
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
