/* 应用程序 chklang.c, 获取当前任务语言模式并打印 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    /* 获取当前任务语言模式 */
    int langmode = api_getlang();
    static char s1[23] = {  /* "日语JIS模式" 的编码 */
        0x93, 0xfa, 0x96, 0x7b, 0x8c, 0xea, 0x83, 0x56, 0x83, 0x74, 0x83, 0x67,
        0x4a, 0x49, 0x53, 0x83, 0x82, 0x81, 0x5b, 0x83, 0x68, 0x0a, 0x00
    };
    static char s2[17] = {  /* "日语EUC模式" 的编码 */
        0xc6, 0xfc, 0xcb, 0xdc, 0xb8, 0xec, 0x45, 0x55, 0x43, 0xa5, 0xe2, 0xa1,
        0xbc, 0xa5, 0xc9, 0x0a, 0x00
    };

    /* 根据当前任务语言模式并在当前终端给出提示 */
    if (langmode == 0) {
        api_putstr0("English ASCII mode\n");
    }
    if (langmode == 1) {
        api_putstr0(s1);
    }
    if (langmode == 2) {
        api_putstr0(s2);
    }

    /* 退出应用程序 */
    api_end();
}
