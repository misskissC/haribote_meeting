; api0021.nas,系统调用号为21的系统调用-打开文件。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api021.nas"]

    GLOBAL _api_fopen

[SECTION .text]
; 同 api001.nas

; _api_fopen,
; 打开fname所指文件。
_api_fopen: ; int api_fopen(char *fname);
    PUSH EBX
    MOV  EDX,21      ; EDX=21,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+8] ; fname
    INT  0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                     ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET              ; 返回到调用本函数的下一语句处