; api004.nas,系统调用号为4的系统调用-结束应用程序

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api004.nas"]

    GLOBAL  _api_end

[SECTION .text]
; 同 api001.nas

; _api_end,
; 退出应用程序
_api_end:   ; void api_end(void);
    MOV EDX,4   ; EDX=系统调用号,见 hrb_api()
    INT 0x40    ; 进入内核执行 _asm_hrb_api,见 init_gdtidt()
                ; _asm_hrb_api 执行EDX=4的子程序后将调用 _asm_end_app 结束引用程序