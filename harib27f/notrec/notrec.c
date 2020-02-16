/* 应用程序 notrec.c,  */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    int win;
    char buf[150 * 70];
    /* 打开150*70大小标题为notrec透明色为255的窗口,
     * 在该窗口上绘制几个矩形区域。*/
    win = api_openwin(buf, 150, 70, 255, "notrec");
    api_boxfilwin(win,   0, 50,  34, 69, 255);
    api_boxfilwin(win, 115, 50, 149, 69, 255);
    api_boxfilwin(win,  50, 30,  99, 49, 255);
    /* 回车退出本应用程序 */
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
