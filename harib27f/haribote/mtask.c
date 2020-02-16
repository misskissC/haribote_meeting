/* mtask.c, 多任务管理程序接口 */

#include "bootpack.h"

/* 系统任务管理结构体全局指针变量;
 * 任务定时器全局指针变量。*/
struct TASKCTL *taskctl;
struct TIMER *task_timer;

/* task_now,
 * 获取系统任务中管理当前正运行任务的结构体基址。*/
struct TASK *task_now(void)
{
    /* 获取当前正运行任务的任务层, 然后返回
     * 该任务层中管理正运行任务的结构体基址。*/
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

/* task_add,
 * 在系统任务管理结构体中加入task。*/
void task_add(struct TASK *task)
{
    /* 获取task管理任务所在任务层 */
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    /* 将task所管理任务添加到其任务层中 tasks 数组末尾 */
    tl->tasks[tl->running] = task;
    tl->running++;   /* 当前任务层可运行任务数增1 */
    task->flags = 2; /* 将已添加到任务层中的任务置可运行状态 */
    return;
}

/* task_remove,
 * 将task所管理任务从其任务层中移除。*/
void task_remove(struct TASK *task)
{
    int i;
    /* task任务所在任务层级 */
    struct TASKLEVEL *tl = &taskctl->level[task->level];

    /* 在task所在任务层遍历到指向task的数组指针元素 */
    for (i = 0; i < tl->running; i++) {
        if (tl->tasks[i] == task) {
            break;
        }
    }
    /* 递减当前任务层处于运行状态的任务数 */
    tl->running--;

    /* 移除任务task后,tasks数组中位于task后面的任务将依次往前移,
     * 所以,如果task在正运行任务之前则应将正运行任务的下标递减。*/
    if (i < tl->now) {
        tl->now--;
    }
    /* 若task原为其任务层中最后一个任务,则再调度本任务层中第一个任务运行 */
    if (tl->now >= tl->running) {
        tl->now = 0;
    }
    /* 为被移除任务的运行状态置为休眠状态(不可调度状态) */
    task->flags = 1;

    /* 将task之后任务依次往前移 */
    for (; i < tl->running; i++) {
        tl->tasks[i] = tl->tasks[i + 1];
    }

    return;
}

/* task_switchsub,
 * 遍历并记录当前应被调度运行任务的任务层。
 * 
 * 任务层[0,MAX_TASKLEVELS)的调度优先级依次降低。
 * 若高优先级任务层中含处于可运行状态的任务时则
 * 优先调度该任务层中的任务运行。*/
void task_switchsub(void)
{
    int i;
    /* 依次遍历任务层[0, MAX_TASKLEVELS),首次遍历
     * 到有处于可运行状态任务的任务层时退出循环。*/
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        if (taskctl->level[i].running > 0) {
            break;
        }
    }
    /* 置当前需被调度运行任务的任务层和不再调度任务层的标识 */
    taskctl->now_lv = i;
    taskctl->lv_change = 0;
    return;
}

/* task_idle,
 * 空闲任务程序代码。
 * 该程序任务代码位于任务管理的最低层,当系统
 * 中没有其他任务运行时, 该任务会被调度运行。
 *
 * task_idle 让CPU进入休眠; 当复位或中断到来
 * 时CPU才会被唤醒而继续执行下一条指令。*/
void task_idle(void)
{
    for (;;) {
        io_hlt();
    }
}

/* task_init,
 * 系统任务管理初始化,包括
 * 初始化GDT和IDT;
 * 初始化 管理系统所有任务运行的 结构体;同时为当
 * 前程序分配任务管理结构体;设置闲置任务到优先级
 * 最低任务层中,以在系统无其他任务运行时被调度运行。*/
struct TASK *task_init(struct MEMMAN *memman)
{
    int i;
    struct TASK *task, *idle;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

    /* 分配内存用作系统任务管理结构体空间 */
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));

    /* 在GDT中设置各任务的TSS和LDT,经此处设置后,GDT内容为
     * GDT[0]-保留未用;
     * GDT[1]-描述内核数据内存段;
     * GDT[2]-描述内核代码内存段;
     * GDT[3..1002]-分别描述任务[0...999]TSS内存段;
     * GDT[1003..2002]-分别描述任务[0..999]LDT内存段。*/
    for (i = 0; i < MAX_TASKS; i++) {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
        set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT);
    }
    /* 初始化系统任务管理结构体中 任务层管理结构体数组 */
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }

    /* 为当前正运行程序分配任务管理结构体, 只会在主程序HarMain中
     * 调用一次;并将该结构体加入到系统任务管理体系中。当由本程序
     * 切换到其他程序中运行时, CPU会将当前程序运行上下文备份到TR
     * 所指向的(task->sel)TSS中。*/
    task = task_alloc();
    task->flags = 2;    /* 标识当前任务处于运行状态 */
    task->priority = 2; /* 当前任务时间片优先级为20ms */
    task->level = 0;    /* 当前任务位于最高任务层级 */
    task_add(task);     /* 将task所管理任务添加到其对应任务层(0)中 */
    task_switchsub();   /* 任务层级调度(结果会标记运行任务层0中的任务) */
    load_tr(task->sel); /* 将当前任务的TSS选择符加载到TR寄存器 */

    /* 设置任务调度切换定时器,当前设置超时时间约为20ms,即约20ms
     * 后让timer.c/inthandler20调用task_switch进行任务调度。*/
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

/* 设置一个(内核态)任务,当系统中无其他可运行任务时该任务会被调度运行(闲置任务) */
    idle = task_alloc(); /* 为闲置任务分配任务管理结构体 */
    idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024; /* 闲置任务栈内存 */
    idle->tss.eip = (int) &task_idle; /* 闲置任务代码起始处 */
    idle->tss.es = 1 * 8; /* 闲置任务数据段选择符 */
    idle->tss.cs = 2 * 8; /* 闲置任务代码段选择符 */
    idle->tss.ss = 1 * 8; /* 闲置任务数据段(栈)选择符 */
    idle->tss.ds = 1 * 8; /* 闲置任务数据段选择符 */
    idle->tss.fs = 1 * 8; /* 闲置任务数据段选择符 */
    idle->tss.gs = 1 * 8; /* 闲置任务数据段选择符 */

    /* 将运行时间片为1的空闲任务添加到级别最低的任务层中待调度 */
    task_run(idle, MAX_TASKLEVELS - 1, 1);
    /* idle没有初始化其level,其在task_run中未被赋值前也不会被读取 */

    /* 返回当前任务管理结构体 */
    return task;

/* 任务定时器超时即在20ms后,timer.c/inthandler20会调用task_switch进
 * 行一次任务调度,根据当前系统任务管理结构体中的状态,粗略跟踪下这次
 * 任务调度情况吧。
 *
 * [1]
 * task_add(task)和task_switchsub在task所在任务层(0)添加了task所管
 * 理任务,即对应taskctl->now_lv = 0。
 * taskctl->level[taskctl->now_lv]->now = 0(初始化为0)即对应本任务;
 * 
 * [2]
 * 然后,task_switch设置任务定时器的超时值为欲切换任务运行时间片,即
 * 让当前任务时间片运行完毕后再进行任务切换;在测到欲切换任务正是当
 * 前任务时则则继续运行当前任务。20ms后重复[1-2]操作。
 *
 * 在无其他任务加入时,任务调度将一直重复[1-2]操作。在有其他任务加入
 * 到任务层0中时,当 当前任务时间片运行完毕后, 将依次调度任务层0中后
 * 续加入的任务运行,调度策略见此文末尾归纳。
 *
 * 只有当任务层0(更高优先级任务层)中任务都处于休眠状态(被移除)时,再
 * 通过task_switchsub()调度任务层,低优先级任务层中的任务才会被调度。*/

}

/* task_alloc,
 * 在系统任务管理结构体中遍历一个任务管理结构体并初始化,
 * 若成功返回该任务管理结构体首地址, 失败则返回0。*/
struct TASK *task_alloc(void)
{
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++) {
        if (taskctl->tasks0[i].flags == 0) {
            task = &taskctl->tasks0[i];
            task->flags = 1; /* 为所管理任务置休眠的初始状态 */
            task->tss.eflags = 0x00000202; /* IF = 1以允许CPU处理本任务中断 */
            /* 初始化当前任务各寄存器值 */
            task->tss.eax = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.iomap = 0x40000000;
            task->tss.ss0 = 0;
            return task;
        }
    }
    return 0;
}

/* task_run,
 * 将task所管理程序任务的运行时间片设置为priority,然后将该任
 * 务置在任务层level中以待被调度运行。task_run会置任务层调度
 * 标志,这会让任务调度函数task_switch调用task_switchsub调度
 * 含可运行任务的优先级最高的任务层中的任务运行。*/
void task_run(struct TASK *task, int level, int priority)
{
    /* 若任务层级小于0则保持任务的任务层级 */
    if (level < 0) {
        level = task->level;
    }
    
    /* 调整task所管理任务的运行时间片 */
    if (priority > 0) {
        task->priority = priority;
    }

    /* 若task 所管理任务处于可被调度运行状态且欲调整任务层级
     * level与任务原任务层级不同则将任务从原来所在的任务层级
     * 中移除(任务被移除后其运行状态将被设置为休眠状态1)。*/
    if (task->flags == 2 && task->level != level) {
        task_remove(task); 
    }
    /* 若task所管理任务处于非运行状态则将该任务添加到任务层level中 */
    if (task->flags != 2) {
        task->level = level;
        task_add(task);
    }

    /* 置切换任务层级调度标识,由task_switchsub使用 */
    taskctl->lv_change = 1;
    return;
}

/* task_sleep,
 * 将task所指任务休眠即将其从其所在任务层中移除, 若task所指
 * 任务当前正在运行则在本任务休眠后调度优先级最高的任务运行。*/
void task_sleep(struct TASK *task)
{
    struct TASK *now_task;
    /* 若task所管理任务处于可被运行状态, */
    if (task->flags == 2) {
        /* 获取当前正运行任务 */
        now_task = task_now();
        /* 将task所管理任务从其任务层中移除 */
        task_remove(task);

        /* 若task所管理任务为当前正运行任务, 则切
         * 换优先级最高的处于可运行状态的任务运行。*/
        if (task == now_task) {
            /* 调度优先级最高的任务层 */
            task_switchsub();
            /* 获取优先级最高任务层中应被调度运行的任务 */
            now_task = task_now();
            /* 调度new_task所管理任务运行。
             *
             * 这里的参数0没有实际意义,
             * 当CPU发现now_task->sel为
             * TSS选择符时,会将TSS中eip
             * 值赋给eip寄存器。*/
            farjmp(0, now_task->sel);
        }
    }
    return;
}

/* task_switch,任务切换。*/
void task_switch(void)
{
    /* 从当前正运行任务的任务层开始, */
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    /* 获取该任务层正运行的任务, */
    struct TASK *new_task, *now_task = tl->tasks[tl->now];

    /* 获取即将被调度运行的下一个任务的索引,
     * 若该索引达任务层任务数则再从头调度。*/
    tl->now++;
    if (tl->now == tl->running) {
        tl->now = 0;
    }
    /* 如果任务层调度标志被置位, 则
     * 调度优先级最高任务层中的任务。*/
    if (taskctl->lv_change != 0) {
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }
    /* 从当前被调度任务层中获取将会被调度的任务 */
    new_task = tl->tasks[tl->now];
    
    /* 将即将被调度任务的时间片加入到定时器链表中,
     * 当该定时器超时后,定时器中断处理函数将重新调
     * 用本函数进行任务调度,见timer.c/inthandler20。*/
    timer_settime(task_timer, new_task->priority);

    /* 若欲被调度任务非当前正运行任务则进行任务切换 */
    if (new_task != now_task) {
        /* 调度new_task所管理任务运行。
         * 
         * 这里的参数0没有实际意义,
         * 当CPU发现now_task->sel为
         * TSS选择符时,会将TSS中eip
         * 值赋给eip寄存器。*/
        farjmp(0, new_task->sel);
    }
    return;
}

/* 每一种任务调度策略都有其优点和缺点,粗略了解其优缺以利用其优点。
 * |————————————————————————|
 * |         |----------|   |
 * | level 9 |0 1 2...99|   |
 * |         |----------|   |
 * |         tasks          |
 * |————————————————————————|
 *             .
 *             .
 *             .
 * |————————————————————————|
 * |         |----------|   |
 * | level 1 |0 1 2...99|   |
 * |         |----------|   |
 * |         tasks          |
 * |————————————————————————|
 * 
 * |————————————————————————|
 * |         |----------|   |
 * | level 0 |0 1 2...99|   |
 * |         |----------|   |
 * |         tasks          |
 * |————————————————————————|
 * 
 * [1]
 * 对于处于可调度运行状态的任务,level数越低其被调度优先级越高;
 * 处于同level级的任务,tasks数组所指向任务将会被依次调度运行。
 *
 * [2]
 * 在调度某任务层级中的任务时,只有任务层调度切换标志被置位后才
 * 会根据[1]中调度策略再次进行任务调度, 否则只会依次调度当前正
 * 运行任务的 任务层中的任务。
 * —— 避免只调度某任务层中的任务, 如加入优先级较高任务时应置位
 * 任务层调度标志。
 *
 * [3]
 * 除显式调用任务切换接口外,只有当前正任务运行时间片运行完毕后
 * 在定时器中断处理函数中进行任务调度。
 * —— 权衡任务运行时间片和其优先级。*/
