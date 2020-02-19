### haribote_meeting
---
`haribote_meeting` just only means haribote OS learning, reading and annotating.

I had tried my best to read and annotate haribote OS during 2019.06 ~ 2019.11, the small fragment following extracted from harib27f/haribote/graphic.c and harib27f/haribote/naskfunc.nas respectively.
```C
/* boxfill8,
 * 用色号c充当窗口[(x0,y0),(x1,y1)]区域画面信息,窗口x方向像素点数为xsize。
 * |===========================|
 * | (x0,y0)|-------|          |
 * |        |ccccccc|          |
 * |        |-------|(x1,y1)   |
 * |                           |
 * |===========================|
 * |<-----window xsize-------->|
 * 
 * vram, 用于缓存窗口的画面信息;
 * [(x0,y0),(x1,y1)], 基于窗口左上角的窗口区域;
 * c, 填充窗口[(x0,y0),(x1,y1)]区域的色号。
 *
 * 当vram为显存基址,xsize为屏幕x箱数点时,屏幕[(x0,y0),(x1,y1)]区域将会直接显示色号c对应的RGB颜色。*/
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;

    /* 将色号c写入窗口像素(x,y)处所对应缓存中 */
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    }

    return;
}
```

```asm
; _start_app,
; 跳转执行cs:eip处(应用程序)指令,内核当前栈内存的寄存器将被备份在 tss_esp0 所指栈内存中。
_start_app: ; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
    PUSHAD ;依次入栈EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI寄存器
    MOV EAX,[ESP+36] ; EAX=eip
    MOV ECX,[ESP+40] ; ECX=cs
    MOV EDX,[ESP+44] ; EDX=esp
    MOV EBX,[ESP+48] ; EBX=ds
    MOV EBP,[ESP+52] ; EBP=&tss.esp0
    MOV [EBP  ],ESP  ; 将内核栈顶地址ESP存入 tss.esp0 中
    MOV [EBP+4],SS   ; 将内核栈内存段选择符SS保存在 tss.ss0 中

    ; 让数据段寄存器加载ds
    MOV ES,BX
    MOV DS,BX
    MOV FS,BX
    MOV GS,BX

    ; 将应用程序代码段和数据段选择符低2位置1即DPL=3(用户程序级)
    OR   ECX,3
    OR   EBX,3

    ; 将分别承载应用程序 SS ESP CS EIP 值的EBX EDX ECX EAX 依次入栈,
    ; RETF指令将从栈中弹出几个寄存器依次赋给 EIP,CS,ESP,SS 寄存器即
    ; 跳转参数cs:eip所标识的(应用程序)地址处。
    PUSH EBX
    PUSH EDX
    PUSH ECX
    PUSH EAX
    RETF
/* 在此处看看应用程序退出过程吧。
 * 
 * 当前任务在内核中调用 _start_app 跳转执行应用程序后,内核栈状况
 * |---|
 * |EIP| <- 调用 _start_app 为段内调用,所以 EIP 入栈
 * |---|
 * |EAX| 以下寄存器由 _start_app 入栈备份
 * |---|
 * |ECX|
 * |---|
 * |...|
 * |---|
 * |ESI|
 * |---|
 * |EDI|
 * |---|
 * 
 * 应用程序调用系统调用 api_end() 时会让内核把内核栈顶地址传给 _asm_end_app 
 * 子程序并跳转执行该子程序。_asm_end_app 子程序将内核栈中 EDI...EAX 依次弹
 * 出恢复他们在调用 _start_app 时的值,然后执行 RET 指令将 EIP 弹出给 EIP 寄
 * 存器,这样就让CPU继续执行调用 _start_app 处后续语句了。这样就结束了应用程
 * 序的执行,而回到内核中。回到内核中后,内核程序将会为所启动的应用程序回收相
 * 关内存资源。应用程序的启动见 cmd_app() 前后相关函数。*/
```
Goddess hopes more knowledgeable guys just like you can  ontinue to improve haribote_meeting together.
