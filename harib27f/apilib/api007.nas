; api007.nas,系统调用号为7的系统调用-在应用程序窗口中绘制矩形

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api007.nas"]

    GLOBAL _api_boxfilwin

[SECTION .text]
; 同 api001.nas

; _api_boxfilwin,
; 在win所管理窗口[(x0,y0),(x1,y1)]区域绘制色号为col的矩形。
_api_boxfilwin: ; void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
    PUSH EDI          ; 依次备份 EDI ESI EBP EBX
    PUSH ESI
    PUSH EBP
    PUSH EBX
    MOV  EDX,7        ; EDX=7,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+20] ; win
    MOV  EAX,[ESP+24] ; x0
    MOV  ECX,[ESP+28] ; y0
    MOV  ESI,[ESP+32] ; x1
    MOV  EDI,[ESP+36] ; y1
    MOV  EBP,[ESP+40] ; col
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX          ; 依次恢复 EBX EBP ESI EDI
    POP  EBP
    POP  ESI
    POP  EDI
    RET              ; 返回到调用本函数的下一语句处