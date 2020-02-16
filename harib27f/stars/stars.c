/* 应用程序 stars.c, 在新建窗口指定区域随机描绘点 */

/* 系统调用声明头文件 */
#include "apilib.h"

/* 声明C标准库中的 rand()函数 */
int rand(void);

void HariMain(void)
{
    char *buf;
    int win, i, x, y;

    /* 初始化应用内存分配;分配150*100字节用作窗口画面缓冲区;
     * 新建150*100大小标题为stars无透明色的窗口;
     * 在窗口[(6,26),(143,93)]区域绘制色号为0的矩形。*/
    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "stars");
    api_boxfilwin(win,  6, 26, 143, 93, 0);

    /* 在矩形区域随机位置描绘50个色号为3的点(星星) */
    for (i = 0; i < 50; i++) {
        x = (rand() % 137) +  6;
        y = (rand() %  67) + 26;
        api_point(win, x, y, 3;
    }
    /* 回车结束本应用程序 */
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
