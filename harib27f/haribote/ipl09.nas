; haribote-ipl
; TAB=4

; ipl09.nas, 操作系统引导程序

CYLS EQU 9 ; 从磁盘读取haribote时所读柱面数
;即加上引导程序,haribote共9*2*18=324扇区=162Kb.

ORG 0x7c00  ; 告知编译器, 引导程序段内偏移基址为0x7c00
;因为引导程序的段基址默认从0开始, 所以段内偏移基址需为
;0x7c00, 这样才能让CPU正确开始执行(cs:ip=0:0x7c00)被加
;载到[0x07c00, 0x07e00)中的引导程序。


; 因为haribote支持FAT12文件系统且haribote在该文件系统中,
; 所以引导区中包含了FAT12保留区域内容。
;
; 粗略了解FAT12文件系统在磁盘上的大体格式。
; ---------------------------
; |保留区域|FAT区域|数据区域|
; ---------------------------
; [1] 保留区域总体描述了FAT文件系统的基本组成和相关参数。
; [2] FAT区域管理文件系统簇的使用情况。
; [3] 数据区用于保存文件数据。
;
; 先来看看FAT12保留区域内容吧。
; (对于偶尔不明白其实际含义的注释,暂可略过,file.c 可能助该处内容的理解)
;
; 这段被用作FAT12保留区域的描述内容共占62字节。
; 对加载引导程序的BIOS来说, 这62字节是透明的:
; BIOS加载引导程序到[0x07c00, 0x07e00)内存段后,
; 若引导程序最后两字节为0xaa55则跳转执行0x07c00
; 处内容(对应此处的'jmp entry'指令)。
    JMP entry       ; 跳转指令
    DB  0x90        ; nop指令的机器码(与跳转指令共占3字节)
    DB  "HARIBOTE"  ; 用作操作系统名称
    DW  512         ; 磁盘扇区字节数
    DB  1           ; 磁盘每簇扇区数
    DW  1           ; 保留区扇区数
    DB  2           ; FAT(文件分配表)数量
    DW  224         ; 根目录下的文件最大数量
    DW  2880        ; 文件系统扇区总数, 2880个扇区为1.44Mb,
                    ; 扇区数若超过65535,则此处为0, [32,35]偏移处为文件系统扇区总数。
    DB  0xf0        ; 文件系统所在存储介质类型, 0xf0表示可移动介质
    DW  9           ; 一个FAT所占扇区数
    DW  18          ; 每磁道扇区数
    DW  2           ; 磁头数
    DD  0           ; 无隐藏扇区(若有则在保留区域前)
    DD  2880        ; 文件系统所占扇区数
    DB  0,0,0x29    ; BIOS int 13h磁盘号; 保留未使用; 0x29表示下一个值才表示卷数
    DD  0xffffffff  ; 卷数(卷序列号)
    DB  "HARIBOTEOS " ; 用作卷标, 磁盘名称
    DB  "FAT12   "    ; 用作文件系统类型标签
    ; 62字节偏移处, FAT12保留区域内容结束
    RESB    18        ; 在描述FAT12文件系统概样之后填充18字节0(之后便从80字节处开始)


; 由FAT12保留区域第一条跳转指令跳转到此
entry:
    MOV AX,0 ;设置引导程序数据段和栈段基址(寄存器)为0
    MOV SS,AX
    MOV SP,0x7c00 ;栈顶基址为0x7c00
    MOV DS,AX

; 调用readfast子程序读取haribote到[0x08200, 0x30800)内存段中
    MOV AX,0x0820
    MOV ES,AX
    MOV CH,0           ; 柱面初始值,从柱面0开始读取
    MOV DH,0           ; 磁头初始值,从磁头0开始读取
    MOV CL,2           ; 扇区初始值,从扇区2开始读取,1对应引导区
    MOV BX,18*2*CYLS-1 ; ES:BX指向保存从磁盘读取的hariboe的内存首地址,
                       ; BX同时兼任保存未读扇区总数的任务。
    CALL readfast

; 将haribote柱面数保存在ds:[0x0ff0]处,即0:0x0ff0处,供后续程序使用。
; 读取haribote到[0x08200, 0x30800)内存中后, 跳转到0xc200处执行haribote
    MOV BYTE [0x0ff0],CYLS
    JMP 0xc200
;粗略理解0xc200,
;haribote引导程序ipl09.nas被编译链接为二进制文件ipl09.bin,
;haribote的内核程序文件最终被编译链接为二进制文件为haribote.sys。
;作者按照FAT12文件系统格式将ipl09.bin和haribote.sys写入磁盘中:
;将ipl09.bin中的二进制数据写入到引导区(包含引导程序和FAT12保留区域内容);
;将haribote.sys文件按照设定的FAT12格式存入磁盘中,
;haribote.sys的文件名和文件数据在磁盘中的位置由作者根据FAT12设计而被定。
;
;在作者设计的FAT12文件系统中,
;第34扇区处开始存第一个文件的数据(文件名从第20个扇区开始存储),
;所以haribote内核程序在内存中的地址=
;0x08200 + (33 - 1(除去引导区)) * 512 = 0x0c200,即0:0xc200.
;
;应该是为了节省篇幅便于描述,
;作者在书中并没有阐述这个过程,而是用二进制编辑器查看haribote.sys在磁盘中的存储信息。
;
;另外,asmhead.s源程序被链接在haribote.sys最前面,
;所以CPU跳转执行0:0xc200即是跳转asmhead.s中的入口程序处继续执行。
;接下来就看看asmhead.nas汇编程序吧。
;
; ipl09.nas执行后,内存空间大体分布如下。
; 0x00000|----------------------------------|
;        |            1KB RAM               |
;        | BIOS Interrupt vector table etc. |
; 0x003FF|==================================|----
;        |                                  |  ↑
;        |   ipl09.nas in [7c00h, 7e00h)    |  |
;        |  haribote OS in [8200h, 30800h)  |  |
;        |                                  |  |
;        |             639KB                | available
;        |         RAM addr space           |  |
;        |                                  |  |
;        |                                  |  ↓
; 0x9FFFF|==================================|----
;        |                                  |
;        |              128K                |
;        |    video card ram addr space     |
; 0xBFFFF|==================================|
;        |                                  |
;        |             256KB                |
;        |      BIOS ROM addr space         |
;        |                                  |
;        |                                  |
; 0xFFFFF|==================================|


; 通过BIOS 10h中断调用打印haribote读取失败的提示信息
error:
    MOV AX,0    ; msg所在内存段 基址
    MOV ES,AX
    MOV SI,msg  ; msg段内偏移地址
putloop:
    MOV AL,[SI] ; 取msg中的当前字符
    ADD SI,1    ; 让si指向下一个字符
    CMP AL,0
    JE  fin     ; 若到AL上的值为字符串末尾(0标识)则跳转fin处
    MOV AH,0x0e ; AH=0x0e: 显示AL上的ASCII字符到屏幕上
    MOV BX,15   ; BH:字符显示模式;BL:前景色
    INT 0x10    ; 调用BIOS 10h显示入参指定的字符
    JMP putloop

; 读取haribote失败, 则最后休眠死机
fin:
    HLT
    JMP fin

; 读取haribote失败后所欲显示的提示信息
msg:
    DB  0x0a, 0x0a  ;回车的ASCII
    DB  "load error"
    DB  0x0a
    DB  0 ;提示信息结束标志


; 调用BIOS 13h AH=2h读取磁盘,
; DH=当前磁头;
; DL=驱动器号;
; CH-当前柱面;
; CL-起始扇区;
; AL-欲读扇区数;
;
; 从以上参数表征的磁盘块中读取到的数据保存到ES:BX指向的内存段中
; eflag.CF=0 如果读取成功
; eflag.CF=1 如果读取失败
readfast:
; 此处到.skip3之前的程序是为了求得当前可读取的最大扇区数,
; 已达到一次尽可能多读的目的。
;
; 首先根据当前段剩余容量计算所能读取扇区的最大值。
; 即计算还需读多少个扇区可读满当前内存段(64Kb), 
; 内存段从内存地址0处开始算起。
;
; 因为每读取一个扇区到ES:BX指向内存段,
; 段地址ES就会相应增加0x200(一个扇区大小),
; 根据段地址的bit[9..19]可判断已读取的扇区数。
;
; 在x86实模式中, 一个内存段为64Kb, 
; 只需使用bit[9..16]来判断是否已读满当前段:
; "每当"向bit[16]进位1时则表示已读满当前段。
    MOV AX,ES
    SHL AX,3    ; 将段地址的bit[9..11]移到AH低3位中
    AND AH,0x7f ; 取段地址bit[9..15]的值
    MOV AL,128  ; 128 * 512 = 64Kb(x86实模式下 一个段的大小)
    SUB AL,AH   ; AL=AL-AH,即计算读满当前段还需的扇区数

; 此处到.skip2的指令是保证最多只能读取不足9个柱面的扇区数
; 根据剩余扇区数BX得到当前所能读取的最大扇区数并存于AH中
    MOV AH,BL   ; 当未读扇区数小于256时,BL值就是未读扇区数,尤其当未读扇区所剩无几时,该子程序段有效
    CMP BH,0    ; if (BH != 0) { AH = 18; }, 该条件略微粗糙, if (BX>=18){AH=18}更好一点
    JE  .skip1
    MOV AH,18
.skip1:
    CMP AL,AH   ; if (AL > AH) { AL = AH; }
    JBE .skip2
    MOV AL,AH
.skip2:
    MOV AH,19
    SUB AH,CL   ; AH = 19 - CL; 计算当前磁道还未读取的扇区数与AH中
    CMP AL,AH   ; if (AL > AH) { AL = AH; }
    JBE .skip3
    MOV AL,AH
; 到此处, AL包含了当前所能读取扇区的最大值

.skip3:
    PUSH BX
    MOV SI,0    ; 用si记录每次读磁盘的错误次数
retry:
    MOV AH,0x02 ; AH=0x02: 读磁盘
    MOV BX,0    ; 所读内容保存在ES:BX指向的内存段
    MOV DL,0x00 ; 读A盘驱动器
    PUSH ES     ; 在栈中备份以下几个寄存器
    PUSH DX
    PUSH CX
    PUSH AX
    INT 0x13    ; 调用BIOS 13h读磁盘
    JNC next    ; eflag.CF=0即若读取成功则跳转next处
    ADD SI,1    ; 读取失败次数增1
    CMP SI,5
    JAE error   ; if (si >=5) 则跳转error处
    MOV AH,0x00 ; if (si < 5) 则重置A驱重读, AH=0表重置磁盘功能号
    MOV DL,0x00 ; A驱动器号
    INT 0x13
    POP AX
    POP CX
    POP DX
    POP ES
    JMP retry
next:
    POP AX
    POP CX
    POP DX
    POP BX      ; ES寄存器的值出栈给BX
    SHR BX,5    ; BX右移5位,对于段地址来说相当于右移了9位
    MOV AH,0
    ADD BX,AX   ; BX += AL, 即更新已读扇区数
    SHL BX,5    ; 再将BX左移5位,对于段地址来说相当于左移了9位,即让bit[9..]记录已读扇区数
    MOV ES,BX   ; 将BX计算结果赋给ES, 将ES出栈给BX到此处的整个过程相当于ES += AL * 0x20;
    POP BX      ; 再从栈中取出真正的BX
    SUB BX,AX   ; 将未读扇区总数减去已读扇区数
    JZ  .ret    ; 若已读完所有扇区则跳转至.ret处返回
    ADD CL,AL   ; 否则更新下一次读磁盘时的起始扇区
    CMP CL,18   ;
    JBE readfast; 若起始扇区小于磁道扇区数即还未读完当前磁道则返回到readfast处继续阅读
    MOV CL,1    ; 若已读完当前磁道,则将起读扇区为1,准备读取下一个磁道
    ADD DH,1
    CMP DH,2
    JB  readfast ; DH<2表示当前磁头为1,则将磁头设置为2即读取当前柱面另一面
    MOV DH,0     ; 若当前磁头为2则又回到磁头1(读取下一柱面)
    ADD CH,1     ; 若当前磁头为2则表明当前柱面正反面都已读取完毕, 则应读取下一柱面了
    JMP readfast
.ret:            ; 读取haribote完毕,子程序返回
    RET

; 本段程序从偏移0x7c00开始, $表示当前位置基于段内偏移基址的值,
; 即将[引导程序末尾处即.ret处, 0x7dfe]置为0
    RESB    0x7dfe-$
    DB  0x55, 0xaa      ; 设置引导区有效标志0xaa55
