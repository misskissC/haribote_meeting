/* 应用程序 winhelo.c,新建标题为hello应用程序窗口 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    int win;
    char buf[150 * 50];
    /* 新建150*150大小标题为hello无透明色窗口,
     * 在键入回车时退出。*/
    win = api_openwin(buf, 150, 50, -1, "hello");
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
