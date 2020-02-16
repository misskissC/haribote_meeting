/* mouse.c, 鼠标管理程序接口 */

/* 粗略理解鼠标和CPU的连接。
 *         =====
 *         |CPU|
 *         =====
 *           ↕
 * =======================
 * |     60h    64h      |
 * |---------------------|
 * |        8042         |
 * |---------------------|
 * |状态寄存器 控制寄存器|
 * |输入寄存器 输出寄存器|
 * =======================
 *           ↕
 *          ps/2
 *           ↕
 *    ===============
 *    | 键盘 | 鼠标 |
 *    ===============
 *  
 * CPU和8042芯片通过引脚直接相连,
 * 键盘设备(芯片)和鼠标设备(芯片)
 * 通过ps/2接口和8042相连。
 * 
 * 通过端口地址,CPU可直接和8042交
 * 互数据或命令。当CPU要与键盘或鼠
 * 标交互数据时需以8042作为桥梁,当
 * 8042接收到的CPU命令是与键盘或鼠
 * 标交互时,8042便充当这个中间桥梁。*/


#include "bootpack.h"

/* 管理鼠标缓冲队列的全局指针;
 * 标识鼠标缓冲队列中数据的全局变量。*/
struct FIFO32 *mousefifo;
int mousedata0;

/* inthandler2c,
 * 鼠标中断C处理函数,读取鼠标输入到鼠标缓冲队列中。
 * 
 * 当有鼠标输入而向PIC输出中断时,
 * CPU处理PIC申请的鼠标中断时会执行IDT[0x2c]中的处
 * 理函数 _asm_inthandler2c (见int.c和dsctbl.c),该
 * 处理函数会调用此处的C处理函数 inthandler2c。*/
void inthandler2c(int *esp)
{
    int data;
    
    /* 已差不多可正确读取鼠标数据时,
     * 设定PIC,向PIC发EOI命令时结束鼠标中断 */
    io_out8(PIC1_OCW2, 0x64);
    io_out8(PIC0_OCW2, 0x62);

    /* 读取鼠标数据并将其数据添加上鼠标标识
     * 后存入鼠标缓冲队列中,供其他任务读取 */
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata0);
    return;
}

/* KEYCMD_SENDTO_MOUSE,
 * 写鼠标命令,具体的鼠标命令随后由60h端口下发;
 *
 * MOUSECMD_ENABLE,开启鼠标发送数据功能的命令。*/
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE     0xf4

/* enable_mouse,
 * 初始化键盘控制器使能鼠标并设置鼠标缓冲队列和鼠标数据标识。*/
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
    mousefifo = fifo;
    mousedata0 = data0;
    
    /* 等待键盘输入缓冲器空闲, 若键盘空闲则
     * 通过8042下发使能鼠标的命令到鼠标设备上。*/
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

    /* 使能鼠标后,鼠标将会回复0xfa的消息;
     * 置phase=0标识鼠标中断传送的信息为0xfa。*/
    mdec->phase = 0;
    return;
}

/* mouse_decode,
 * 解析鼠标数据dat,将解析结果存于mdec指向的结构体中。
 *
 * 鼠标被操作时会向PIC输出中断信号, 中断C处理函数inthandler2c
 * 在每次鼠标中断发生时就从8042输出寄存器中读取一字节鼠标数据,
 * 鼠标以3字节为一组。
 * 
 * 当每读满3字节并完成解析时mouse_decode返回1;若鼠标数据解析失
 * 败则返回-1,若还未解析满3字节则返回0。*/
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
    if (mdec->phase == 0) {
        /* 在phase=0时,表处于等待鼠标回复0xfa
         * 阶段,等到则置接收鼠标第一个字节数据阶段。*/
        if (dat == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    }
    if (mdec->phase == 1) {
        /* dat鼠标第1字节数据 */
        if ((dat & 0xc8) == 0x08) {
            /* 据作者观察(当然此文也观察过), 鼠标第1字
             * 节数据bit[3]=1,bit[7..6]=00,若鼠标第1字
             * 节数据不满足以上状态则表明 鼠标数据传递
             * 可能有误,所以在此丢弃并继续等待并解析第
             * 一字节数据。
             * 
             * bit[2..0]置位时分别代表鼠标滑轮点击, 鼠
             * 标右击和鼠标左击。bit[5..4]分别跟鼠标上
             * 下和左右移动的方向, 值为0时表示往上或右
             * 移动,值为1时表示往下或左移动。
             * 
             * 接收到第一个字节后置phase=2表示接下来接
             * 收鼠标第二字节数据。*/
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2) {
        /* 接收鼠标第二字节数据(左右移动)并置
         * phase=3以接收鼠标第三字节数据。*/
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3) {
        /* 鼠标第3字节数据接收完毕,置phase=1
         * 表继续接收下一组鼠标数据。*/
        mdec->buf[2] = dat;
        mdec->phase = 1;
    
        /* 一组鼠标数据接收完毕,开始解析。*/

        /* btn,点击事件;x,左右移动位移量;
         * y,上下移动位移量。*/
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        
        /* 若鼠标第1字节bit[5..4]皆为1,则表示鼠标分别
         * 在往下或左方向移动, 鼠标将这两个方向分别视
         * 为y和x的负方向。与此对应, 此时鼠标中断传送
         * 上来的y和x是一个负数( 的低8位)。在32位补码
         * 表示数中, 将y和x的高24位扩展为1时, 就得到y
         * 和x的负数值, 从而获得了鼠标位移的真实值。*/
        if ((mdec->buf[0] & 0x10) != 0) {
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) {
            mdec->y |= 0xffffff00;
        }
        
        /* 屏幕显示画面时原点在左上角,而鼠
         * 标移动位移量的原点在屏幕左下角。
         * 所以鼠标在y方向的位移值方向跟屏
         * 幕实际坐标方向相反,所以此处进行
         * 符号取反。*/
        mdec->y = - mdec->y;
        return 1;
    }
    return -1;
}
