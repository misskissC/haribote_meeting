; api0017.nas,系统调用号为17的系统调用-初始化定时器

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api017.nas"]

    GLOBAL  _api_inittimer

[SECTION .text]
; 同 api001.nas

;_api_inittimer,
; 初始化timer所管理定时器,定时器基数为data.
_api_inittimer: ; void api_inittimer(int timer, int data);
    PUSH EBX
    MOV  EDX,17       ; EDX=17,系统调用号,见 hrb_api()
    MOV  EBX,[ESP+ 8] ; timer
    MOV  EAX,[ESP+12] ; data
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET               ; 返回到调用本函数的下一语句处