/* 应用程序 calc.c, 命令行计算器 */

/* 此程序用递归和优先级参数实现了命令行计算器,
 * 比较难以被理解。在 ktv 听菩萨坝党派人士唱歌
 * 的那段时间里也没有清晰地想明白该程序的实现方式。*/

/* 系统调用声明头文件,C标准库头文件 */
#include "apilib.h"
#include <stdio.h>  /* sprintf */

/* 用32位有符号第2小负数(最大正数溢出2)作为无效数据标识 */
#define INVALID -0x7fffffff

/* 声明 stdlib.h 中的函数 */
int strtol(char *s, char **endp, int base);

char *skipspace(char *p);
int getnum(char **pp, int priority);

void HariMain(void)
{
    int i;
    char s[30], *p;

    /* 获取本任务命令行参数 */
    api_cmdline(s, 30);

    /* 跳过应用程序名 calc */
    for (p = s; *p > ' '; p++) { }
    i = getnum(&p, 9);

    /* 在当前窗口中以十进制和十六进制输出计算结果 */
    if (i == INVALID) {
        api_putstr0("error!\n");
    } else {
        sprintf(s, "= %d = 0x%x\n", i, i);
        api_putstr0(s);
    }
    
    /* 退出应用程序 */
    api_end();
}

/* 跳过p所指内存段中的空格,返回非空格字符的地址 */
char *skipspace(char *p)
{
    for (; *p == ' '; p++) { }
    return p;
}

/* 命令行计算器支持的运算符及运算符优先级由高到低[0~6]
 * +, -, ~, ();
 * *, /, %;
 * +, -;
 * <<, >>;
 * &;
 * ^;
 * |;
 * 同优先级运算符结合性由左至右。
 *
 * 如 1+2*3+4 用递归和优先级 priority 保证的计算顺序为 2*3, 1+6, 7+4;
 * 如 1-2+3   用递归和优先级 priority 保证的计算顺序为 1-2, -1+3 */
int getnum(char **pp, int priority)
{
    char *p = *pp;
    int i = INVALID, j;

    /* 跳过空格 */
    p = skipspace(p);

    /* 首先获取带单目运算符的数字,如 +1, -1, ~1 */
    if (*p == '+') {
        p = skipspace(p + 1);
        i = getnum(&p, 0);
    } else if (*p == '-') {
        p = skipspace(p + 1);
        i = getnum(&p, 0);
        if (i != INVALID) {
            i = - i;
        }
    } else if (*p == '~') {
        p = skipspace(p + 1);
        i = getnum(&p, 0);
        if (i != INVALID) {
            i = ~i;
        }
    /* 处理括号内的表达式 */
    } else if (*p == '(') {
        p = skipspace(p + 1);
        i = getnum(&p, 9);
        if (*p == ')') {
            p = skipspace(p + 1);
        } else {
            i = INVALID;
        }
    /* 将参与运算的数字字符串转换为十进制数字 */
    } else if ('0' <= *p && *p <= '9') {
        i = strtol(p, &p, 0);
    } else {
        i = INVALID;
    }

    /* 处理带两个操作数的运算符,保证高优先级运算符的
     * 表达式先被计算,同优先级表达式结合性由左至右。*/
    for (;;) {
        if (i == INVALID) {
            break;
        }
        p = skipspace(p);
        if (*p == '+' && priority > 2) {
            p = skipspace(p + 1);
            j = getnum(&p, 2);
            if (j != INVALID) {
                i += j;
            } else {
                i = INVALID;
            }
        } else if (*p == '-' && priority > 2) {
            p = skipspace(p + 1);
            j = getnum(&p, 2);
            if (j != INVALID) {
                i -= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '*' && priority > 1) {
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID) {
                i *= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '/' && priority > 1) {
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID && j != 0) {
                i /= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '%' && priority > 1) {
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID && j != 0) {
                i %= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '<' && p[1] == '<' && priority > 3) {
            p = skipspace(p + 2);
            j = getnum(&p, 3);
            if (j != INVALID && j != 0) {
                i <<= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '>' && p[1] == '>' && priority > 3) {
            p = skipspace(p + 2);
            j = getnum(&p, 3);
            if (j != INVALID && j != 0) {
                i >>= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '&' && priority > 4) {
            p = skipspace(p + 1);
            j = getnum(&p, 4);
            if (j != INVALID) {
                i &= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '^' && priority > 5) {
            p = skipspace(p + 1);
            j = getnum(&p, 5);
            if (j != INVALID) {
                i ^= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '|' && priority > 6) {
            p = skipspace(p + 1);
            j = getnum(&p, 6);
            if (j != INVALID) {
                i |= j;
            } else {
                i = INVALID;
            }
        } else {
            break;
        }
    }
    p = skipspace(p);
    *pp = p;
    return i;
}
