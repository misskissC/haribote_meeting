; api0018.nas,系统调用号为18的系统调用-设置定时器

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api018.nas"]

    GLOBAL _api_settimer

[SECTION .text]
; 同 api001.nas

; _api_settimer,
; 设置timer所管理定时器超时时间为time(单位10ms)
_api_settimer:  ; void api_settimer(int timer, int time);
    PUSH EBX
    MOV  EDX,18       ; EDX=18,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+ 8] ; timer
    MOV  EAX,[ESP+12] ; time
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET               ; 返回到调用本函数的下一语句处