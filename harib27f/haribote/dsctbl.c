/* dsctbl.c, 设置 GDT&&IDT 的程序接口 */

#include "bootpack.h"

/* 设置GDT和IDT后,内存空间分布大体如下。
 * 0x00000|-------------------------|
 *        |         ...             |
 * 100000h|=========================|
 *        |   floppy image(FAT12)   |
 * 267fffh|=========================|
 *        |          ...            |
 * 26f800h|=========================|
 *        |        IDT[256]         |
 * 270000h|=========================|
 *        |        GDT[8192]        |
 * 280000h|=========================|
 *        |   haribote os(C part)   |
 * 2fffffh|=========================|
 *        |  stack and global-data  |
 *        |  haribote os (C part)   |
 * 3fffffh|=========================|
 *        |          ....           | */

/* init_gdtidt,初始化GDT,IDT。
 *
 * 在asmhead.nas中为进入保护模式设置过GDT,
 * 此处重新设置后续程序会用到的GDT和IDT。*/
void init_gdtidt(void)
{
    /* haribote分别在
     * [ADR_GDT, ADR_GDT+LIMIT_GDT]和[ADR_IDT,ADR_IDT+LIMIT_IDT]内存段设置GDT和IDT。*/
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
    int i;

    /* 先将GDT内存段初始化为0,保证了GDT[0]保留为0 */
    for (i = 0; i <= LIMIT_GDT / 8; i++) {
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    /* 设置haribote数据内存段和代码内存段GDT表项。
     *
     * GDT[1]描述内存地址空间[0x0, 0xffffffff]对应内存段,该内存段特权级为0即系统
     * 级,具 有效,可读写(即数据内存段)并使用ss:esp来维护栈内存等属性。
     * 
     * GDT[2]描述内存地址空间[ADR_BOTPAK,ADR_BOTPAK+LIMIT_BOTPAK](共64Kb)的内存段,
     * 该内存段的特权级为0(即系统级),具有效, 可读可执行(即代码内存段)并使用ss:esp
     * 维护栈内存等属性。GDT[2]所描述内存段刚好是haribote os C部分指令所在内存段。
     * 
     * 将GDT基址和限长信息加载给GDTR寄存器。*/
    set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    load_gdtr(LIMIT_GDT, ADR_GDT);

    /* 先将IDT初始化为0,将IDT基址和限长信息加载给IDTR寄存器 */
    for (i = 0; i <= LIMIT_IDT / 8; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(LIMIT_IDT, ADR_IDT);

    /* 在IDT中为以下中断设置中断处理入口程序,
     * IDT[0x00..0x10]是Intel规定保留用于设定特殊中断/异常入口处理程序的,
     * IDT[0x0c..0x0d]分别用于设定栈异常(如栈越界到数据段之外内存)和保护
     * 异常(如非法访问内存)的中断入口处理程序。
     * 
     * IDT[0x21..0x2f]在 int.c 中被设置为用来设定PIC各中断的中断入口处理
     * 程序;IDT[0x20]用于设定定时器中断入口处理程序; IDT[0x21]用于设定键
     * 盘中断入口处理程序;IDT[0x2c]用于设定鼠标中断处理程序。
     * 
     * IDT[0x40]用于设定haribote os api(系统调用)的入口处理程序,即通过指
     * 令int 0x40进入内核调用指定函数。
     *
     * 各中断处理入口程序定义在naskfunc.nas中。
     * 2*8为GDT段描述符GDT[2]的选择符,即各中断处理函数都在操作系统所在内
     * 存段。
     * 
     * AR_INTGATE32 用作设定IDT描述符有效且存在(P=1),特权级DPL=0(系统级),
     * IDT描述符类型为中断门描述符等属性。
     * AR_INTGATE32 +0x60:用作设定IDT描述符存在(P=1),特权级DPL=3(用户级),
     * IDT类型为中断门描述符等属性。
     * 
     * IDT[0x40]中的特权级(DPL)设置为3是因为用户程序会通过int 40h进行系统
     * 调用,通过int指令引用IDT描述符时CPU会检查CPL <= IDT.DPL的条件是否成
     * 立,只有该条件成立时CPU才会引用该IDT描述符。用户程序当前特权级CPL为
     * 3,所以用于设定系统调用的IDT[0x40]的特权级需为3。另外,通过IDT访问GDT
     * 所描述内存段时,CPL >= GDT.DPL时也可访问。*/
    set_gatedesc(idt + 0x0c, (int) asm_inthandler0c, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x0d, (int) asm_inthandler0d, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x40, (int) asm_hrb_api,      2 * 8, AR_INTGATE32 + 0x60);

    return;
}

/* set_segmdesc,
 * 设置sd指向的GDT段描述符,
 * sd,GDT段描述符内存首地址;limit,段描述符所描述内存段基于段基址最大偏移;
 * base,段描述符所描述内存段基址;ar,段描述符特权级,类型等属性。*/
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    /* 如果内存段超过1Mb,则置位GDT段描述符
     * 内存颗粒度位G(bit[55])。GDT段描述符
     * G位置位后,CPU将以4Kb为单位计算该GDT
     * 段描述符所描述内存段长度。*/
    if (limit > 0xfffff) {
        ar |= 0x8000; /* G_bit = 1 */
        limit /= 0x1000; /* 单位换算为4Kb */
    }
    /* 根据GDT段描述符位格式,通过GDT段描述符结构体设置GDT描述符。
     *
     * bit[15..0],内存段长度低16位;
     * bit[31..16],内存段基址低16位;
     * bit[39..32],内存段基址23..16位;
     * bit[47..40],有效位P,特权级DPL,类型TYPE等;
     * bit[55..48],内存颗粒度,内存段长度19..16位;
     * bit[63..56],内存段基址高8位。*/
    sd->limit_low    = limit & 0xffff;
    sd->base_low     = base & 0xffff;
    sd->base_mid     = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high    = (base >> 24) & 0xff;
    return;
}

/* set_gatedesc,
 * 设置gd指向的IDT描述符,
 * gd,IDT描述符内存首地址;offset,处理程序在其所在段的偏移地址;
 * selector,处理程序所在内存段的段选择符;ar,IDT描述符有效位,特权级,类型等属性。*/
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    /* 根据IDT段描述符位格式,通过IDT描述符结构体设置IDT描述符。
     * 
     * bit[15..0],处理程序偏移地址低16位;
     * bit[31..16],处理程序所在内存段的段选择符;
     * bit[39..32],保留未用;
     * bit[47..40],有效位P,特权级DPL,类型TYPE等;
     * bit[63..48],处理程序偏移地址高16位。*/
    gd->offset_low   = offset & 0xffff;
    gd->selector     = selector;
    gd->dw_count     = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high  = (offset >> 16) & 0xffff;
    return;
}
