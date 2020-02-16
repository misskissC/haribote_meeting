; api0013.nas,系统调用号为13的系统调用-绘制线条

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api013.nas"]

    GLOBAL _api_linewin

[SECTION .text]
; 同 api001.nas

; _api_linewin,
; 在win所管理窗口中以色号col绘制起于(x0,y0)终于(x1,y1)的线条
_api_linewin:   ; void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
    PUSH EDI
    PUSH ESI
    PUSH EBP
    PUSH EBX
    MOV  EDX,13       ; EDX=13,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+20] ; win
    MOV  EAX,[ESP+24] ; x0
    MOV  ECX,[ESP+28] ; y0
    MOV  ESI,[ESP+32] ; x1
    MOV  EDI,[ESP+36] ; y1
    MOV  EBP,[ESP+40] ; col
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    POP  EBP
    POP  ESI
    POP  EDI
    RET              ; 返回到调用本函数的下一语句处