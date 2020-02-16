# 将目标文件集赋给变量 OBJS_BOOTPACK
OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
    int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
    mtask.obj window.obj console.obj file.obj tek.obj

# haribote 工具路径,依赖头文件路径,可根据实际目录更改
TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/

# make 是解析 Makefile 的工具;
# nask.exe 是汇编编译器;
# ccl.exe 是C程序编译器;
# gas2nask.exe,obj2bim.exe,bin2obj.exe,bim2hrb.exe 是目标文件格式转换工具;
# makefont.exe 是将 hankaku.txt 文件转换为字库文件的工具;
# haribote.rul 相当于链接脚本;
# eding.exe  是制作映像文件的工具;
# imgtol.com 是将映像文件写入软盘的工具;
# golib00.exe 库文件制作工具;
# copy,del 是拷贝和删除命令。
MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

# make 默认解析规则,
# 由于目标 default 不存在,所以该规则下的命令将会被无条件执行。
# [1] 解析目标 ipl09.bin 所在规则;
# [2] 解析目标 haribote.sys 所在规则。
default :
    $(MAKE) ipl09.bin
    $(MAKE) haribote.sys

# 由目标 default 所在规则触发解析。
# 当目标 ipl09.bin 不存在或其先决依赖文件 ipl09.nas Makefile 改变时,
# 该规则下的命令将会被执行: 将汇编引导程序 ipl09.nas 编译成目标文件
# ipl09.bin。
ipl09.bin : ipl09.nas Makefile
    $(NASK) ipl09.nas ipl09.bin ipl09.lst

# 由目标 haribote.sys 所在规则触发解析
# 当目标 asmhead.bin 不存在 或其先决依赖文件 asmhead.nas Makefile 改变时
# 该规则下的命令将被执行, 将汇编程序 asmhead.nas 编译为 asmhead.bin 。
asmhead.bin : asmhead.nas Makefile
    $(NASK) asmhead.nas asmhead.bin asmhead.lst

# 由目标 hankaku.obj 所在规则触发解析
# 当目标 hankaku.bin 不存在或其先决依赖条件 hankaku.txt Makefile 改变时,
# 该规则下的命令会被执行: 将文件 hankaku.txt 转换为 hankaku.bin 。
hankaku.bin : hankaku.txt Makefile
    $(MAKEFONT) hankaku.txt hankaku.bin

# 由目标 bootpack.bim 所在规则触发解析
# 当目标 hankaku.obj 不存在或其先决依赖条件 hankaku.bin Makefile 改变时,
# 该规则下的命令会被执行: 将目标文件 hankaku.bin 转换为 hankaku.obj,
# 起始地址为 _hankaku 。
hankaku.obj : hankaku.bin Makefile
    $(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

# 由目标 bootpack.hrb 所在规则触发解析
# 当目标 bootpack.bim 不存在或其先决依赖条件 $(OBJS_BOOTPACK) Makefile 改变时,
#该改规则下的命令会被执行: 将目标文件 $(OBJS_BOOTPACK) 链接到 bootpack.bim 中。
#
# 该规则会引发目标 $(OBJS_BOOTPACK) 所在规则的解析(匹配第3个隐式规则)。
bootpack.bim : $(OBJS_BOOTPACK) Makefile
    $(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
        $(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

# 由目标 haribote.hrb 所在规则触发解析
# 当目标 bootpack.hrb 不存在或其先决依赖条件 bootpack.bim Makefile 改变时,
# 该规则下的命令会被执行: 将目标文件 bootpack.bin 转换为 bootpack.hrb。
#
# 该规则会引发目标 bootpack.bim 所在规则的解析。
bootpack.hrb : bootpack.bim Makefile
    $(BIM2HRB) bootpack.bim bootpack.hrb 0

# 由目标 default 所在规则触发解析。
# 当目标 haribote.sys 不存在或其先决依赖文件 asmhead.bin bootpack.hrb Makefile
# 改变时,该规则下的命令将会被执行: 将 asmhead.bin 和 bootpack.hrb 拷贝到 haribote.sys
# 文件中。
#
# 该规则会检查目标 asmhead.bin 和 bootpack.hrb 所在规则的检查。
haribote.sys : asmhead.bin bootpack.hrb Makefile
    copy /B asmhead.bin+bootpack.hrb haribote.sys

# 以下是3个隐式规则,自动变量 $* 代表目标命名中 % 之前部分命名;
# 第3个隐式隐式规则由 bootpack.bim 目标所在规则匹配并被解析,遂
# 匹配第2个隐式规则并被解析,再匹配第1个隐式规则并被解析。
# 
# 根据与 .gas 文件同名的 .c文件生成 .gas文件,
# 若相应的 .gas 文件不存在或 .c文件有变时。
#
# 如
# 由第2条隐式规则匹配并形成实际规则 
# bootpack.gas : bootpack.c Makefile
#   $(CC1) -o $*.gas $*.c
# 该规则生成 bootpack.gas, 第2条规则再生成 bootpack.nas,
# 第3条规则再生成 bootpack.obj,这样就得到了 $(OBJS_BOOTPACK) 中的目标文件,
# 从而生成目标 bootpack.bim
%.gas : %.c bootpack.h Makefile
    $(CC1) -o $*.gas $*.c
#
# 根据与 .nas 文件同名的 .gas文件生成 .nas文件,
# 若相应的 .nas 文件不存在或 .gas文件有变时。
#
# 如
# 由第3条隐式规则匹配并形成实际规则
# bootpack.nas : bootpack.gas Makefile
#   $(GAS2NASK) $*.gas $*.nas
# 该规则会匹配第1条隐式规则并形成实际的规则而被解析。
%.nas : %.gas Makefile
    $(GAS2NASK) $*.gas $*.nas
#
# 根据与 $(OBJS_BOOTPACK) 中 .obj 文件同名的 .nas 文件生成 .obj 文件,
# 当相应 .obj 文件不存在或 %.nas 有变时。
#
# 如
# bootpack.obj : bootpack.nas Makefile
#   $(NASK) $*.nas $*.obj $*.lst
# 
# 该规则会匹配第2条隐式规则并形成实际的规则而被解析。
%.obj : %.nas Makefile
    $(NASK) $*.nas $*.obj $*.lst
# 举个简单例子感受下吧。
# 文件 main.c Makefile
# 
# Makefile 内容如下
# all: main.out
# 
# main.out : main.o
#     gcc main.o -o main.out
# 
# main.o : main.c
# 
# %.o: %.c
#     gcc -c -o $*.o $*.c
#
# 在 Makefile 同目录下执行 Make 
# gcc -c -o main.o main.c
# gcc main.o -o main.out

# make clean 
# 由于目标 clean 不存在,所以该规则下的命令会被无条件执行。
# 删除由 make 解析本 Makefile 时所生成的文件。
clean :
    -$(DEL) asmhead.bin
    -$(DEL) hankaku.bin
    -$(DEL) *.lst
    -$(DEL) *.obj
    -$(DEL) *.map
    -$(DEL) *.bim
    -$(DEL) *.hrb

# make src_only 
# 由于目标 src_only 不存在,所以该规则下的命令会被无条件执行。
# [1] 清理由 make 解析本 Makefile 时所生成的文件;
# [2] 删除引导程序目标文件;
# [3] 删除内核目标文件 haribote.sys。
# 即只保留源文件。
src_only :
    $(MAKE) clean
    -$(DEL) ipl09.bin
    -$(DEL) haribote.sys
