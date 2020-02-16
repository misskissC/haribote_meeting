; api003.nas,系统调用号为3的系统调用-打印指定数字符

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api003.nas"]

    GLOBAL  _api_putstr1

[SECTION .text]
; 同 api001.nas

; api_putstr1,
; 打印s所指字符序列前l个字符
_api_putstr1: ; void api_putstr1(char *s, int l);
    PUSH EBX          ; 备份 EBX 
    MOV  EDX,3        ; EDX=3,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+ 8] ; s
    MOV  ECX,[ESP+12] ; l
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX          ; 恢复 EBX 
    RET               ; 返回到调用本函数的下一语句处