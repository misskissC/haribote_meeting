; api0014.nas,系统调用号为14的系统调用-释放管理窗口结构体内存资源

; 同 api001.nas

[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api014.nas"]

    GLOBAL _api_closewin

[SECTION .text]
; 同 api001.nas

; _api_closewin,
; 关闭由win所管理的窗口
_api_closewin:  ; void api_closewin(int win);
    PUSH EBX
    MOV  EDX,14      ; EDX=14,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+8] ; win
    INT  0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                     ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET              ; 返回到调用本函数的下一语句处