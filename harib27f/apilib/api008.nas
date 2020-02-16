; api008.nas,系统调用号为8的系统调用-初始化应用程序内存管理

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api008.nas"]

    GLOBAL _api_initmalloc

[SECTION .text]
; 同 api001.nas

; _api_initmalloc,
; 初始化应用程序内存管理
_api_initmalloc:    ; void api_initmalloc(void);
    PUSH EBX
    MOV  EDX,8           ; EDX=8,系统调用号,见 hrb_api()
    MOV  EBX,[CS:0x0020] ; 应用程序头部偏移20h处存储了堆内存在应用程序数据内存段中的偏移地址
    MOV  EAX,EBX
    ADD  EAX,32*1024     ; 预留32KB
    MOV  ECX,[CS:0x0000] ; 应用程序头部0x0处存储了应用程序数据段大小
    SUB  ECX,EAX         ; ECX -= EAX,还剩下的数据段大小;该段内存用作堆内存
    INT  0x40            ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                         ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET                  ; 返回到调用本函数的下一语句处