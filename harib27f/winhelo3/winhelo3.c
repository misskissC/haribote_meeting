/* 应用程序 winhelo3.c,在新建窗口中显示指定字符串 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    char *buf;
    int win;

    /* 初始化应用程序内存分配,分配150*50字节内存用作窗口画面信息缓冲区;
     * 在窗口[(8,36),(141,43]区域以色号6绘制矩形;
     * 在窗口(28,28)处以色号0显示12个字符hello,world。
     * 键入回车结束应用程序。*/
    api_initmalloc();
    buf = api_malloc(150 * 50);
    win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfilwin(win,  8, 36, 141, 43, 6 );
    api_putstrwin(win, 28, 28, 0, 12, "hello, world");
    
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
