; api0022.nas,系统调用号为22的系统调用-关闭文件。

; 同 api001.nas
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api022.nas"]

    GLOBAL _api_fclose

[SECTION .text]
; 同 api001.nas

; _api_fclose,
; 关闭fhandle对应的文件。
_api_fclose:    ; void api_fclose(int fhandle);
    MOV EDX,22      ; EDX=22,系统调用号,见 hrb_api()
    MOV EAX,[ESP+4] ; fhandle
    INT 0x40        ; 进入内核执行 _asm_hrb_api,见 init_gdtidt(),
                    ; _asm_hrb_api 执行完系统调用号为3的子程序后将执行IRETD返回
    RET             ; 返回到调用本函数的下一语句处