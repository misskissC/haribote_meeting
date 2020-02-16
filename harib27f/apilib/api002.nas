; api002.nas,系统调用号为2的系统调用-打印字符串

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api002.nas"]

    GLOBAL  _api_putstr0

[SECTION .text]
; 同 api001.nas

; _api_putstr0,
; 打印字符串s,见内核程序 hrb_api()
_api_putstr0:   ; void api_putstr0(char *s);
    PUSH EBX         ; 备份 EBX 
    MOV  EDX,2       ; EDX=2,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+8] ; EBX=参数s的值
    INT  0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                     ; _asm_hrb_api 执行完系统调用号为2的子程序后将执行IRETD返回
    POP  EBX         ; 恢复 EBX 
    RET              ; 返回到调用本函数的下一语句处