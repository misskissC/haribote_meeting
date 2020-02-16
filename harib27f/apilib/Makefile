# haribote 系统调用库 Makefile

# 将目标文件集赋给变量 OBJS_API
OBJS_API =  api001.obj api002.obj api003.obj api004.obj api005.obj api006.obj \
        api007.obj api008.obj api009.obj api010.obj api011.obj api012.obj \
        api013.obj api014.obj api015.obj api016.obj api017.obj api018.obj \
        api019.obj api020.obj api021.obj api022.obj api023.obj api024.obj \
        api025.obj api026.obj api027.obj alloca.obj

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
# 即解析目标 apilib.lib 所在规则。
default :
    $(MAKE) apilib.lib

# make apilib.lib 
# apilib.lib 不存在或先决依赖文件 Makefile $(OBJS_API) 有改变时,
# 该规则下的命令将会被执行, 即将 $(OBJS_API) 目标文件打包到库文
# 件 apilib.lib 中。该命令将会匹配后续隐式规则。
apilib.lib : Makefile $(OBJS_API)
    $(GOLIB) $(OBJS_API) out:apilib.lib

# 以下是隐式规则,自动变量 $* 代表目标命名中 % 之前部分命名;
# 该隐式规则由 apilib.lib 目标所在规则匹配并被解析, 即根据
# 与 $(OBJS_API) 中 .obj 文件同名的 .nas 文件生成 .obj 文件。
%.obj : %.nas Makefile
    $(NASK) $*.nas $*.obj $*.lst

# make clean 
# 由于目标 clean 不存在,所以改规则下的命令会被无条件执行,
# 即删除由 make 解析本 Makefile 时生成的中间文件。
clean :
    -$(DEL) *.lst
    -$(DEL) *.obj

# make src_only 
# 由于目标 src_only 不存在,所以该规则下的命令会被无条件执行,
# 即删除由 make 解析本 Makefile 时生成的所有文件,,只保留源文件。
src_only :
    $(MAKE) clean
    -$(DEL) apilib.lib
