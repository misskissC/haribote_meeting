/* 应用程序 gview.c,用图片做窗口背景图片应用程序 */

/* 系统调用声明头文件 */
#include "apilib.h"

/* 64Kb字节类型 */
struct DLL_STRPICENV {  /* 64KB */
    int work[64 * 1024 / 4];
};

struct RGB {
    unsigned char b, g, r, t;
};

/* bmp.nasm */
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

unsigned char rgb2pal(int r, int g, int b, int x, int y);
void error(char *s);

void HariMain(void)
{
    struct DLL_STRPICENV env;
    char filebuf[512 * 1024], winbuf[1040 * 805];
    char s[32], *p;
    int win, i, j, fsize, xsize, info[8];
    struct RGB picbuf[1024 * 768], *q;

    /* 获取应用程序所在窗口命令行输入,
     * 并跳过 gview 命令和空格。*/
    api_cmdline(s, 30);
    for (p = s; *p > ' '; p++) { }
    for (; *p == ' '; p++) { }

    /* 打开图片,获取图片大小,根据图片大小将图片读
     * 到缓冲区 filebuf 中后关闭管理图片的结构体。*/
    i = api_fopen(p); if (i == 0) { error("file not found.\n"); }
    fsize = api_fsize(i, 0);
    if (fsize > 512 * 1024) {
        error("file too large.\n");
    }
    api_fread(filebuf, fsize, i);
    api_fclose(i);

    /* 判读图片类型, 若非 BMP 或 JPG 则退出应用程序 */
    if (info_BMP(&env, info, fsize, filebuf) == 0) {
        if (info_JPEG(&env, info, fsize, filebuf) == 0) {
            api_putstr0("file type unknown.\n");
            api_end();
        }
    }
    /* info 各字节含义, */
    /*  info[0] : 图片类型(1:BMP, 2:JPEG) */
    /*  info[1] : 色彩模式 */
    /*  info[2] : 图片x方向像素点数 */
    /*  info[3] : 图片y方向像素点数 */
    
    /* 若图片x方向或y方向像素点数大于屏幕像素点数则进行错误提示并退出应用程序 */
    if (info[2] > 1024 || info[3] > 768) {
        error("picture too large.\n");
    }

    /* 打开画面信息缓冲区为winbuf(xsize, info[3]+37)标题为gview无透明色的窗口 */
    xsize = info[2] + 16;
    if (xsize < 136) {
        xsize = 136;
    }
    win = api_openwin(winbuf, xsize, info[3] + 37, -1, "gview");

    /* 将图片内容转换为RGB颜色信息到picbuf缓冲区中 */
    if (info[0] == 1) {
        i = decode0_BMP (&env, fsize, filebuf, 4, (char *) picbuf, 0);
    } else {
        i = decode0_JPEG(&env, fsize, filebuf, 4, (char *) picbuf, 0);
    }
    if (i != 0) {
        error("decode error.\n");
    }

    /* 将picbuf中的RGB信息转换为色号信息存于窗口缓冲区中,然后刷新窗口画面 */
    for (i = 0; i < info[3]; i++) {
        p = winbuf + (i + 29) * xsize + (xsize - info[2]) / 2;
        q = picbuf + i * info[2];
        for (j = 0; j < info[2]; j++) {
            p[j] = rgb2pal(q[j].r, q[j].g, q[j].b, j, i);
        }
    }
    api_refreshwin(win, (xsize - info[2]) / 2, 29, (xsize - info[2]) / 2 + info[2], 29 + info[3]);

    /* 窗口接收到 'Q' 或 'q' 时退出 */
    for (;;) {
        i = api_getkey(1);
        if (i == 'Q' || i == 'q') {
            api_end();
        }
    }
}

unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
	static int table[4] = { 3, 1, 0, 2 };
	int i;
	x &= 1; /* ��������� */
	y &= 1;
	i = table[x + y * 2];	/* ���ԐF����邽�߂̒萔 */
	r = (r * 21) / 256;	/* ����� 0�`20 �ɂȂ� */
	g = (g * 21) / 256;
	b = (b * 21) / 256;
	r = (r + i) / 4;	/* ����� 0�`5 �ɂȂ� */
	g = (g + i) / 4;
	b = (b + i) / 4;
	return 16 + r + g * 6 + b * 36;
}

void error(char *s)
{
	api_putstr0(s);
	api_end();
}
