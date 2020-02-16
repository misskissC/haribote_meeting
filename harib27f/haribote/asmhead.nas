; haribote-os boot asm
; TAB=4

; asmhead.nas, 从x86实模式进入保护模式

; 告知编译器,本程序文件中包含i486指令
[INSTRSET "i486p"]


; 通过VBE指定 1024 x 768 x 8bit 彩色模式
VBEMODE EQU 0x105
;
; 列举下VBE所支持的显示模式
; 0x100 :  640 x  400 x 8bit 彩色
; 0x101 :  640 x  480 x 8bit 彩色
; 0x103 :  800 x  600 x 8bit 彩色
; 0x105 : 1024 x  768 x 8bit 彩色
; 0x107 : 1280 x 1024 x 8bit 彩色

; 进入保护模式后,
; asmhead.nas会将haribote除asmhead.nas的剩余程序
; 拷贝到扩展内存段中,并用一段内存用作软盘缓冲区。
;
; 以下地址分别代表
; haribote剩余程序将被拷贝到扩展内存中的起始地址,
; 存储整个软盘内容的起始地址,
; ipl09.nas所读取的软盘内容在内存中的起始地址(实际上是8200h)。
BOTPAK  EQU 0x00280000
DSKCAC  EQU 0x00100000
DSKCAC0 EQU 0x00008000

; 以下值皆为段地址0的段内偏移地址,
; 这些内存用于存储通过VBE或BIOS获取到的启动信息。
;
; 这些值分别用作
; 存储haribote OS所占总柱面数(由ipl09.nas设置),
; 保存键盘状态标志,
; 存储色素位数信息,
; 存储x分辨率,
; 存储y分辨率,
; 存储显存起始地址。
CYLS  EQU 0x0ff0
LEDS  EQU 0x0ff1
VMODE EQU 0x0ff2
SCRNX EQU 0x0ff4
SCRNY EQU 0x0ff6
VRAM  EQU 0x0ff8

; 告知编译器后续程序的段内偏移基址为0xc200
    ORG 0xc200

; 检查显卡是否支持VBE,
; 若不支持则跳转scrn320处使用BIOS设置画面显示。
;
; 若支持VBE,显卡能力集信息将输出到ES:DI指向的内存段。
    MOV AX,0x9000
    MOV ES,AX
    MOV DI,0
    MOV AX,0x4f00
    INT 0x10
    CMP AX,0x004f
    JNE scrn320   ; if (AX != 0x004f) goto scrn320;

; 检查VBE版本,小于v2.0的VBE不支持高分辨率显示,
; 则跳转scrn320处使用BIOS设置画面显示。
;
; 通过int 10h 4f00h 获取到的显卡能力集中,
; ES:DI+4处存储了版本信息。
    MOV AX,[ES:DI+4]
    CMP AX,0x0200
    JB  scrn320 ; if (AX < 0x0200) goto scrn320

; 通过VBE获取105h代表的显示模式
; 即1024 x 768 x 8bit 彩色显示模式相关信息。
;
; 若AX=0X004F则表示获取成功,
; 显示模式信息存于ES:DI指向内存,共256字节。
    MOV CX,VBEMODE
    MOV AX,0x4f01
    INT 0x10
    CMP AX,0x004f
    JNE scrn320

; 检查显示模式是否符合要求
    CMP BYTE [ES:DI+0x19],8
    JNE scrn320 ; 1个像素点所占位数需为8
    CMP BYTE [ES:DI+0x1b],4
    JNE scrn320 ; 显存显示图像的方式需为压缩像素方式
    MOV AX,[ES:DI+0x00]
    AND AX,0x0080
    JZ  scrn320 ; 需支持线性帧缓冲模式

; 通过VBE设置1024 x 768 x 8彩色显示模式。
;
; 作者实践需加上4000h才能设置成功,VBE手册中未提及。
    MOV BX,VBEMODE+0x4000
    MOV AX,0x4f02
    INT 0x10
; 设置成功后,
; 将105h显示模式相关参数存储到指定内存中供后续程序使用。
    MOV BYTE [VMODE],8 ; 8bits 1像素
    MOV AX,[ES:DI+0x12]
    MOV [SCRNX],AX  ; x分辨率
    MOV AX,[ES:DI+0x14]
    MOV [SCRNY],AX  ; y分辨率
    MOV EAX,[ES:DI+0x28]
    MOV [VRAM],EAX  ; 显存起始地址
    JMP keystatus

; 若显卡不支持VBE或高分辨率,
; 则通过BIOS设置320 x 200的显示画面。
scrn320:
    MOV AL,0x13
    MOV AH,0x00
    INT 0x10
; 将VGA显示模式属性保存到指定内存
; VGA显存地址空间为[0xa0000, 0xc0000)
    MOV BYTE [VMODE],8
    MOV WORD [SCRNX],320
    MOV WORD [SCRNY],200
    MOV DWORD [VRAM],0x000a0000


; 通过BIOS int 16 2 获取键盘状态存于0:LEDS内存中。
keystatus:
    MOV AH,0x02
    INT 0x16 ; keyboard BIOS
    MOV [LEDS],AL

; 若在此处不进行中断初始化可编程中断控制器,
; 则设置可编程中断控制器屏蔽所有中断。
; 禁止8259A-1和8258A-2所有中断。
    MOV AL,0xff
    OUT 0x21,AL
    NOP
    OUT 0xa1,AL
; 没有先经过ICW名字组初始化8259A,
; 怀疑此处下发OCW命令字的语句没有起作用,
; 起不处理中断的语句其实是下一个指令CLI。

    CLI ; 禁止CPU处理中断

; 开启第21根地址线,以访问1Mb之外的内存。
; 键盘控制器8042 P21引脚输出高电平将选通地址线A20。
    CALL waitkbdout ; 等待键盘输入寄存器空闲
; 往键盘控制器8042 64h端口写d1h命令,
; 表示即将往60h端口写数据输出到P2引脚。
    MOV  AL,0xd1
    OUT  0x64,AL
    
    CALL waitkbdout
; 往8042 60h端口写数据0xdf输出到P2引脚以使能A20。
    MOV  AL,0xdf
    OUT  0x60,AL
    CALL waitkbdout

; 设置GDT,
; 将设置在0:GDTR0处的GDT信息加载到GDTR寄存器中。
    LGDT [GDTR0]

; 禁止内存分页机制(CR0.bit[31]=0),
; 开启保护模式(CR0.bit[0]=1)。
    MOV EAX,CR0
    AND EAX,0x7fffffff
    OR  EAX,0x00000001
    MOV CR0,EAX
    JMP pipelineflush ; 刷新CPU预取指队列
;开启保护模式后,CPU以保护模式机制运行,在开启保护模式指令之后的指令
;都会以保护模式运行方式运行。x86CPU手册中提示,需在开启保护模式指令
;后立即跟一条跳转指令以刷新(丢掉)CPU预取指队列中的指令。
;
;刷新CPU预取指队列的跳转指令jmp pipelineflush 是在实模式下被读入到
;CPU中的,CPU在实模式下执行跳转指令时会刷新CPU预取指队列,
;同时jmp pipelineflush 这条跳转指令也能在保护模式下正确运行,因为它
;是一条段内跳转指令,其跳转与段地址无关,
;jmp pipelineflush功能相当于EIP += pipelineflush - EIP。

pipelineflush:
; 进入保护模式用跳转指令刷新CPU预取指队列后,
; 需设置各数据段寄存器指向数据内存段。
    MOV AX,1*8
    MOV DS,AX
    MOV ES,AX
    MOV FS,AX
    MOV GS,AX
    MOV SS,AX

; 将haribote OS 剩余程序即本程序标号bootpack
; 之后的512Kb内容拷贝到[0x280000,0x300000)内存段中。
;
; haribote OS 剩余程序没有512字节那么大,
; 拷贝512字节算是给haribote OS预留一些空间吧。
    MOV  ESI,bootpack
    MOV  EDI,BOTPAK
    MOV  ECX,512*1024/4
    CALL memcpy

; 接下来的两段子程序把软盘内容拷贝到[100000h, 128800h),
; 即作者把起始于100000h处的1.44Mb内存段用作存储软盘内容。
;
; 将引导区程序拷贝到[100000h, 100200h)内存段中。
    MOV  ESI,0x7c00
    MOV  EDI,DSKCAC
    MOV  ECX,512/4
    CALL memcpy

; 将[8200, 30800h)内存段内容拷贝到[100200h,128a00h)
    MOV  ESI,DSKCAC0+512
    MOV  EDI,DSKCAC+512
    MOV  ECX,0
    MOV  CL,BYTE [CYLS]
    IMUL ECX,512*18*2/4
    SUB  ECX,512/4 ; 减去引导程序大小
    CALL memcpy

; asmhead.nas与后续主要用C语言编写的程序
; 即被称作bootpack.hrb的部分共同构成haribote OS(haribote.sys)。
; C程序部分由作者改写的gcc编译器编译链接而成,
; 所得到的C语言可执行文件的头部包含了数据相关信息,
; 此处根据此头部作相关拷贝。
;
; haribote os(haribote.sys)
; |---------------------------------------------------|
; |             | header added |     bootpack.hrb     |
; | asmhead.nas |              |                      |
; |             |  by linker   | haribote os (C part) |
; |---------------------------------------------------|
    ; 从头部获取haribote C程序中的全局数据大小,
    ; 若haribote C程序中无全局数据则跳转执行skip处指令。
    MOV  EBX,BOTPAK
    MOV  ECX,[EBX+16]
    ADD  ECX,3  ; ECX += 3;
    SHR  ECX,2  ; ECX /= 4;
    JZ   skip
    ; 全局数据在haribote C程序中的偏移地址,
    ; 求得haribote C程序中全局数据的起始地址。
    MOV  ESI,[EBX+20]
    ADD  ESI,EBX
    ; haribote C程序栈顶初始位置
    MOV  EDI,[EBX+12]
    CALL memcpy ; 将haribote C程序数据部分拷贝到栈顶之后
skip:
    MOV ESP,[EBX+12] ; 初始化haribote os栈顶
    JMP DWORD 2*8:0x0000001b ; cs:eip=2*8:0x1b
;cs=2*8时是GDT[2]的选择符,
;这会使得CPU跳转执行GDT[2].base_addr+eip即0x280000+0x1b处指令,
;即跳转执行0x28001b处指令。
;
;haribote C程序偏移0x1b处为跳转指令的机器码,
;偏移0x1c处存了该跳转指令的目标地址 —— haribote C程序的入口地址。
;
;也就是说, 在执行经作者改编的gcc编译器和链接器所得到的C可执行程序时,
;需跳转到可执行程序0x1b偏移处才能执行到C程序的入口。
; 
;再者,作者在改编gcc编译器和链接器时,把C程序的入口函数也改为HariMain了。
;
;粗略理解这些内容是为了说明,
;jmp dword 2*8:0x1b最终会跳转到haribote os中的HariMain函数处,
;其被定义在bootpack.c中。


; asmhead.nas经过多次拷贝后,内存空间分布大体如下。
; 0x00000|-------------------------|
;        |      infos by BIOS      |
; 100000h|=========================|
;        |  memory floppy storage  |
; 267fffh|=========================|
;        |          ...            |
; 280000h|=========================|
;        |   haribote os(C part)   |
; 2fffffh|=========================|
;        |  stack and global-data  |
;        |  haribote os (C part)   |
; 3fffffh|=========================|
;        |          ....           |
; 作者没有公布所改编的编译器源码,头部信息如何组织具体不详。
; haribote os栈和数据区的内存地址空间是从作者的书中看过来的。
; 此文只粗略理解这个过程,能从这个过程梳理通相关原理就可以了,
; 具体细节也不去纠结了,以把更多精力放在将要盛装出场的HariMain吧。


; 等待键盘控制器8042输入寄存器空闲
waitkbdout:
    ; 读8042状态寄存器,
    ; 8042状态寄存器bit[1]=1表输入寄存器满,
    ; 尝试读一下输出寄存器以清空下输入寄存器内容,
    ; 若状态寄存器bit[1]=1则继续等待。
    IN  AL,0x64
    AND AL,0x02
    IN  AL,0x60
    JNZ waitkbdout
    RET

; while (ecx--)
;    mov es:edi, ds:esi
;    edi += 4
;    esi += 4
; 以4字节为单位, 拷贝ecx次,
; 拷贝ds:esi 指向内存段中数据到es:edi内存段。
memcpy:
    MOV EAX,[ESI]
    ADD ESI,4
    MOV [EDI],EAX
    ADD EDI,4
    SUB ECX,1
    JNZ memcpy  ; if (ecx != 0) goto memcpy;
    RET

; 告知编译器后续内容起始地址以16字节对齐
    ALIGNB  16
GDT0:
    RESB 8  ; GDT[0]保留为0

    ; GDT[1], 描述内存段为有效的数据内存段,
    ; 段内存基址=0, 段长=0xffffffff,dpl=0
    DW   0xffff,0x0000,0x9200,0x00cf

    ; GDT[2], 描述内存段为有效的可读可执行内存段,
    ; 段内存基址=0x00280000, 段长=0x7ffff字节;
    ; GDT[2]是专程为haribote 用C主写部分程序设计的。
    DW   0xffff,0x0000,0x9a28,0x0047

    DW  0

; GDT的长度和基址信息
GDTR0:
    ; GDT目前只有3个GDT描述符, 所以GDT限长3*8-1
    ; GDT内存地址。
    DW  8*3-1
    DD  GDT0

    ALIGNB  16
bootpack:
