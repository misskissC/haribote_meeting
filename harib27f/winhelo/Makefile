# 应用程序 winhelo.c Makefile

# 可执行应用程序名,应用程序栈内存大小,应用程序堆内存大小 变量
APP      = winhelo
STACK    = 8k
MALLOC   = 0k

# 将上层目录的 app_make.txt 内容拷贝到此处
include ../app_make.txt

# 会不会和 ../app_make.txt 生成的 $(APP).hrb 冲突呢
$(APP).hrb : $(APP).org Makefile
    $(COPY) $(APP).org $(APP).hrb
