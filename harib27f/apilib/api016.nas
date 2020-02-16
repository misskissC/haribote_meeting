; api0016.nas,系统调用号为16的系统调用-分配定时器

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api016.nas"]

    GLOBAL _api_alloctimer

[SECTION .text]
; 同 api001.nas

; _api_alloctimer,
; 分配(管理)定时器(结构体)。
_api_alloctimer:    ; int api_alloctimer(void);
    MOV EDX,16  ; EDX=16,系统调用号,见 hrb_api()
    INT 0x40    ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    RET         ; 返回到调用本函数的下一语句处