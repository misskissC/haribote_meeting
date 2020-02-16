# 应用程序 gview Makefile, 因为有多个源文件,所以没有使用../app_make.txt

# 可执行应用程序名,应用程序栈内存大小,应用程序堆内存大小 变量
APP      = gview
STACK    = 4480k
MALLOC   = 0k

# haribote 工具路径,依赖头文件路径,内核程序路径,可根据实际目录更改
TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/
APILIBPATH   = ../apilib/
HARIBOTEPATH = ../haribote/

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
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -I../ -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
BIM2BIN  = $(TOOLPATH)bim2bin.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del


# make 默认解析规则,
# 即解析 $(APP).hrb 目标所在规则
default :
    $(MAKE) $(APP).hrb

# 由第4条隐式规则触发被解析,即依赖 $(APP).obj 生成 $(APP).bim,
# 且触发目标为 $(APP).obj 规则的解析,即匹配第3条隐式规则被解析,即触发生成
# $(APP).obj bmp.obj jpeg.obj。
$(APP).bim : $(APP).obj bmp.obj jpeg.obj $(APILIBPATH)apilib.lib Makefile
    $(OBJ2BIM) @$(RULEFILE) out:$(APP).bim map:$(APP).map stack:$(STACK) \
        $(APP).obj jpeg.obj bmp.obj $(APILIBPATH)apilib.lib

# 由 make run 触发解析,将应用程序加入映像文件中
haribote.img : ../haribote/ipl20.bin ../haribote/haribote.sys $(APP).hrb \
        Makefile
    $(EDIMG)   imgin:../../z_tools/fdimg0at.tek \
        wbinimg src:../haribote/ipl20.bin len:512 from:0 to:0 \
        copy from:../haribote/haribote.sys to:@: \
        copy from:$(APP).hrb to:@: \
        copy from:../nihongo/nihongo.fnt to:@: \
        imgout:haribote.img

# 以下是5条隐式规则,自动变量 $* 代表目标命名中 % 之前部分命名;
# 第5条隐式规则由目标 default 下的命令 make $(APP).hrb 触发解析
# 即触发解析规则 
# $(APP).hrb : $(APP).org Makefile
#   $(BIM2BIN) -osacmp in:$*.org out:$*.hrb
# 第5条规则触发解析第4条规则,第4条规则触发解析 $(APP).bim 目标所
# 在规则的解析,该规则将触发第3条隐式规则的解析,遂触发第2,1条规则
# 被解析,从而完成由源文件 $(APP).C 生成可执行程序 $(APP).hrb。
#
# 先决依赖文件后缀为 .gas 的规则将触发该规则被解析,即根据C源文件
# 生成同名的 .gas 文件。
%.gas : %.c ../apilib.h Makefile
    $(CC1) -o $*.gas $*.c
# 先决依赖文件后缀为 .nas 的规则将触发该规则被解析,即根据 .gas 文
# 件生成同名的 .nas 文件。
%.nas : %.gas Makefile
    $(GAS2NASK) $*.gas $*.nas
# 先决依赖文件后缀为 .obj 的规则将触发该规则被解析,即根据 .nas 文
# 件生成同名的 .obj 文件。
%.obj : %.nas Makefile
    $(NASK) $*.nas $*.obj $*.lst
# 先决依赖文件后缀为 .org 的规则将触发该规则被解析,即根据 .bim 文
# 件生成同名的 .org 文件。
%.org : %.bim Makefile
    $(BIM2HRB) $*.bim $*.org $(MALLOC)
# 先决依赖文件后缀为 .hrb 的规则将触发该规则被解析,即根据 .org 文
# 件生成同名的 .hrb 文件。
%.hrb : %.org Makefile
    $(BIM2BIN) -osacmp in:$*.org out:$*.hrb

# make run
# 由于目标 run 不存在,所以该规则下的命令会被无条件执行
# [1] 生成包含内核和本可执行应用程序映像文件;
# [2] 将映像文件拷贝到虚拟机同目录的 fdimage0.bin 中;
# [3] 启动虚拟机 qemu.exe 加载 fdimage0.bin 运行。
run :
    $(MAKE) haribote.img
    $(COPY) haribote.img ..\..\z_tools\qemu\fdimage0.bin
    $(MAKE) -C ../../z_tools/qemu

# make full
# 由于目标 full 不存在,所以该规则下的命令会被无条件执行
# [1] 重新生成系统调用库;
# [2] 重新生成本可执行应用程序。
full :
    $(MAKE) -C $(APILIBPATH)
    $(MAKE) $(APP).hrb

# make run_full
# 由于目标 run_full 不存在,所以该规则下的命令会被无条件执行
# [1] 重新生成系统调用库;
# [2] 重新生成内核文件;
# [3] 启动虚拟机 qemu.exe 运行程序。
run_full :
    $(MAKE) -C $(APILIBPATH)
    $(MAKE) -C ../haribote
    $(MAKE) run

# make clean
# 由于目标 clean 不存在,所以该规则下的命令会被无条件执行
# 删除由 make 解析本 Makefile 生成的中间文件。
clean :
    -$(DEL) *.lst
    -$(DEL) gview.obj
    -$(DEL) jpeg.obj
    -$(DEL) *.map
    -$(DEL) *.bim
    -$(DEL) *.org
    -$(DEL) haribote.img

# make src_only
# 由于目标 src_only 不存在,所以该规则下的命令会被无条件执行
# 删除由 make 解析本 Makefile 生成的所有文件,即只保留源文件。
src_only :
    $(MAKE) clean
    -$(DEL) $(APP).hrb
