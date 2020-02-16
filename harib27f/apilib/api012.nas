; api0012.nas,系统调用号为12的系统调用-刷新窗口指定区域画面

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api012.nas"]

    GLOBAL _api_refreshwin

[SECTION .text]
; 同 api001.nas

; _api_refreshwin,
; 刷新win所管理窗口[(x0,y0),(x1,y1)]区域画面
_api_refreshwin:    ; void api_refreshwin(int win, int x0, int y0, int x1, int y1);
    PUSH EDI
    PUSH ESI
    PUSH EBX
    MOV  EDX,12       ; EDX=12,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+16] ; win
    MOV  EAX,[ESP+20] ; x0
    MOV  ECX,[ESP+24] ; y0
    MOV  ESI,[ESP+28] ; x1
    MOV  EDI,[ESP+32] ; y1
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    POP  ESI
    POP  EDI
    RET               ; 返回到调用本函数的下一语句处