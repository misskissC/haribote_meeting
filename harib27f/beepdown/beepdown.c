/* 应用程序 beepdown.c,在人类能听到声音频率范围内蜂鸣 */

/* 系统调用声明头文件 */
#include "apilib.h"

void HariMain(void)
{
    int i, timer;

    /* 分配定时器,设置定时器超时数据基数为 128 */
    timer = api_alloctimer();
    api_inittimer(timer, 128);
    
    for (i = 20000000; i >= 20000; i -= i / 100) {
        /* 设置发出20KHz - 20Hz范围内的蜂鸣,声音频率每次递减1% */
        api_beep(i);

        /* 设置定时器超时为10ms,若获取到的数据不为所设置的128则退出循环 */
        api_settimer(timer, 1);
        if (api_getkey(1) != 128) {
            break;
        }
    }

    /* 关闭蜂鸣器并退出应用程序 */
    api_beep(0);
    api_end();
}
