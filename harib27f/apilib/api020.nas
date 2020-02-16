; api0020.nas,系统调用号为20的系统调用-蜂鸣发生器。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api020.nas"]

    GLOBAL _api_beep

[SECTION .text]
; 同 api001.nas

; _api_beep,
; 蜂鸣发生器,tone为声音频率,0时关闭。
_api_beep:  ; void api_beep(int tone);
    MOV EDX,20      ; EDX=20,系统调用号,见 hrb_api()
    MOV EAX,[ESP+4] ; tone,声音频率
    INT 0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                    ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    RET             ; 返回到调用本函数的下一语句处