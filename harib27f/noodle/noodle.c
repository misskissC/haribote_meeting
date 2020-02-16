/* 应用程序 noodle.c,在新建应用窗口中计时 */

/* 标准库头文件;系统调用声明头文件 */
#include <stdio.h>
#include "apilib.h"

void HariMain(void)
{
    char *buf, s[12];
    int win, timer, sec = 0, min = 0, hou = 0;

    /* 初始化应用内存分配;分配150*150字节内存用作窗口画面缓冲区;
     * 打开150*150大小标题为noodle无透明色窗口;分配并初始化定时器。*/
    api_initmalloc();
    buf = api_malloc(150 * 50);
    win = api_openwin(buf, 150, 50, -1, "noodle");
    timer = api_alloctimer();
    api_inittimer(timer, 128);
    
    for (;;) {
        sprintf(s, "%5d:%02d:%02d", hou, min, sec);
        /* win所管理窗口[(28,27),(115,41)]区域绘制色号为7的矩形 */
        api_boxfilwin(win, 28, 27, 115, 41, 7);
        /* 从(28,27)处以色号0输出s所指字符串 */
        api_putstrwin(win, 28, 27, 0, 11, s);
        /* 设置定时器超时时间为1s,任意键可退出本循环 */
        api_settimer(timer, 100);
        if (api_getkey(1) != 128) {
            break;
        }
        /* 更新秒,分,时 */
        sec++;
        if (sec == 60) {
            sec = 0;
            min++;
            if (min == 60) {
                min = 0;
                hou++;
            }
        }
    }
    /* 任意键退出循环后退出本应用程序 */
    api_end();
}
