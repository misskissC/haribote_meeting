; api0025.nas,系统调用号为25的系统调用-读取文件内容。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api025.nas"]

    GLOBAL _api_fread

[SECTION .text]
; 同 api001.nas

; _api_fread,
; 从fhandle所管理文件最多读取maxsize字节内容到buf所指内存段中。
_api_fread: ; int api_fread(char *buf, int maxsize, int fhandle);
    PUSH EBX
    MOV  EDX,25       ; EDX=25,系统调用号,见 hrb_api()
    MOV  EAX,[ESP+16] ; fhandle
    MOV  ECX,[ESP+12] ; maxsize
    MOV  EBX,[ESP+8]  ; buf
    INT  0x40         ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                      ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    POP  EBX
    RET               ; 返回到调用本函数的下一语句处