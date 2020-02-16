/* 应用程序 sosu3.c,在当前终端显示[1,10000]间的素数 */

/* 标准库头文件;系统调用声明头文件 */
#include <stdio.h>
#include "apilib.h"

#define MAX 10000

void HariMain(void)
{
    char *flag, s[8];
    int i, j;

    /* 初始化内存管理;分配MAX字节堆内存 */
    api_initmalloc();
    flag = api_malloc(MAX);
    
    for (i = 0; i < MAX; i++) {
        flag[i] = 0;
    }
    for (i = 2; i < MAX; i++) {
        if (flag[i] == 0) {
            sprintf(s, "%d ", i);
            api_putstr0(s);

            /* 标记当前素数的倍数不予显示 */
            for (j = i * 2; j < MAX; j += i) {
                flag[j] = 1;
            }
        }
    }
    /* 结束应用程序 */
    api_end();
}
