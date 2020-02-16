; api005.nas,系统调用号为5的系统调用-打开运行在用户态的命令行窗口

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api005.nas"]

    GLOBAL _api_openwin

[SECTION .text]
; 同 api001.nas

; _api_openwin,
; 打开一个应用程序命令行窗口。
; 窗口画面信息存于buf所指内存段中,窗口宽高分别为xsiz,ysiz,
; col_inv标识窗口是否包含透明色,title所指字符为窗口标题。
_api_openwin:   ; int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
    PUSH EDI          ; 依次备份 EDI ESI EBX
    PUSH ESI
    PUSH EBX
    MOV  EDX,5        ; EDX=5,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+16] ; buf
    MOV  ESI,[ESP+20] ; xsiz
    MOV  EDI,[ESP+24] ; ysiz
    MOV  EAX,[ESP+28] ; col_inv
    MOV  ECX,[ESP+32] ; title
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX          ; 依次恢复 EBX ESI EDI 
    POP  ESI
    POP  EDI
    RET               ; 返回到调用本函数的下一语句处