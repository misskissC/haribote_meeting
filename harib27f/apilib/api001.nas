; api001.nas,系统调用号为1的系统调用-打印字符

; 告知编译器
; 目标文件格式,本文件中包含 i486 指令;
; 编译成 32位机器码; 本程序原文件名。
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api001.nas"]


; 告知编译器_api_putchar标号为全局标识符
    GLOBAL  _api_putchar

; 告知编译器代码段开始
[SECTION .text]

; _api_putchar,
; 打印字符c,见内核程序 hrb_api()
_api_putchar:   ; void api_putchar(int c);
    MOV EDX,1       ; EDX = 系统调用号,见 hrb_api()
    MOV AL,[ESP+4]  ; AL  = 参数
    INT 0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                    ; _asm_hrb_api 执行完系统调用号为1的子程序后将执行IRETD返回
    RET             ; 返回到调用本函数的下一语句处

; 涉及特权级变化的 INT 指令相当于
; push ss 
; push esp 
; pushf
; push cs
; push eip
;
; IRETD 指令为 INT 指令逆操作
