; 告知编译器
; 目标文件格式,本文件中包含 i486 指令;
; 编译成 32位机器码; 本程序原文件名。
[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "alloca.nas"]

    GLOBAL  __alloca

[SECTION .text]

__alloca:
		ADD		EAX,-4
		SUB		ESP,EAX
		JMP		DWORD [ESP+EAX]		; RET�̑���
