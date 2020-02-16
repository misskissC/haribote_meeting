; api0011.nas,系统调用号为11的系统调用-绘制一个点

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api011.nas"]

    GLOBAL _api_point

[SECTION .text]
; 同 api001.nas

; _api_point,
; 在win所管理窗口的(x,y)处以色号col绘制一个点
_api_point: ; void api_point(int win, int x, int y, int col);
    PUSH EDI
    PUSH ESI
    PUSH EBX
    MOV  EDX,11       ; EDX=11,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+16] ; win
    MOV  ESI,[ESP+20] ; x
    MOV  EDI,[ESP+24] ; y
    MOV  EAX,[ESP+28] ; col
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    POP  ESI
    POP  EDI
    RET               ; 返回到调用本函数的下一语句处