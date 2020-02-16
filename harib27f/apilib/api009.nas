; api009.nas,系统调用号为9的系统调用-内存分配

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api009.nas"]

    GLOBAL _api_malloc

[SECTION .text]
; 同 api001.nas

; _api_malloc,
; 从应用程序堆内存空间分配size大小内存
_api_malloc:    ; char *api_malloc(int size);
    PUSH EBX
    MOV  EDX,9           ; EDX=9,系统调用号,见 hrb_api()
    MOV  EBX,[CS:0x0020] ; 堆内存在数据段中的偏移,即管理
                         ; 应用程序堆内存分配的结构体偏移地址,见 _api_initmalloc
    MOV  ECX,[ESP+8]     ; size
    INT  0x40            ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                         ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET                  ; 返回到调用本函数的下一语句处