; api0023.nas,系统调用号为23的系统调用-设置文件内容位置。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api023.nas"]

    GLOBAL _api_fseek

[SECTION .text]
; 同 api001.nas

; _api_fseek,
; 从fhandle所管理文件的mode标识的位置移动offset。
_api_fseek: ; void api_fseek(int fhandle, int offset, int mode);
    PUSH EBX
    MOV  EDX,23       ; EDX=23,系统调用号,见 hrb_api()
    MOV  EAX,[ESP+8]  ; fhandle
    MOV  ECX,[ESP+16] ; mode
    MOV  EBX,[ESP+12] ; offset
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET               ; 返回到调用本函数的下一语句处