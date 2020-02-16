[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
; hello5.nas,用汇编程序编写在当前终端输出 hello,world\n 的程序

; 告知编译器源文件名
[FILE "hello5.nas"]

; 声明 _HariMain 为全局标识符
    GLOBAL _HariMain

; 告知编译器代码段开始
[SECTION .text]

; 用汇编程序完成打印字符串的系统调用
_HariMain:
    MOV EDX,2   ; 系统调用号2
    MOV EBX,msg ; 字符串
    INT 0x40    ; 进入打印字符串msg的系统调用
    MOV EDX,4   ; 系统调用号4
    INT 0x40    ; 进入系统调用退出本应用程序

; 告知编译器数据段开始
[SECTION .data]

; 字符串信息
msg:
    DB "hello, world", 0x0a, 0
