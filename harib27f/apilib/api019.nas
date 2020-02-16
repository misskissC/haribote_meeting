; api0019.nas,系统调用号为19的系统调用-释放定时器

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api019.nas"]

    GLOBAL _api_freetimer

[SECTION .text]
; 同 api001.nas

;_api_freetimer,
; 释放timer所管理定时器。
_api_freetimer: ; void api_freetimer(int timer);
    PUSH EBX
    MOV  EDX,19       ; EDX=19,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+ 8] ; timer
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET               ; 返回到调用本函数的下一语句处