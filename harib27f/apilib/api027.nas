; api0027.nas,系统调用号为27的系统调用-获取语言模式。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api027.nas"]

    GLOBAL _api_getlang

[SECTION .text]
; 同 api001.nas

; _api_getlang,
; 获取语言模式。
_api_getlang:   ; int api_getlang(void);
    MOV EDX,27  ; EDX=26,系统调用号,见 hrb_api()
    INT 0x40    ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    RET         ; 返回到调用本函数的下一语句处