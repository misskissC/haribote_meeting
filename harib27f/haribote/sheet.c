/* sheet.c, 管理窗口画面信息的程序接口 */

/* graphic.c 的功能是将像素点阵转换为画面信息(调色板色号)。
 * sheet.c 的功能是在缓冲区中组织画面信息,或将画面信息写入显存以显示在屏幕上。
 *
 * haribote 显示画面过程大体如下
 * [1] 字符。
 * 字符编码 --> (字库)像素点阵 --> 缓存中的画面信息(调色板色号) --> 写入显存 --> 屏幕显示。
 * [2] 图形或窗口。
 * 图形形状(字符标识的像素点阵,如鼠标) --> 缓存中的画面信息(调色板色号) --> 写入显存 --> 屏幕显示。*/

#include "bootpack.h"

/* 标识 struct SHEET 结构体为使用状态 */
#define SHEET_USE 1

/* shtctl_init,
 * 初始化管理屏幕窗口的结构体(struct SHTCTL)与屏幕无窗口的初始状态相对应。
 * vram,显存基址;xsize和ysize分别是显卡(屏幕)x方向和y方向像素点数。*/
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *ctl;
    int i;

    /* 为管理屏幕窗口的结构体分配内存 */
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }

    /* 分配内存用于记录屏幕应显示画面来自哪些图层 */
    ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0) {
        memman_free_4k(memman, (int) ctl, sizeof (struct SHTCTL));
        goto err;
    }
    ctl->vram  = vram;  /* 显存基址 */
    ctl->xsize = xsize; /* 屏幕x方向像素点数 */
    ctl->ysize = ysize; /* 屏幕y方向像素点数 */
    ctl->top = -1;      /* 屏幕顶层图层高度为-1(屏幕暂无窗口) */
    /* 置 用于管理屏幕窗口画面信息的结构体数组为初始状态 */
    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;
        ctl->sheets0[i].ctl = ctl;
    }
err:
    return ctl;
}

/* sheet_alloc,
 * 从屏幕窗口画面信息管理结构体数组 sheets0 中分配一个
 * 空闲未用元素。成功则返回该结构体首地址,失败则返回0。*/
struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    struct SHEET *sht;
    int i;

    /* 在管理屏幕窗口信息的结构体数组中遍历, */
    for (i = 0; i < MAX_SHEETS; i++) {
        /* 当首次遍历到一个空闲未使用的结构体元素时,
         * 则立即标识该结构体处于使用状态并返回其首地址。*/
        if (ctl->sheets0[i].flags == 0) {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;
            sht->height = -1;
            sht->task = 0;
            return sht;
        }
    }
    return 0; /* 若已无空闲元素则返回0 */
}

/* sheet_setbuf,
 * 设置sht所管理窗口的画面相关信息,画面信息缓冲区,窗口像素点大小,是否包含透明色。*/
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
    sht->buf = buf;         /* 窗口画面信息(色号)缓冲区基址 */
    sht->bxsize = xsize;    /* 窗口x方向像素点数 */
    sht->bysize = ysize;    /* 窗口y方向像素点数 */
    sht->col_inv = col_inv; /* 窗口中是否含透明色标识(-1则无) */
    return;
}

/* sheet_refreshmap,
 * 从窗口图层高度为h0的图层开始遍历, 直到屏幕最顶层图层,将画面落在屏幕区域
 * z=[(vx0, vy0), (vx1, vy1)]中的窗口的图层高度记录下来,即标记各图层窗口在
 * 区域z中的画面显示情况。若低图层与高图层窗口在区域z中的显示有重叠,该重叠
 * 部分的标记会被更高图层标记覆盖。*/
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
    unsigned char *buf, sid, *map = ctl->map;
    struct SHEET *sht;

    /* 将[(vx0, vy0), (vx1, vy1)]区域控制在屏幕区域内 */
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    /* 依次标记[h0, ctl->top]图层窗口在屏幕区
     * 域[(vx0, vy0), (vx1, vy1)]中的显示情况。*/
    for (h = h0; h <= ctl->top; h++) {
        /* 获取管理 图层高度为h的窗口的 结构体;
         * 将管理该窗口的结构体在 sheets0 数组中的下标作为图层h窗口的标识;
         * 获取窗口h画面信息的缓冲区基址。*/
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0;
        buf = sht->buf;

        /* 根据指定屏幕区域[(vx0, vy0),(vx1, vy1)]结合
         * 窗口起始位置和大小,计算二者交集作为最终区域。
         * 此处所得到的bx0,by0,bx1,by1为基于图层高度为
         * h窗口起始位置的像素点数。*/
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
        if (by1 > sht->bysize) { by1 = sht->bysize; }

/* 若图层高度为h的窗口与map的z=[(bx0,by0), (bx1,by1)]区域有重叠, 则将
 * 管理该窗口画面信息结构体在 sheets0 数组中的下标标记在map相应位置上。*/

        /* 窗口h中无透明色, */
        if (sht->col_inv == -1) {
            /* 若窗口像素点起始位置以4字节对齐,则以4字节为单位进行标记 */
            if ((sht->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) {
                bx1 = (bx1 - bx0) / 4; /* 窗口x方向的4字节数 */
                sid4 = sid | sid << 8 | sid << 16 | sid << 24; /* 得到4字节的sid */
                for (by = by0; by < by1; by++) {
                    vy = sht->vy0 + by;
                    vx = sht->vx0 + bx0;
                    /* 以4字节(int)为单位将sid标记窗口与区域z的重叠部分 */
                    p = (int *) &map[vy * ctl->xsize + vx];
                    for (bx = 0; bx < bx1; bx++) {
                        p[bx] = sid4;
                    }
                }
                
            /* 逐字节标记未以4字节对齐的窗口与z区域重叠部分, */
            } else {
                for (by = by0; by < by1; by++) {
                    vy = sht->vy0 + by;
                    for (bx = bx0; bx < bx1; bx++) {
                        vx = sht->vx0 + bx;
                        map[vy * ctl->xsize + vx] = sid;
                    }
                }
            }
        /* 若窗口中包含透明色, */
        } else {
            /* 则将窗口与区域z重叠部分的非透明部分标记为sid */
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by;
                for (bx = bx0; bx < bx1; bx++) {
                    vx = sht->vx0 + bx;
                    if (buf[by * sht->bxsize + bx] != sht->col_inv) {
                        map[vy * ctl->xsize + vx] = sid;
                    }
                }
            }
        }
    }
/* 若参数[(vx0,vy0), (vx1, vy1)]为窗口w0区域z,起始图层高度h0=0。
 * 假设屏幕上依次出现两个窗口w0和w1,则本函数对区域z的标记如下。
 * map                       map
 * |—————————————————————|   |—————————————————————|
 * |                     |   |       w1            |
 * |                     |   |       |————————|    |
 * |   w0                |   |   w0  |........|    |
 * |   |———————|         |   |   |———|===|....|    |
 * |   |0000000|         |-->|   |000|111|....|    |
 * |   |0000000|         |   |   |000|111|....|    |
 * |   |———————|         |   |   |———|===|....|    |
 * |                     |   |       |........|    |
 * |                     |   |       |————————|    |
 * |                     |   |                     |
 * |—————————————————————|   |—————————————————————|
 * 首先, 根据实参[(vx0,vy0), (vx1, vy1)],屏幕像素点大小
 * 以及窗口位置和大小计算w0的标记区域区域z,于是屏幕区域
 * z与map所指内存段所对应区域将被标记为0,即管理w0窗口画
 * 面信息结构体在sheets0数组中的下标。
 *
 * 遍历到窗口w1所在图层时,w1窗口与区域z只有部分重叠,于是
 * 将管理窗口w1画面信息的结构体在sheets0 中的下标1标记在
 * 与区域z重叠部分。
 * 各窗口与区域z共同重叠部分的标记将由更高层窗口标记覆盖。*/

    return;
}

/* sheet_refreshsub,
 * 从图层高度为h0的窗口开始遍历直到图层h1的窗口,
 * 刷新各图层窗口在屏幕区域[(vx0, vy0), (vx1, vy1)]中的画面显示。
 * 若低图层与高图层在区域[(vx0, vy0), (vx1, vy1)]中的画面有重叠,重叠部分画面会被更高图层画面覆盖。*/
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1, bx2, sid4, i, i1, *p, *q, *r;
    unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
    struct SHEET *sht;

    /* 将区域[(vx0, vy0),(vx1, vy1)]限制ctl所管理的屏幕区域内 */
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    /* 依次刷新图层[h0, h1]窗口在[(vx0, vy0),(vx1, vy1)]区域中的画面 */
    for (h = h0; h <= h1; h++) {
        /* 获取管理 图层高度h窗口的 结构体;
         * 获取窗口h的画面和标识。*/
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;
        
       /* 根据指定屏幕区域[(vx0, vy0), (vx1, vy1)]结合
        * 窗口起始位置和大小,计算二者交集作为最终区域。
        * 此处所得到的bx0,by0,bx1,by1为基于图层高度为
        * h窗口起始位置的像素点数。*/
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
        if (by1 > sht->bysize) { by1 = sht->bysize; }

/* 将图层高度为h的窗口在[(bx0,by0), (bx1,by1)]区域的画面写入显存vram中显示 */

        /* 若窗口起始位置像素点以4字节对齐, 则以4字节为单位刷新窗口画面 */
        if ((sht->vx0 & 3) == 0) {
            /* 计算x方向包含多少个4字节数 */
            i  = (bx0 + 3) / 4;
            i1 =  bx1      / 4;
            i1 = i1 - i;
            
            sid4 = sid | sid << 8 | sid << 16 | sid << 24; /* 4字节的sid */
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by;
                /* 逐字节刷新一行中 开始像素点位置处非4字节对齐的画
                 * 面, 当(bx0, by0)从窗口中某点时有可能出现这种情况。*/
                for (bx = bx0; bx < bx1 && (bx & 3) != 0; bx++) {
                    vx = sht->vx0 + bx;
                    if (map[vy * ctl->xsize + vx] == sid) {
                        vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                    }
                }
                /* 以4字节为单位处理像素点位置以4字节对齐的部分 */
                vx = sht->vx0 + bx;
                p = (int *) &map[vy * ctl->xsize + vx];
                q = (int *) &vram[vy * ctl->xsize + vx];
                r = (int *) &buf[by * sht->bxsize + bx];
                for (i = 0; i < i1; i++) {
                    /* 若4字节皆有画面信息, 则直接将4字节画
                     * 面信息写入到与屏幕像素点对应的vram中 */
                    if (p[i] == sid4) {
                        q[i] = r[i];
                    /* 若4字节中含透明色,则逐字节将非透明画面信息写入到与屏幕对应的vram中 */
                    } else {
                        bx2 = bx + i * 4;
                        vx = sht->vx0 + bx2;
                        if (map[vy * ctl->xsize + vx + 0] == sid) {
                            vram[vy * ctl->xsize + vx + 0] = buf[by * sht->bxsize + bx2 + 0];
                        }
                        if (map[vy * ctl->xsize + vx + 1] == sid) {
                            vram[vy * ctl->xsize + vx + 1] = buf[by * sht->bxsize + bx2 + 1];
                        }
                        if (map[vy * ctl->xsize + vx + 2] == sid) {
                            vram[vy * ctl->xsize + vx + 2] = buf[by * sht->bxsize + bx2 + 2];
                        }
                        if (map[vy * ctl->xsize + vx + 3] == sid) {
                            vram[vy * ctl->xsize + vx + 3] = buf[by * sht->bxsize + bx2 + 3];
                        }
                    }
                }
                /* 将一行末尾部分不足4字节部分的画面
                 * 信息逐字节写入与屏幕对应的vram中 */
                for (bx += i1 * 4; bx < bx1; bx++) {
                    vx = sht->vx0 + bx;
                    if (map[vy * ctl->xsize + vx] == sid) {
                        vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                    }
                }
            }
        
        /* 若窗口起始像素点位置非4字节对齐, */
        } else {
            /* 则逐字节将当前图层画面信息写入与屏幕对应的vram中, */
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by;
                for (bx = bx0; bx < bx1; bx++) {
                    vx = sht->vx0 + bx;
                    if (map[vy * ctl->xsize + vx] == sid) {
                        vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                    }
                }
            }
        }
    }
/* 若参数z=[(vx0,vy0), (vx1, vy1)]为w0区域,h0=0,h1=1。
 * 假设屏幕上依次出现窗口w0和w1,则本函数在区域z中所刷新画面过程如下。
 * screen(vram)              screen(vram)
 * |—————————————————————|   |—————————————————————|
 * |                     |   |       w1            |
 * |                     |   |       |————————|    |
 * |   w0                |   |   w0  |........|    |
 * |   |———————|         |   |   |———|===|....|    |
 * |   |nnnnnnn|         |-->|   |nnn|mmm|....|    |
 * |   |nnnnnnn|         |   |   |nnn|mmm|....|    |
 * |   |———————|         |   |   |———|===|....|    |
 * |                     |   |       |........|    |
 * |                     |   |       |————————|    |
 * |                     |   |                     |
 * |—————————————————————|   |—————————————————————|
 * 首先,根据实参z=[(vx0,vy0), (vx1, vy1)], 屏幕像素点大
 * 小以及窗口位置和大小计算画面更新区域z(w0时为整个区域
 * z; w1时为部分区域z)。然后将当前图层窗口的画面信息依次
 * 写入与z重叠区域对应的显存中以显示在屏幕上。*/

    return;
}

/* sheet_updown,
 * 将sht所管理窗口画面信息移到指定图层高度height图层中并刷新显示。*/
void sheet_updown(struct SHEET *sht, int height)
{
    /* 获取管理屏幕所有画面显示的结
     * 构体地址;备份窗口原图层高度。*/
    struct SHTCTL *ctl = sht->ctl;
    int h, old = sht->height;

    /* 将欲调整图层高度限制[-1, ctl->top]合
     * 理范围内,图层高度为-1的窗口不会被显示。*/
    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }

    /* 记录窗口画面新图层高度 */
    sht->height = height;

    /* 开始调整窗口图层高度 */

    /* 窗口原图层高度大于欲调整图层高度, */
    if (old > height) {
        if (height >= 0) {
            /* 将图层高度在[height, old)之间的图层依次
             * 上移一层后,在sheets数组height处插入sht结
             * 构体,即将sht管理窗口的图层高度调整为height。*/
            for (h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            
/* 在sht所管理窗口画面信息所占屏幕区域z=[(sht->vx0, sht->vy0),(sht->vx0+sht->bxsize,sht->vy0+sht->bysize)]中,
 * 依次标记[height + 1, ctl->top]图层在该区域内中的显示情况。只需检查并更新[height + 1, ctl->top]图层在该区域
 * 的标记是因为[0, height-1]图层与其上层图层窗口在区域z中的重叠关系未发生变化; sht从old图层下移到height图层后,
 * 需更新其上层图层[hitight + 1,ctl->top]在区域z中显示情况的标记, 以使得各图层窗口在z区域中的重叠部分由上层图
 * 层窗口标识标记。
 *
 * 在刷新屏幕画面显示时,只需刷新屏幕区域z中[height+1, old)图层窗口的画面信息即可,即[old + 1, ctl->top]图层窗口
 * 在屏幕区域z中与下层图层窗口的重叠画面未发生改变。*/
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);

        /* 若欲设置图层高度表征窗口不显示(-1), */
        } else {
            /* 若sht窗口原不在最顶层,则需将[old, ctl->top]
             * 图层依次往下一层, 并更新顶层图层高度值; 若
             * sht窗口在最顶层,则直接更新顶层图层高度即可。*/
            if (ctl->top > old) {
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
            
/* 在sht所管理窗口画面信息所占屏幕区域z=[(sht->vx0, sht->vy0), (sht->vx0+sht->bxsize, sht->vy0+sht->bysize)]中,
 * 依次标记[0,ctl->top]图层在该区域中的显示情况。因为old图层窗口被抽取掉,所以需要刷新[0,old - 1]图层的画面显示,
 * 处于old图层上层的图层[old,ctl->top]的窗口画面不用刷新。*/
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
        }
    
    /* 若sht对应窗口原图层高度old小于欲设置图层高度, */
    } else if (old < height) {
        if (old >= 0) {
            /* 则将[old, height]之间的图层依次往下移一层,并在sheets
             * 数组height处插入sht以将其图层高度从old调整为height。*/
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            
        /* 若sht对应窗口原图层高度为-1, */
        } else {
            /* 则将[height, top]之间的图层依次往上移一层,
             * 并在sheets数组中height处插入sht以及更新屏
             * 幕顶层窗口的图层高度。*/
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
/* 在sht所管理窗口画面信息所占屏幕区域z=[(sht->vx0, sht->vy0), (sht->vx0+sht->bxsize, sht->vy0+sht->bysize)]中,
 * 依次标记[height, ctl->top]图层在区域z中的显示情况。由于图层窗口上移到height图层或在height图层插入窗口时,需要
 * 更新[height,ctl->top]图层窗口在区域z中显示的标记情况(height与上层窗口重叠部分由上层窗口覆盖)
 *
 * 对于画面刷新,只需检查刷新height图层窗口会在区域z中会显示部分,更高图层窗口在该区域中的显示无变化。*/
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }
    return;
}

/* sheet_refresh,
 * 刷新sht所管理窗口画面信息在屏幕[bx0, by0), (bx1, by1)]区域中的画面显示。*/
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
    return;
}

/* sheet_slide,
 * 将sht所管理窗口画面从原位置移到屏幕(vx0, vy0)像素点位置处。*/
void sheet_slide(struct SHEET *sht, int vx0, int vy0)
{
    /* 获取管理屏幕所有画面的结构体;
     * 备份窗口在屏幕上的原起始位置。*/
    struct SHTCTL *ctl = sht->ctl;
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;

    /* 更新sht所管理窗口起始位置 */
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        /* 依次更新[0, ctl->top]图层窗口在屏幕区域
         * zo=[(old_vx0, old_vy0), (old_vx0+sht->bxsize, old_vy0+sht->bysize)]中的画面标记; */
        sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        /* 依次更新[sht->height,ctl->top]图层窗口在屏幕区域
         * zn=[(vx0, vy0), (vx0+sht->bxsize, vy0+sht->bysize)]中的画面标记,
         * 由于sht将覆盖zn整个区域低图层画面标记会被上层图层画面标记覆盖,所以不用冗余标记; */
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);

        /* 当作height图层窗口被抽走,所以需刷新刷新图层[0,height-1]窗口在zo区域中的显示; */
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);

        /* 刷新height图层窗口在zn区域中的画面显示,完成height图层移到(vx0,vy0)起始位置处 */
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

/* sheet_free,
 * 取消sht所指结构体与所管理窗口的关联。将sht管理的窗
 * 口的图层高度置为-1,将sht所指结构体置为空闲未用状态。*/
void sheet_free(struct SHEET *sht)
{
    if (sht->height >= 0) {
        sheet_updown(sht, -1); /* 将sht所管理图层隐藏 */
    }
    sht->flags = 0; /* 将sht所指结构体状态置为空闲未使用 */
    return;
}
