; api0015.nas,系统调用号为15的系统调用-读取当前(任务循环队列)数据

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api015.nas"]

    GLOBAL _api_getkey

[SECTION .text]
; 同 api001.nas

; _api_getkey,
; 从当前任务内核循环队列中获取数据;
; mode=0,无数据直接返回;mode=1,等待直到有数据。
_api_getkey:    ; int api_getkey(int mode);
    MOV EDX,15      ; EDX=15,系统调用号,见 hrb_api()
    MOV EAX,[ESP+4] ; mode
    INT 0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                    ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    RET             ; 返回到调用本函数的下一语句处