/* 应用程序 hello4.c,在当前终端输出hello */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    /* 调用在当前终端打印字符串的系统调用
     * 输出hello,world\n后退出应用程序。*/
    api_putstr0("hello, world\n");
    api_end();
}
