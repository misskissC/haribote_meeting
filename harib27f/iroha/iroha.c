/* 应用程序 iroha.c,在当前终端输出半角字符 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    /* 一段半角字符串编码,最后两个字符分别为回车和0 */
    static char s[9] = { 0xb2, 0xdb, 0xca, 0xc6, 0xce, 0xcd, 0xc4, 0x0a, 0x00 };

    /* 输出半角字符串并退出应用程序 */
    api_putstr0(s);
    api_end();
}
