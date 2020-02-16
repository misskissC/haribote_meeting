; api0026.nas,系统调用号为26的系统调用-获取窗口命令行数据。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api026.nas"]

    GLOBAL _api_cmdline

[SECTION .text]
; 同 api001.nas

; _api_cmdline,
; 获取命令行数据到buf所指内存段,最多获取maxsize字节。
_api_cmdline:   ; int api_cmdline(char *buf, int maxsize);
    PUSH EBX
    MOV  EDX,26       ; EDX=26,系统调用号,见 hrb_api()
    MOV  ECX,[ESP+12] ; maxsize
    MOV  EBX,[ESP+8]  ; buf
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET               ; 返回到调用本函数的下一语句处