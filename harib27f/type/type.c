/* 应用程序 type.c,将指定文件内容输出到当前终端 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    int fh;
    char c, cmdline[30], *p;

    /* 获取当前任务的输入 */
    api_cmdline(cmdline, 30);
    for (p = cmdline; *p > ' '; p++) { }
    for (; *p == ' '; p++) { }

    /* 打开指定文件并读取其内容输出到当前终端上后退出应用程序 */
    fh = api_fopen(p);
    if (fh != 0) {
        for (;;) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            api_putchar(c);
        }
    } else {
        api_putstr0("File not found.\n");
    }
    api_end();
}
