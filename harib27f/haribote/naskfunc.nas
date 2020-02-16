; naskfunc
; TAB=4

; naskfunc.nas, 将只能用汇编语句实现或更益用汇编语句实现的功能
; 通过汇编子程序提供给C程序调用,所有的参数都通过栈传递。现在常
; 用的编译会适当选择用寄存器传参, 都通过栈传递参数可能是作者根
; 据当时需要而改写gcc 编译器所得到的结果。该编译器在将 C全局标
; 识符转换为汇编标识符时,会自动在C全局标识符前加'_'前缀。

[FORMAT "WCOFF"]      ; 告知编译器创建目标文件的格式
[INSTRSET "i486p"]    ; 告知编译器,本程序文件中包含i486指令
[BITS 32]             ; 告知编译器将汇编转换为32位机器码
[FILE "naskfunc.nas"] ; 告知编译器本程序源文件名

; GLOBAL 告知编译器其后跟随的标号为全局标识符;
; EXTERN 告知编译器其后所声明标识符在其他文件中定义。
    GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt
    GLOBAL _io_in8,  _io_in16,  _io_in32
    GLOBAL _io_out8, _io_out16, _io_out32
    GLOBAL _io_load_eflags, _io_store_eflags
    GLOBAL _load_gdtr, _load_idtr
    GLOBAL _load_cr0, _store_cr0
    GLOBAL _load_tr
    GLOBAL _asm_inthandler20, _asm_inthandler21
    GLOBAL _asm_inthandler2c, _asm_inthandler0c
    GLOBAL _asm_inthandler0d, _asm_end_app
    GLOBAL _memtest_sub
    GLOBAL _farjmp, _farcall
    GLOBAL _asm_hrb_api, _start_app
    EXTERN _inthandler20, _inthandler21
    EXTERN _inthandler2c, _inthandler0d
    EXTERN _inthandler0c
    EXTERN _hrb_api

[SECTION .text] ;告知编译器代码段开始


; 粗略理解作者所改编 编译器函数内核函数调用栈帧。
; ---------------------------------------------
; call_fun(arg1, arg2);
; |----|
; |... |
; |----|
; |arg2| 函数参数从右向左
; |----| 依次入栈
; |arg1|
; |----|
; |EIP | call(段内调用时入栈备份EIP寄存器)
; |----|<-- ESP (SS指向内核数据段,见asmhead.nas)
; |... |


; _io_hlt,让CPU进入休眠。
;
; HLT 指令让CPU进入休眠;当复位,
; 中断到来时才唤醒CPU继续执行下一条指令(RET)。
_io_hlt: ; void io_hlt(void);
    HLT
    RET

; _io_cli,
; 禁止CPU处理调用者所在任务中断。
_io_cli: ; void io_cli(void);
    CLI
    RET

; _io_sti,
; 使能CPU处理调用者所在任务中断。
_io_sti: ; void io_sti(void);
    STI
    RET

; _io_stihlt,
; 让CPU进入睡眠,显式允许中断唤醒CPU。
_io_stihlt: ; void io_stihlt(void);
    STI
    HLT
    RET

; _io_in8,
; 从I/O端口地址port读取1字节数据并返回。
_io_in8:  ; int io_in8(int port);
    MOV EDX,[ESP+4] ; 从父函数栈中取实参port于EDX
    MOV EAX,0
    IN  AL,DX       ; 从端口读取1字节数据到作为函数返回值的EAX低字节中
    RET

; _io_in16,
; 从I/O端口地址port读取2字节数据并返回。
_io_in16: ; int io_in16(int port);
    MOV EDX,[ESP+4] ; 从父函数栈中取实参port于EDX
    MOV EAX,0
    IN AX,DX        ;从端口读取2字节数据到作为函数返回值的EAX的低2字节中
    RET

; _io_in32,
; 从I/O端口地址port读取4字节数据并返回。
_io_in32: ; int io_in32(int port);
    MOV EDX,[ESP+4] ; 从复函数栈中取实参port于EDX
    IN  EAX,DX      ; 从端口读取4字节内容到作为函数返回值的EAX中
    RET

; _io_out8,
; 将data最低字节写往I/O端口地址port。
_io_out8: ; void io_out8(int port, int data);
    MOV EDX,[ESP+4] ; 从父函数栈中取实参port于EDX
    MOV AL,[ESP+8]  ; 从父函数栈中取实参data于AL
    OUT DX,AL       ;将data最低字节写往端口port
    RET

; _io_out16,
; 将data低2字节数据写往I/O端口地址port。
_io_out16: ; void io_out16(int port, int data);
    MOV EDX,[ESP+4] ; 从父函数栈中取实参port于EDX
    MOV EAX,[ESP+8] ; 从父函数栈中取实参data于EAX
    OUT DX,AX       ; 将data低2字节写往端口
    RET

; _io_out32,
; 将4字节的data写往I/O端口地址port。
_io_out32: ; void io_out32(int port, int data);
    MOV EDX,[ESP+4] ; 从父函数栈中取实参port于EDX
    MOV EAX,[ESP+8] ; 从父函数栈中取实参data于EAX
    OUT DX,EAX      ; 将data写往端口port
    RET

; _io_load_eflags,
; 获取32位标志寄存器EFLAG的值并返回。
_io_load_eflags:   ; int io_load_eflags(void);
    PUSHFD  ; 将EFLAG入栈
    POP EAX ; 将刚入栈的EFLAG出栈赋给eax
    RET     ; eax将充当_io_load_eflags子程序的返回值

; _io_store_eflags,
; 将eflags赋值给标志寄存器EFLAG。
_io_store_eflags: ; void io_store_eflags(int eflags);
    MOV   EAX,[ESP+4] ; 从栈中取实参到EAX中
    PUSH  EAX         ; 将EAX入栈
    POPFD             ; 从栈中弹出EAX的值赋给EFLAG
    RET

; _load_gdtr,
; 将GDT内存信息加载给GDTR寄存器,addr为GDT基址,limit为GDT限长。
_load_gdtr: ; void load_gdtr(int limit, int addr);
    MOV  AX,[ESP+4] ; 从父函数栈中取实参limit(实际只有16位)于AX
    MOV  [ESP+6],AX ; 将limit置于addr低字节一边,即构成addr limit 6字节信息
    LGDT [ESP+6]    ; 将GDT基址和限长加载给GDTR寄存器
    RET

; _load_idtr,
; 加载IDT内存信息给IDTR寄存器,addr为IDT基址,limit为IDT限长。
_load_idtr: ; void load_idtr(int limit, int addr);
    MOV  AX,[ESP+4] ; 从父函数栈中取实参limit(实际值只有16位)
    MOV  [ESP+6],AX ; 将limit置于addr字节一边即构成addr limit 6字节信息
    LIDT [ESP+6]    ;将IDT基址和限长加载给IDTR寄存器
    RET

; _load_cr0,
; 获取CR0寄存器值并返回。
_load_cr0:  ; int load_cr0(void);
    MOV EAX,CR0 ; 将CR0赋给EAX然后返回
    RET

; _store_cr0,
; 将 cr0 设置给CR0寄存器。
_store_cr0: ; void store_cr0(int cr0);
    MOV EAX,[ESP+4] ; 取栈内实参到EAX
    MOV CR0,EAX     ; 设置CR0
    RET

; _load_tr,
; 将tr加载给TR任务寄存器,tr为任务号。
_load_tr: ; void load_tr(int tr);
    LTR [ESP+4] ; 将父函数栈中实参即任务号tr加载给TR寄存器
    RET


; 粗略理解在用户程序中发生(涉及特权级变化)中断的大体过程。
; -------------------------------------------------------
; [1] 中断现场保护
; push  ss
; pushl esp
; pushf TF=IF=0
; push  cs
; push  eip
; 
; [2] 栈切换(特权级发生变化),用户程序到内核程序的切换
; ss:esp=(当前任务段描述符)TSS.ss0:TSS.esp0;
; 
; 加载GDT[ IDT[中断号].处理程序所在内存段的选择符 ]到cs隐式部分,
; eip=IDT[中断号].处理程序偏移地址。
;
; 若不涉及特权级变化,如在内核程序中发生中断,则无栈寄存器入栈和栈切换过程。

; _asm_inthandler20,
; 定时器中断入口处理程序。
_asm_inthandler20:
    PUSH    ES      ; 依次备份当前任务 ES DS 数据段寄存器
    PUSH    DS
    PUSHAD          ; 依次入栈备份EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    MOV     EAX,ESP
    PUSH    EAX     ; 充当 inthandler20() 参数
    MOV     AX,SS   ; 保证DS,ES数据寄存器指向内核数据段,以访问内核数据
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler20  ; 调用定时器中断C处理函数 inthandle20()
    POP     EAX            ; 回收实参栈内存
    POPAD                  ; 依次出栈EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    POP     DS             ; 依次恢复DS ES 数据段寄存器
    POP     ES
    IRETD ; 恢复CPU的中断现场保护

; _asm_inthandler21,
; 键盘中断入口处理承程序。
_asm_inthandler21:
    PUSH    ES      ; 依次备份当前任务 ES DS 数据段寄存器
    PUSH    DS
    PUSHAD          ; 依次入栈备份EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    MOV     EAX,ESP
    PUSH    EAX     ; 充当 inthandler21() 参数
    MOV     AX,SS   ; 保证DS,ES数据寄存器指向内核数据段,以访问内核数据
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler21  ; 调用键盘中断C处理函数 inthandler21()
    POP     EAX            ; 回收参数栈内存
    POPAD                  ; 依次出栈EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    POP     DS             ; 依次恢复DS ES 数据段寄存器
    POP     ES
    IRETD ; 恢复CPU的中断现场保护

; _asm_inthandler2c,
; 鼠标中断处理入口程序。
_asm_inthandler2c:
    PUSH    ES      ; 依次备份当前任务 ES DS 数据段寄存器
    PUSH    DS
    PUSHAD          ; 依次入栈备份EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    MOV     EAX,ESP
    PUSH    EAX     ; 充当 inthandler2c() 参数
    MOV     AX,SS   ; 保证让DS,ES数据寄存器指向内核数据段,以访问内核数据
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler2c ; 调用键盘中断C处理函数 inthandler2c()
    POP     EAX           ; 回收参数栈内存
    POPAD                 ; 依次出栈EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    POP     DS            ; 依次恢复DS ES 数据段寄存器
    POP     ES
    IRETD                 ; 恢复CPU的中断现场保护


; 对于Intel保留使用的中断/异常向量IDT[0..16]号,CPU
; 在进行中断现场保护时,还会往栈中压入异常错误码,即
; ------------------------------------------------
; [1] 中断现场保护
; push  ss
; pushl esp
; pushf TF=IF=0
; push  cs
; push  eip
; push  er
; 
; [2] 栈切换(特权级发生变化),用户程序到内核程序的切换
; ss:esp=(当前任务段描述符)TSS.ss0:TSS.esp0;
; 
; 加载GDT[ IDT[中断号].处理程序所在内存段的选择符 ]到cs隐式部分,
; eip=IDT[中断号].处理程序偏移地址。
;
; 若不涉及特权级变化,如在内核程序中发生中断,则无栈寄存器入栈和栈切换过程。

; _asm_inthandler0c,
; 栈异常(如访问栈时超过了应用程序数据段)中断处理入口程序。
_asm_inthandler0c:
    STI ; 允许CPU处理中断(CPU在栈异常现场保护时设置了TF=IF=0即禁止了CPU处理中断)
    PUSH    ES      ; 依次备份当前任务 ES DS 数据段寄存器
    PUSH    DS
    PUSHAD          ; 依次入栈备份EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    MOV     EAX,ESP
    PUSH    EAX     ; 充当 inthandler0c() 参数
    MOV     AX,SS   ; 保证让DS,ES数据寄存器指向内核数据段,以访问内核数据
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler0c ; 调用栈异常C处理函数 inthandler0c()
    CMP     EAX,0         ; 判断 inthandler0c() 的返回值是否为0,
    JNE     _asm_end_app  ; 若该返回值不为0则跳转 _asm_end_app 处结束应用程序

; 跳转执行 _asm_end_app 后,CPU将跳转启动应用程序之后的内核语句处执行,后续程序不会再
; 被执行。不用担心引用程序栈帧问题,应用程序结束后所有内存资源都将被回收,见 cmd_app。
    POP     EAX           ; 回收 inthandle0c() 参数栈内存
    POPAD                 ; 依次出栈EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    POP     DS            ; 依次恢复DS ES 数据段寄存器
    POP     ES
    ADD     ESP,4         ; 跳过栈中的异常错误码 er
    IRETD                 ; 恢复CPU的中断现场保护

; _asm_inthandler0d,
; 保护异常中断处理入口程序。
_asm_inthandler0d:
    STI ; 允许CPU处理中断(CPU在栈异常现场保护时设置了TF=IF=0即禁止了CPU处理中断)
    PUSH    ES
    PUSH    DS
    PUSHAD                ; 依次入栈备份EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    MOV     EAX,ESP
    PUSH    EAX           ; 充当 inthandler0d() 参数
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler0d ; 调用保护异常C处理函数 inthandler0d()
    CMP     EAX,0         ; 判断inthandler0d返回值,
    JNE     _asm_end_app  ; 若该返回值不为0则跳转 _asm_end_app 处结束应用程序

; 跳转执行 _asm_end_app 后,CPU将跳转启动应用程序之后的内核语句处执行,后续程序不会再
; 被执行。不用担心引用程序栈帧问题,应用程序结束后所有内存资源都将被回收,见 cmd_app。
    POP     EAX           ; 回收 inthandler0d() 参数栈内存
    POPAD                 ; 依次出栈EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    POP     DS
    POP     ES
    ADD     ESP,4         ; 跳过栈中的异常错误码 er
    IRETD                 ; 恢复CPU的中断现场保护

; _memtest_sub,
; 以4Kb为单位测试[esp+12+4, esp+12+8)内存段中始于 esp+16 连续可用的内存段。
; 对于每个4Kb内存块,测试其最后4字节,若该4字节可用则标识整4Kb内存块可用。
_memtest_sub:   ; unsigned int memtest_sub(unsigned int start, unsigned int end)
    PUSH EDI    ; 先备份EDI ESI EBX 三个寄存器
    PUSH ESI
    PUSH EBX
    MOV  ESI,0xaa55aa55 ; pat0 = 0xaa55aa55;
    MOV  EDI,0x55aa55aa ; pat1 = 0x55aa55aa;
    MOV  EAX,[ESP+12+4] ; 从栈中取出第1个参数到EAX(start);
mts_loop:
    MOV EBX,EAX
    ADD EBX,0xffc   ; 指向4Kb内存块最后4字节;
    MOV EDX,[EBX]   ; 备份4Kb内存块最后4字节内容到EDX;
    MOV [EBX],ESI   ; 将0xaa55aa55写入4Kb块最后4字节中;
    XOR DWORD [EBX],0xffffffff  ; 将写入内容翻转;
    CMP EDI,[EBX]
    JNE mts_fin     ; 若翻转失败则表明内存不可用则跳转mts_fin处
    XOR DWORD [EBX],0xffffffff  ;再次翻转4Kb块最后4字节内容;
    CMP ESI,[EBX]
    JNE mts_fin     ; 若翻转失败则表明内存不可用则跳转mts_fin处
    MOV [EBX],EDX   ; 恢复4Kb内存块最后4字节内容
    ADD EAX,0x1000  ; 检查下一个4Kb内存块;
    CMP EAX,[ESP+12+8]
    JBE mts_loop    ; 若未达所检查的内存地址上限则继续mts_loop循环;
    POP EBX         ; 出栈恢复相应寄存器,并返回
    POP ESI
    POP EDI
    RET
; 未检查完所有内存块时程序会执行到此处。
; 恢复最后4Kb块的最后4字节内容,
; 恢复相应寄存器并返回
mts_fin:
    MOV [EBX],EDX
    POP EBX
    POP ESI
    POP EDI
    RET

; _farjmp,
; 实现段间跳转即CS:EIP=cs:eip。
_farjmp: ; void farjmp(int eip, int cs);
    JMP FAR [ESP+4] ; EIP=[ESP+4], CS=[ESP+8]
    RET

; _farcall,
; 实现段间调用即CS:EIP=cs:eip。
_farcall: ; void farcall(int eip, int cs);
    CALL FAR [ESP+4] ; EIP=[ESP],CS=[ESP+8]
    RET

; _asm_hrb_api,
; 系统调用(int 40h)入口处理程序。
_asm_hrb_api:
    STI ; 允许CPU处理中断(CPU在栈异常现场保护时设置了TF=IF=0即禁止了CPU处理中断)
    PUSH    DS
    PUSH    ES
    PUSHAD  ; 依次入栈备份EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI 用户
    PUSHAD  ; 依次入栈EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI作为hrb_api函数参数
    MOV     AX,SS
    MOV     DS,AX        ; 将内核数据段加载给数据段寄存器以访问内核数据
    MOV     ES,AX
    CALL    _hrb_api     ; 调用系统调用C处理函数 hrb_api()
    CMP     EAX,0        ; 若 hrb_api() 返回值不为0则跳转 _asm_end_app 结束应用程序,
    JNE     _asm_end_app ; api_end() 系统调用的返回值将不为0,若跳转 _asm_end_app,则后续语句不会被执行
    ADD     ESP,32       ; 回收 hrb_api() 函数实参栈内存
    POPAD                ; 依次出栈 EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    POP     ES
    POP     DS
    IRETD ; 恢复CPU系统调用限长保护(系统调用现场保护同中断调用现场保护)

; _asm_end_app,
; 结束应用程序即跳转调用启动引用程序的 _start_app 后续语句处。
_asm_end_app:
; EAX值为tss.esp0地址即 TSS 中维护内核栈顶变量成员的地址
    MOV ESP,[EAX]       ; esp=tss.esp0
    MOV DWORD [EAX+4],0 ; tss.ss0 = 0,内核数据段
    POPAD               ; 与 _start_app 中的PUSHAD对应
    ; 此处RET指令刚好将内核中调用 _start_app
    ; 时压入内核栈中的eip弹出给eip寄存器
    RET

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
