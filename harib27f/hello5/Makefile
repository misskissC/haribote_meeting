# 应用程序 hello5.c Makefile

# 可执行应用程序名,应用程序栈内存大小,应用程序堆内存大小 变量
APP      = hello5
STACK    = 1k
MALLOC   = 0k

# 将上层目录的 app_make.txt 内容拷贝到此处
include ../app_make.txt

# ../app_make.txt 也会生成 $(APP).hrb,会不会冲突呢
$(APP).hrb : $(APP).org Makefile
    $(COPY) $(APP).org $(APP).hrb
