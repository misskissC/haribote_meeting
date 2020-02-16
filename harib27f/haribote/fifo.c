/* fifo.c, 循环队列(fifo)程序接口,用于进程(任务)通信 */

#include "bootpack.h"

/* 队列满标志 */
#define FLAGS_OVERRUN 0x0001

/* fifo32_init,
 * 初始化一个用于当前任务和task所指任务通信的队列,由fifo指向。*/
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
{
/* 见struct FIFO32 */
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    fifo->task = task;
    return;
}

/* fifo32_put,
 * 往fifo所指队列队尾端加入数据data。*/
int fifo32_put(struct FIFO32 *fifo, int data)
{
    if (fifo->free == 0) {
        /* 设置队列满标志 */
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    /* 将data写入队列数据尾端,
     * 并更新队列中下一空闲位置和队列中空闲大小。*/
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;
    /* 给目标通信进程发送数据后,若其没有处于运行状态(2)
     * 则将其唤醒以及时读取本进程给其发的数据data。*/
    if (fifo->task != 0) {
        if (fifo->task->flags != 2) {
            task_run(fifo->task, -1, 0);
        }
    }
    return 0;
}

/* fifo32_get,
 * 从fifo所指队列队头读取数据,队列中无数据返回-1。*/
int fifo32_get(struct FIFO32 *fifo)
{
    int data;
    if (fifo->free == fifo->size) {
        /* 队列空闲大小和队列大小相等表队列无数据 */
        return -1;
    }
    /* 从队列数据头读取数据,更新队列数据头位置和队列空闲大小。*/
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

/* fifo32_status,
 * 获取fifo所指队列是否完全空闲。
 * 完全空闲-返回0,否则返回队列数据个数。*/
int fifo32_status(struct FIFO32 *fifo)
{
    return fifo->size - fifo->free;
}
