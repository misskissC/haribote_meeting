/* keyboard.c, 键盘管理程序接口 */

#include "bootpack.h"

/* 键盘缓冲队列全局指针,键盘缓冲队列数据的基数/标识
 * (键盘数据加上 keydata0 后发往键盘缓冲队列中)。*/
struct FIFO32 *keyfifo;
int keydata0;

/* inthandler21,
 * 键盘中断C处理函数,读取键盘输入到键盘缓冲队列中。
 * 
 * 当有键盘输入而向PIC输出中断时,
 * CPU处理PIC申请的键盘中断时会执行IDT[21h]中的入口处
 * 理程序 _asm_inthandler21 (见int.c和dsctbl.c), 该处
 * 理函数会调用此处的C处理函数 inthandler21.*/
void inthandler21(int *esp)
{
    int data;

    /* 发送EOI命令结束键盘中断 */
    io_out8(PIC0_OCW2, 0x61);
    /* 读键盘输入扫描码 data,并将其加上键盘数
     * 据基数发送到键盘队列中, 供其他任务读取。*/
    data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, data + keydata0);
    return;
}

/* PORT_KEYSTA,键盘控制器8042状态寄存器端口;
 * KEYSTA_SEND_NOTREADY,8042状态控制器输入缓冲器满的标志;
 * KEYCMD_WRITE_MODE,写入键盘控制命令字节的命令,具体的命令字节随后从60h端口写入;
 *
 * KBC_MODE, KEYCMD_WRITE_MODE的参数即具体的命令字节: 
 * 键盘以PC兼容方式运作,使用鼠和标键盘以及鼠标和键盘中断,
 * 未完成当前键盘数据时不接收下一个键盘数据等 */
#define PORT_KEYSTA          0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE    0x60
#define KBC_MODE             0x47


/* wait_KBC_sendready,
 * 等待键盘控制器8042输入缓冲器寄存器空。*/
void wait_KBC_sendready(void)
{
    /* 8042状态控制器bit[1]=0时表示其输入缓冲寄存器为空即已可接受输入 */
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

/* init_keyboard,
 * 初始化键盘控制器并设置键盘缓冲队列fifo及其数据标识data0。
 * 
 * 设置键盘控制器8042运作方式:键盘以PC兼容方式运作,在未完成
 * 当前键盘数据的接收时不接收下一个键盘输入。*/
void init_keyboard(struct FIFO32 *fifo, int data0)
{
    /* 设置键盘换成队列和键盘队列数据标识 */
    keyfifo = fifo;
    keydata0 = data0;

    /* 设置键盘运作模式,粗略含义见宏处注释 */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}
