/* memory.c, 内存管理程序接口 */

#include "bootpack.h"

/* 用于将32位标志寄存器bit[18]置1;
 * CR0寄存器bit[29..30]=(11)b时禁止CPU cache功能。*/
#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

/* memtest,
 * 检测内存地址空间[start,end)中起始于start的连续可用内存段。该函数返回
 * 连续可用内存段末端地址i时,即表明内存地址空间[start, i)对应内存段可用。*/
unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    /* 尝试设置标志寄存器EFLAG bit[18]=1,在i386中,
     * EFLAG AC-bit不会为1,以此来判断CPU是386还是486。*/
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        flg486 = 1;
    }

    /* 如果是i486,则恢复EFLAG的bit[18]=1位 */
    eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
    io_store_eflags(eflg);

    /* 禁止i486 CPU的cache功能 */
    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    /* 检测[start, end)内存段中连续可用内存段,
     * 经该函数检测后得到[start, i)内存段可用。*/
    i = memtest_sub(start, end);

    /* 恢复i486 CPU的cache功能 */
    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}

/* memman_init,
 * 初始化内存管理结构体。*/
void memman_init(struct MEMMAN *man)
{
    man->frees = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts = 0;
    return;
}

/* memman_total,
 * 通过man所指内存管理结构体统计当前空闲内存容量并返回。*/
unsigned int memman_total(struct MEMMAN *man)
{
    unsigned int i, t = 0;

    /* man->free 数组按照内存块首地址依次记录空闲内存块,
     * 所以前 man->frees 记录了系统当前所有空闲内存块。*/
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }
    return t;
}

/* memman_alloc,
 * 使用首次适应(FF)算法分配大小为size的内存块,
 * 若分配成功则返回大小为size的内存块首地址,失败则返回0.*/
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
    unsigned int i, a;

    /* (First Fit, 首次适应算法分配内存)
     * 依次搜索内存地址由低到高顺序排列的空闲内存块,当搜到大于等于所当前申请内存
     * 大小size的空闲内存块时,即从当前空闲内存块中分配内存块并返回该内存块基址。*/
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {
            /* 从当前空闲内存块中分配内存,更新当前空闲内存块剩余内存。*/
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {
                /* 若所申请内存刚好和当前空闲内存块大小相同,则去除对该块内存的记录
                 * 并将后续记录空闲内存块的元素依次往前移以填充被去除元素的空缺。*/
                man->frees--;
                for (; i < man->frees; i++) {
                    man->free[i] = man->free[i + 1];
                }
            }
            return a;
        }
    }
    return 0;
}

/* memman_free,
 * 释放内存块[addr, addr + size)。释放成功则返回0; 否则返回-1.*/
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i, j;

    /* 在记录空闲内存块的数组中找到预释放内存块[addr, addr + size)的位置。
     * 该数组按照空闲内存块地址从小到大的顺序依次记录。*/
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    /* free[i - 1].addr < addr (i == man->frees)
     * free[i - 1].addr < addr < free[i].addr (i < man->frees).*/
    if (i > 0) {
        /* 考虑预释放内存块[addr, addr + size)正好能跟前一空闲内存块合并的情况, */
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            /* 则将[addr, addr + size)与其合并。 */
            man->free[i - 1].size += size;
            /* 若[addr, addr + size)没有被加到有空闲内存块记录的数组元素末尾, */
            if (i < man->frees) {
                /* 则再检查[addr, addr + size)能否与后一空闲内存块合并, */
                if (addr + size == man->free[i].addr) {
                    /* 若刚好能合并,则合并,
                     * 并将后需记录空闲内存块的数组元素依次往前移。 */
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            return 0;
        }
    }
    /* 若未到达记录空闲内存块数组元素的末尾, */
    if (i < man->frees) {
        /* 再考虑[addr, addr +size)是否能跟后一空闲内存块合并,若能则合并即可。*/
        if (addr + size == man->free[i].addr) {
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }
    /* 若预释放内存块[addr, addr + size)不能与已记录的前后空闲内存块合并,
     * 则将[addr, addr + size)插入到数组相应位置上。*/
    if (man->frees < MEMMAN_FREES) {
        /* 首先将空闲内存块首地址比addr大的数组元素往后移, */
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        /* 更新数组记录的空闲内存块计数,当前内存空闲块的最大计数。*/
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees;
        }
        /* 将[addr, addr + size)内存块作为空闲内存块插入到数组中。*/
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    /* 若内存空闲块数已打MEMMAN_FRESS块,或者释放代码有BUG则记录内存释
     * 放失败次数和释放失败总大小,便于调试代码或调整MEMAN_FREES的值。
     *
     * 并返回-1表示释放失败。*/
    man->losts++;
    man->lostsize += size;
    return -1;
}

/* memman_alloc_4k,
 * 以4Kb为单位申请内存分配,若size非4Kb对齐则补齐4Kb。*/
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    unsigned int a;

    /* 补齐size让其以4Kb字节内存对齐 */
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

/* memman_free_4k,
 * 以4Kb对齐释放内存块addr */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;

    /* 补齐size让其以4Kb字节内存对齐 */
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
