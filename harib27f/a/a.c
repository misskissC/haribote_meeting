/* 应用程序 a.c,在执行本引用程序的命令行窗口中打印字符 'A' */

/* 系统调用头文件 */
#include "apilib.h"

/* 应用程序入口 */
void HariMain(void)
{
    /* 调用系统调用打印字符 'A',调用系统调用退出应用程序。*/
    api_putchar('A');
    api_end();
}
