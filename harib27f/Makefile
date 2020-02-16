# haribote 工程根目录 Makefile
# 此文阅读工程管理文件欲粗略了解工程管理相关,
# 此文对其内所包含的一些工具的具体用法也不知祥。

# haribote 工具路径,依赖头文件路径,可根据实际目录更改
TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

# make 是解析 Makefile 的工具;
# eding.exe  是制作映像文件的工具;
# imgtol.com 是将映像文件写入软盘的工具;
# copy,del 是拷贝和删除命令。
MAKE     = $(TOOLPATH)make.exe -r
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del


# make 默认解析规则;
# 由于目标 default 不存在,所以该规则下的命令会被无条件执行。
default :
    $(MAKE) haribote.img

# 由目标 default 所在规则触发被解析,
# 当目标 haribote.img 不存在或其先决依赖文件改变时重新生成 haribote.img
haribote.img : haribote/ipl09.bin haribote/haribote.sys Makefile \
        a/a.hrb hello3/hello3.hrb hello4/hello4.hrb hello5/hello5.hrb \
        winhelo/winhelo.hrb winhelo2/winhelo2.hrb winhelo3/winhelo3.hrb \
        star1/star1.hrb stars/stars.hrb stars2/stars2.hrb \
        lines/lines.hrb walk/walk.hrb noodle/noodle.hrb \
        beepdown/beepdown.hrb color/color.hrb color2/color2.hrb \
        sosu/sosu.hrb sosu2/sosu2.hrb sosu3/sosu3.hrb \
        type/type.hrb iroha/iroha.hrb chklang/chklang.hrb \
        notrec/notrec.hrb bball/bball.hrb invader/invader.hrb \
        calc/calc.hrb tview/tview.hrb mmlplay/mmlplay.hrb gview/gview.hrb
    $(EDIMG)   imgin:../z_tools/fdimg0at.tek \
        wbinimg src:haribote/ipl09.bin len:512 from:0 to:0 \
        copy from:haribote/haribote.sys to:@: \
        copy from:ipl09.nas to:@: \
        copy from:make.bat to:@: \
        copy from:a/a.hrb to:@: \
        copy from:hello3/hello3.hrb to:@: \
        copy from:hello4/hello4.hrb to:@: \
        copy from:hello5/hello5.hrb to:@: \
        copy from:winhelo/winhelo.hrb to:@: \
        copy from:winhelo2/winhelo2.hrb to:@: \
        copy from:winhelo3/winhelo3.hrb to:@: \
        copy from:star1/star1.hrb to:@: \
        copy from:stars/stars.hrb to:@: \
        copy from:stars2/stars2.hrb to:@: \
        copy from:lines/lines.hrb to:@: \
        copy from:walk/walk.hrb to:@: \
        copy from:noodle/noodle.hrb to:@: \
        copy from:beepdown/beepdown.hrb to:@: \
        copy from:color/color.hrb to:@: \
        copy from:color2/color2.hrb to:@: \
        copy from:sosu/sosu.hrb to:@: \
        copy from:sosu2/sosu2.hrb to:@: \
        copy from:sosu3/sosu3.hrb to:@: \
        copy from:type/type.hrb to:@: \
        copy from:iroha/iroha.hrb to:@: \
        copy from:chklang/chklang.hrb to:@: \
        copy from:euc.txt to:@: \
        copy from:notrec/notrec.hrb to:@: \
        copy from:bball/bball.hrb to:@: \
        copy from:invader/invader.hrb to:@: \
        copy from:calc/calc.hrb to:@: \
        copy from:tview/tview.hrb to:@: \
        copy from:mmlplay/mmlplay.hrb to:@: \
        copy from:mmldata/kirakira.mml to:@: \
        copy from:mmldata/fujisan.mml to:@: \
        copy from:mmldata/daigo.mml to:@: \
        copy from:mmldata/daiku.mml to:@: \
        copy from:gview/gview.hrb to:@: \
        copy from:pictdata/fujisan.jpg to:@: \
        copy from:pictdata/night.bmp to:@: \
        copy from:nihongo/nihongo.fnt to:@: \
        imgout:haribote.img

# make run
# 由于目标 run 不存在,所以该规则下的规则会被无条件执行。
# [1] 更新映像文件 haribote.img;
# [2] 将更新的映像文件拷贝到虚拟机 qemu.exe 同目录下的 fdimage0.bin中;
# [3] 启动虚拟机 qemu.exe 加载 fdimage0.bin运行。
run :
    $(MAKE) haribote.img
    $(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
    $(MAKE) -C ../z_tools/qemu

# make install
# 由于目标 install 不存在,所以该规则下的命令会被无条件还行。
# [1] 更新映像文件 haribote.img;
# [2] 将更新的影响文件写入软盘中。
install :
    $(MAKE) haribote.img
    $(IMGTOL) w a: haribote.img

# make full
# 由于目标 full 不存在,所以该规则下的命令将会被无条件执行。
# 根据最新工程源文件更新硬盘映像文件 haribote.img。
full :
    $(MAKE) -C haribote
    $(MAKE) -C apilib
    $(MAKE) -C a
    $(MAKE) -C hello3
    $(MAKE) -C hello4
    $(MAKE) -C hello5
    $(MAKE) -C winhelo
    $(MAKE) -C winhelo2
    $(MAKE) -C winhelo3
    $(MAKE) -C star1
    $(MAKE) -C stars
    $(MAKE) -C stars2
    $(MAKE) -C lines
    $(MAKE) -C walk
    $(MAKE) -C noodle
    $(MAKE) -C beepdown
    $(MAKE) -C color
    $(MAKE) -C color2
    $(MAKE) -C sosu
    $(MAKE) -C sosu2
    $(MAKE) -C sosu3
    $(MAKE) -C type
    $(MAKE) -C iroha
    $(MAKE) -C chklang
    $(MAKE) -C notrec
    $(MAKE) -C bball
    $(MAKE) -C invader
    $(MAKE) -C calc
    $(MAKE) -C tview
    $(MAKE) -C mmlplay
    $(MAKE) -C gview
    $(MAKE) haribote.img

# make run_full
# 由于目标 run_full 不存在,所以规则下的命令将会无条件执行。
# [1] 根据工程最新源文件更新映像文件 haribote.img;
# [2] 将更新映像文件拷贝到虚拟机 qemu.ext 同目录下的 fdimage0.bin中;
# [3] 启动虚拟机 qemu.exe 加载 fdimage0.bin 运行。
run_full :
    $(MAKE) full
    $(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
    $(MAKE) -C ../z_tools/qemu

# make install_full
# 由于目标 install_full 不存在,所以该规则下的命令将会被无条件执行。
# [1] 根据工程最新源文件更新映像文件 haribote.img;
# [2] 将更新的影响文件写入软盘中。
install_full :
    $(MAKE) full
    $(IMGTOL) w a: haribote.img

# make run_os
# 由于目标 run_os 不存在,所以该规则下的命令将会被无条件执行。
# [1] 根据 haribote OS 最新源码更新映像文件 haribote.img;
# [2] 启动虚拟机 qemu.exe 加载映像文件运行。
run_os :
    $(MAKE) -C haribote
    $(MAKE) run

clean :

# make src_only
# 由于目标 src_only 不存在,所以该规则下的命令会被无条件执行。
# 删除映像文件 haribote.img。
src_only :
    $(MAKE) clean
    -$(DEL) haribote.img

# make clean_full
# 由于目标 clean_full 不存在,所以该规则下的命令会被无条件执行。
# 在工程各子目录下执行 make clean 命令。
clean_full :
    $(MAKE) -C haribote     clean
    $(MAKE) -C apilib       clean
    $(MAKE) -C a            clean
    $(MAKE) -C hello3       clean
    $(MAKE) -C hello4       clean
    $(MAKE) -C hello5       clean
    $(MAKE) -C winhelo      clean
    $(MAKE) -C winhelo2     clean
    $(MAKE) -C winhelo3     clean
    $(MAKE) -C star1        clean
    $(MAKE) -C stars        clean
    $(MAKE) -C stars2       clean
    $(MAKE) -C lines        clean
    $(MAKE) -C walk         clean
    $(MAKE) -C noodle       clean
    $(MAKE) -C beepdown     clean
    $(MAKE) -C color        clean
    $(MAKE) -C color2       clean
    $(MAKE) -C sosu         clean
    $(MAKE) -C sosu2        clean
    $(MAKE) -C sosu3        clean
    $(MAKE) -C type         clean
    $(MAKE) -C iroha        clean
    $(MAKE) -C chklang      clean
    $(MAKE) -C notrec       clean
    $(MAKE) -C bball        clean
    $(MAKE) -C invader      clean
    $(MAKE) -C calc         clean
    $(MAKE) -C tview        clean
    $(MAKE) -C mmlplay      clean
    $(MAKE) -C gview        clean

# make src_only_full
# 由于目标 src_only_full 不存在,所以该规则下的命令将会被执行。
# [1] 在工程各子目录下执行 make src_only;
# [2] 删除映像文件 haribote.img。
# 即只保留源文件。
src_only_full :
    $(MAKE) -C haribote     src_only
    $(MAKE) -C apilib       src_only
    $(MAKE) -C a            src_only
    $(MAKE) -C hello3       src_only
    $(MAKE) -C hello4       src_only
    $(MAKE) -C hello5       src_only
    $(MAKE) -C winhelo      src_only
    $(MAKE) -C winhelo2     src_only
    $(MAKE) -C winhelo3     src_only
    $(MAKE) -C star1        src_only
    $(MAKE) -C stars        src_only
    $(MAKE) -C stars2       src_only
    $(MAKE) -C lines        src_only
    $(MAKE) -C walk         src_only
    $(MAKE) -C noodle       src_only
    $(MAKE) -C beepdown     src_only
    $(MAKE) -C color        src_only
    $(MAKE) -C color2       src_only
    $(MAKE) -C sosu         src_only
    $(MAKE) -C sosu2        src_only
    $(MAKE) -C sosu3        src_only
    $(MAKE) -C type         src_only
    $(MAKE) -C iroha        src_only
    $(MAKE) -C chklang      src_only
    $(MAKE) -C notrec       src_only
    $(MAKE) -C bball        src_only
    $(MAKE) -C invader      src_only
    $(MAKE) -C calc         src_only
    $(MAKE) -C tview        src_only
    $(MAKE) -C mmlplay      src_only
    $(MAKE) -C gview        src_only
    -$(DEL) haribote.img

# make refresh
# 由于目标 refresh 不存在,所以该规则下的命令会被无条件执行。
# [1] 根据工程最新源文件生成映像文件;
# [2-3] 删除各工程目录下的输出和映像文件。
refresh :
    $(MAKE) full
    $(MAKE) clean_full
    -$(DEL) haribote.img
