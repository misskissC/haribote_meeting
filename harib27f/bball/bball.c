/* 应用程序 bball.c, 绘制线条形成球状 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    int win, i, j, dis;
    char buf[216 * 237];
    struct POINT {
        int x, y;
    };
    static struct POINT table[16] = {
        { 204, 129 }, { 195,  90 }, { 172,  58 }, { 137,  38 }, {  98,  34 },
        {  61,  46 }, {  31,  73 }, {  15, 110 }, {  15, 148 }, {  31, 185 },
        {  61, 212 }, {  98, 224 }, { 137, 220 }, { 172, 200 }, { 195, 168 },
        { 204, 129 }
    };

    /* 打开一个(216*237)像素点标题为bbal无透明色的窗口;并
     * 在窗口[(8,29),(207,228)]区域填充色号0对应的黑色。*/
    win = api_openwin(buf, 216, 237, -1, "bball");
    api_boxfilwin(win, 8, 29, 207, 228, 0);

    /* 在窗口中绘制线条,
     * 点table[0]到点table[1,...,15]的线条,
     * 点table[1]到点table[2,...,15]的线条,
     * ...
     * 点table[14]到点table[15]的线条。
     * 色号从[0,7],table中的点安排合适即可形成球状。*/
    for (i = 0; i <= 14; i++) {
        for (j = i + 1; j <= 15; j++) {
            dis = j - i;
            if (dis >= 8) {
                dis = 15 - dis;
            }
            if (dis != 0) {
                api_linewin(win, table[i].x, table[i].y, table[j].x, table[j].y, 8 - dis);
            }
        }
    }

    /* 直到按下回车键时退出本应用程序 */
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }

    /* 退出应用程序 */
    api_end();
}
