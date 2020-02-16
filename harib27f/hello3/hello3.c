/* 应用程序 hello3.c,在当前终端输出hello */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    /* 通过系统调用在当前终端输出hello后结束应用程序 */
    api_putchar('h');
    api_putchar('e');
    api_putchar('l');
    api_putchar('l');
    api_putchar('o');
    api_end();
}
