; api0024.nas,系统调用号为24的系统调用-获取文件大小和位置信息。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api024.nas"]

    GLOBAL _api_fsize

[SECTION .text]
; 同 api001.nas

; _api_fsize,
; mode=0,获取文件大小;
; mode=1,文件当前位置;
; mode=2,返回文件当前位置与文件末尾的偏移。
_api_fsize: ; int api_fsize(int fhandle, int mode);
    MOV EDX,24      ; EDX=24,系统调用号,见 hrb_api()
    MOV EAX,[ESP+4] ; fhandle
    MOV ECX,[ESP+8] ; mode
    INT 0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                    ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    RET             ; 返回到调用本函数的下一语句处