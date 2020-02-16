; api0010.nas,系统调用号为10的系统调用-内存释放

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api010.nas"]

    GLOBAL _api_free

[SECTION .text]
; 同 api001.nas

; _api_free,
; 释放内存段[addr, addr+size)
_api_free:  ; void api_free(char *addr, int size);
    PUSH EBX
    MOV  EDX,10          ; EDX=10,系统调用号,见 hrb_api()
    MOV  EBX,[CS:0x0020] ; 堆内存在数据段中的偏移,即管理
                         ; 应用程序堆内存分配的结构体偏移地址,见 _api_initmalloc
    MOV  EAX,[ESP+ 8]    ; addr
    MOV  ECX,[ESP+12]    ; size
    INT  0x40            ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                         ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET                  ; 返回到调用本函数的下一语句处