/* 应用程序 star1.c, */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    char *buf;
    int win;

    /* 初始化应用程序内存分配;分配150*100字节内存用作窗口画面缓冲区;
     * 创建150*100大小标题为star1无透明色的窗口,在窗口
     * [(6,26),(143,93)]绘制色号为0的矩形,在窗口(75,59)处绘制色号为3的点。*/
    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "star1");
    api_boxfilwin(win,  6, 26, 143, 93, 0);
    api_point(win, 75, 59, 3);

    /* 回车退出本应用程序 */
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
