/* 应用程序 lines.c,在新建窗口中绘制线条 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    char *buf;
    int win, i;

    /* 初始化应用程序内存分配管理;分配160*100字节内存用作新建窗口画面缓冲区;
     * 打开160*100大小标题为lines无透明色的窗口。*/
    api_initmalloc();
    buf = api_malloc(160 * 100);
    win = api_openwin(buf, 160, 100, -1, "lines");

    /* 在win所管理窗口中绘制线条(win+1作甚)并刷新相应画面区域 */
    for (i = 0; i < 8; i++) {
        api_linewin(win + 1,  8, 26, 77, i * 9 + 26, i);
        api_linewin(win + 1, 88, 26, i * 9 + 88, 89, i);
    }
    api_refreshwin(win,  6, 26, 154, 90);

    /* 当前窗口接收到回车时关闭窗口并退出应用 */
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_closewin(win);
    api_end();
}
