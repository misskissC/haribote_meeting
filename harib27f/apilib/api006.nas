; api006.nas,系统调用号为6的系统调用-在应用程序窗口中打印字符串

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api006.nas"]

    GLOBAL  _api_putstrwin

[SECTION .text]
; 同 api001.nas

; _api_putstrwin,
; 在win所管理的应用程序窗口中(x,y)位置处以色号col打印字符串,字符串长len
_api_putstrwin: ; void api_putstrwin(int win, int x, int y, int col, int len, char *str);
    PUSH EDI    ; 依次备份 EDI ESI EBP EBX
    PUSH ESI
    PUSH EBP
    PUSH EBX
    MOV  EDX,6        ; EDX=6,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+20] ; win
    MOV  ESI,[ESP+24] ; x
    MOV  EDI,[ESP+28] ; y
    MOV  EAX,[ESP+32] ; col
    MOV  ECX,[ESP+36] ; len
    MOV  EBP,[ESP+40] ; str
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX          ; 依次恢复 EBX EBP ESI EDI
    POP  EBP
    POP  ESI
    POP  EDI
    RET               ; 返回到调用本函数的下一语句处