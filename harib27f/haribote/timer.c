/* timer.c, 定时器程序接口 */

#include "bootpack.h"

/* 定时器控制寄存器8254控制器端口地址和通道0计数器端口地址 */
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

/* 管理定时器的全局变量 */
struct TIMERCTL timerctl;

/* 标识管理定时器结构体已分配状态和正在使用状态 */
#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

/* init_pit,
 * 初始化定时器控制器8254,配置其发起中断请求的超时时间;
 * 并启动一个"永不超时的定时器"作为哨兵定时器。
 *
 * 永不超时定时器共包含0xffffffff个计数,haribote定时器
 * 处理函数每约10ms发生一次计数,则0xffffffff约为497年。*/
void init_pit(void)
{
    int i;
    struct TIMER *t;

    /* 将 写方式控制字34h 写入8254控制寄存器。
     * 34h: 
     * 通道计数器0以方式2工作;读/写计数器寄存器时先读/
     * 写低字节,再读/写高字节;计数器计数进制为16进制。*/
    io_out8(PIT_CTRL, 0x34);

    /* 写通道0计数器寄存器,低字节0x9c,高字节0x2e,0x2e9c=11932.
     *
     * 由计数器寄存器初值可得8254通道0计数器向8259A输出中断信
     * 号的周期T=11932 * 1/(8254计数频率), 即中断频率f=8254计
     * 数频率/11932(8254计数频率=1193.18KHz)。
     *
     * 即经以下配置后8254定时器中断频率f=1193.18KHz/11932约100Hz,
     * 即约10ms发生一次定时器中断。*/
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    
    timerctl.count = 0;
    /* 将所有定时器置为未使用状态 */
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;
    }
    
    /* 在定时器内存中分配一个空闲定时器,将其超时值设置为最大超时
     * 值(相当于永不超时),将其状态标识为正在使用的状态, 将其指向
     * 下一个定时器的指针置为0。*/
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;
    
    /* 管理定时器的全局变量使用t0指针指向定时器的第一个定时器,
     * next字段值为t0指针指向定时器的超时值。*/
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;
    return;
}

/* timer_alloc,
 * 分配一个空闲的定时器管理结构体用作定时器管理。
 * 成功则返回该空闲元素地址; 失败返回0. */
struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            timerctl.timers0[i].flags2 = 0;
            return &timerctl.timers0[i];
        }
    }
    return 0;
}

/* timer_free,
 * 释放timer指向的定时器结构体,将其状态设置为未使用状态。*/
void timer_free(struct TIMER *timer)
{
    timer->flags = 0;
    return;
}

/* timer_init,
 * 设置timer所指定时器的循环队列和超时数据。
 * 
 * 定时器超时时,将数据data发往fifo所指缓冲队列中,其他程序(任务)就可从该
 * 队列里读到data数据,从而可根据该data做出后续动作。*/
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

/* timer_settime,
 * 设置timer所指定时器的超时值为timeout(单位10ms),
 * 并将timer插入到以超时值升序排列的定时器链表中。*/
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    int e;
    struct TIMER *t, *s;

    /* 设置timer指向定时器的超时值=超时值+已计时值,
     * 再次设置timer所指定时器为正使用的状态。*/
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    /* 备份EFLAG到e,并设置EFLAG禁止CPU处理中断,
     * 调用io_store_eflags(e)即可恢复EFLAG。*/
    e = io_load_eflags();
    io_cli();

    /* timerctl.t0指向定时器链表头定时器 */
    t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
        /* 若新设置的定时器比链表头的超时值小,
         * 则将该定时器查入到链表头。*/
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    /* 以定时器超时值升序的顺序将timer插入链表中 */
    for (;;) {
        s = t;
        t = t->next;
        if (timer->timeout <= t->timeout) {
            /* 将timer插入到链表相应位置上 */
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
}

/* inthandler20,
 * 定时器中断C处理函数。
 *
 *
 * 定时器中断约每10ms会向8259A IRQ0发起一次中断,见 init_pit()。
 * 定时器中断发生时会调用注册在IDT[20h]中的 _asm_inthandler20,
 * 该程序会调用此处的定时器中断C处理函数 inthandler20 以完成中
 * 断处理,即该函数约每10ms该函数就会被调用一次。*/
void inthandler20(int *esp)
{
    struct TIMER *timer;
    char ts = 0;

    /* 初始化中断控制器8259A后(见 int.c/init_pic),
     * 再写8259A端口地址表明往8259A写OCW1-OCW3组。
     * PIC0_OCW2 && 0x60表明对OCW2编程,0x60为EOI
     * 命令,告知8259A结束此次中断。*/
    io_out8(PIC0_OCW2, 0x60);

    /* 更新系统当前已累计 计时值(单位10ms) */
    timerctl.count++;

    /* 若定时器中的最小超时值还未超时则直接返回 */
    if (timerctl.next > timerctl.count) {
        return;
    }
    
    /* 若最小超时定时器已超时,则遍历定时器链表直到遍
     * 历到非超时定时器才结束遍历。*/
    timer = timerctl.t0;
    for (;;) {
        /* 若遇到非超时定时器则退出遍历 */
        if (timer->timeout > timerctl.count) {
            break;
        }
        /* 将超时定时器状态设置为已分配状态 */
        timer->flags = TIMER_FLAGS_ALLOC;

        /* 若当前定时器不为任务定时器,则表明需定时往其队列中发送数据;
         * 若当前定时器为任务定时器,则表明当前任务运行时间完毕则置任
         * 务切换标志ts=1。*/
        if (timer != task_timer) {
            /* fifo.c */
            fifo32_put(timer->fifo, timer->data);
        } else {
            ts = 1;
        }
        /* 指向下一个超时值最小的定时器 */
        timer = timer->next;
    }

    /* 更新定时器中当前具最小超时值的定时器 */
    timerctl.t0 = timer;
    timerctl.next = timer->timeout;

    /* 若当前任务定时器也超时了,则调用task_switch切换任务。*/
    if (ts != 0) {
        /* mtask.c */
        task_switch();
    }
    return;
}

/* timer_cancel,
 * 取消timer所指定时器,成功返回1,失败返回0。*/
int timer_cancel(struct TIMER *timer)
{
    int e;
    struct TIMER *t;

    /* 备份EFLAG并置位EFLAG禁止CPU处理中断,
     * 调用io_store_eflags(e)即可恢复EFLAG。*/
    e = io_load_eflags();
    io_cli();

    /* 若timer所指定时器处于正使用状态, */
    if (timer->flags == TIMER_FLAGS_USING) {
        if (timer == timerctl.t0) {
            /* 若timer所指定时器为链表头定时器,
             * 则更改链表头定时器为下一个定时器。*/
            t = timer->next;
            timerctl.t0 = t;
            timerctl.next = t->timeout;
        } else {
            /* 若timer所指定时器不为链表头定时器,则遍历到该头结点。
             * t指向timer所指定时器的前一个定时器。*/
            t = timerctl.t0;
            for (;;) {
                if (t->next == timer) {
                    break;
                }
                t = t->next;
            }
            /* 让timer所指定时器上一个定时器指向其下一个定时器 */
            t->next = timer->next;
        }
        /* 将timer所指定时器的状态设置为已分配,
         * 回复EFLAG寄存器的值,允许CPU处理中断。*/
        timer->flags = TIMER_FLAGS_ALLOC;
        io_store_eflags(e);
        return 1;
    }
    io_store_eflags(e);
    return 0;
}

/* timer_cancelall,
 * 取消所关联队列首地址为fifo的所有定时器。
 * 经该函数取消的定时器结构体的状态恢复未使用状态。*/
void timer_cancelall(struct FIFO32 *fifo)
{
    int e, i;
    struct TIMER *t;

    /* 备份EFLAG到e,设置EFLAG禁止CPU处理中断。*/
    e = io_load_eflags();
    io_cli();
    
    /* 取消所有还未被取消且队列首地址为fifo的定时器,
     * 将这些定时器的状态设置为未使用状态。*/
    for (i = 0; i < MAX_TIMER; i++) {
        t = &timerctl.timers0[i];
        if (t->flags != 0 && t->flags2 != 0 && t->fifo == fifo) {
            timer_cancel(t);
            timer_free(t);
        }
    }
    /* 恢复EFLAG */
    io_store_eflags(e);
    return;
}
