/* file.c, 文件读取程序接口 */

/* 粗略理解haribote文件系统。
 * 
 * 回看ipl09.nas,FAT12格式软盘内容大体如下。
 * ---------------------------
 * |保留区域|FAT区域|数据区域|
 * ---------------------------
 * 保留区域共1个扇区(0柱面,0磁头,1扇区),包含了对FAT12总体描述和haribote启动代码;
 * FAT区域共有2个FAT(第2个是第1个的备份),每个占9个扇区(0柱面,0磁头,[2,19]扇区)。
 * 
 * 回看asmhead.nas,整个FAT12软盘被加载到内存地址空间[0x100000, 0x267fff]所对应的
 * 内存段中——软盘映像。*/

/* 根据作者在书中的提示,将FAT12格式软盘内容粗略细化。
 * --------------------------------------------
 * |保留区域|FAT区域|文件信息区域|文件内容区域
 * --------------------------------------------
 * 1        2       20           34
 * 保留区域: 1扇区;
 * FAT区域: [2, 20)扇区,用于索引文件大于512字节部分内容;
 * 文件信息区域: [20, 34)扇区,1个文件信息由 struct FILEINFO 结构体描述;
 * 文件内容: [34,...)扇区
 * 
 * 文件信息中的簇号(扇区号)clustno能够索引文件开始的512字节内容,  接下
 * 来512字节内容的索引为clustno=FAT[clustno]索引,...当clustno在FF8~FFF
 * 范围时,则表示文件内容结束。
 * 
 * 由簇号计算文件内容在软盘中偏移地址的公式可归纳为clustno*512+0x3e00h
 * 那么,文件内容在软盘映像中的地址=clustno * 512 + 0x3e00 + 0x00100000。
 *
 * FAT12各个区域所占大小跟FAT所在介质总大小有关。*/


#include "bootpack.h"

/* file_readfat,
 * 从软盘映像起始处img读取文件分配表即FAT到fat所指内存从。*/
void file_readfat(int *fat, unsigned char *img)
{
    int i, j = 0;

    /* FAT中的内容是经过压缩的: 用3个字节保存2个扇区号, 即
     * 1个扇区号占用1.5字节;根据FAT压缩规律将FAT中的扇区号
     * 信息读取到fat所指内存中。FAT中扇区号的压缩规律为
     * 123 456 --> 23 61 45 即
     * 将两个需4字节保存的扇区号0x123和0x456压缩后用3字节保
     * 存。以下程序按照该规律解压FAT到fat所指内存中。*/
    for (i = 0; i < 2880; i += 2) {
        fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
/* 关于2880: FAT最多保存2880个扇区号(1.44Mb软盘共2880个扇区)。FAT
 * 被压缩后,FAT最多占用2880*1.5字节软盘即不到9扇区,这是在保留区域
 * (ipl09.nas)中填写FAT占9扇区的来源。*/
    return;
}

/* file_loadfile,
 * 从软盘映像(缓存软盘内容内存段)img中从读取size字节文件内容
 * 到buf所指内存段,clustno为文件的起始簇号,fat所指内存段为FAT。*/
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
    int i;

    /* 文件的起始簇号clustno在文件信息中,之后簇号为
     * FAT[clustno]。第n个clustno*512将索引文件第n扇区内容。*/
    for (;;) {
        if (size <= 512) {
            for (i = 0; i < size; i++) {
               buf[i] = img[clustno * 512 + i];
            }
            break;
        }
        for (i = 0; i < 512; i++) {
            buf[i] = img[clustno * 512 + i];
        }
        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
    return;
}

/* file_search,
 * 在finfo所指软盘映像的文件信息区域(共max个文件信息)中搜索name所指
 * 目标命名文件,若搜索成功则返回目标文件的文件信息首地址,失败则返回0。*/
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
    int i, j;
    char s[12];

    /* s用于保存文件名,文件名不足12字符补齐空格 */
    for (j = 0; j < 11; j++) {
        s[j] = ' ';
    }
    
    j = 0;
    for (i = 0; name[i] != 0; i++) {
        if (j >= 11) { return 0; /* 文件名太长则不予处理 */ }

        /* 保证文件名(不含后缀)占8字符 */
        if (name[i] == '.' && j <= 8) {
            j = 8;
        /* 将name所指文件名拷贝到s所指栈内存中 */
        } else {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z') {
                /* 将文件名转为大写字符 */
                s[j] -= 0x20;
            } 
            j++;
        }
    }
    /* 软盘映像共max个文件信息 */
    for (i = 0; i < max; ) {
        /* 此处应该是finfo[i]->name == 0x00。当name第
         * 一字节为0x00时表文件信息区域再无文件信息。*/
        if (finfo->name[0] == 0x00) {
            break;
        }
        /* 若当前文件信息非目录且为type低3位所表征文件,若在
         * 软盘映像中找到目标文件名则返回其文件信息首地址。*/
        if ((finfo[i].type & 0x18) == 0) {
            for (j = 0; j < 11; j++) {
                if (finfo[i].name[j] != s[j]) {
                    goto next;
                }
            }
            return finfo + i;
        }
next:
        i++;
    }
    return 0; /* 目标文件不在软盘映像中则返回0 */
}

/* file_loadfile2,
 * 读取起始簇号为clustno文件内容,共度*psize字节。该函数返回
 * 所读文件内容在内存中的首地址和成功读取的字节数(*psize中)。*/
char *file_loadfile2(int clustno, int *psize, int *fat)
{
    int size = *psize, size2;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char *buf, *buf2;

    /* 分配用于文件读取的内存 */
    buf = (char *) memman_alloc_4k(memman, size);

    /* 从软盘映像中读取指定起始簇号文件的size字节内容到buf中,clustno * 512 + 
     * ADR_DISKIMG + 0x003e00得到clustno对应512字节文件内容在软盘映像中的地址 */
    file_loadfile(clustno, size, buf, fat, (char *) (ADR_DISKIMG + 0x003e00));

    /* 为能在软盘映像中存放更多文件, haribote软盘映像支持文件压缩功能,
     * 即将文件内容压缩后存储。此处解压所读取到的文件内容, 然后释放掉
     * buf原指向的包含压缩数据的内存段,并将解压后文件大小赋给出参, 最
     * 后返回解压文件数据在内存中首地址。*/
    if (size >= 17) { /* tek压缩文件开头带17字节用于标识tek文件 */
        size2 = tek_getsize(buf);
        if (size2 > 0) {
            buf2 = (char *) memman_alloc_4k(memman, size2);
            tek_decomp(buf, buf2, size2);
            memman_free_4k(memman, (int) buf, size);
            buf = buf2;
            *psize = size2;
        }
    }
/* tek是作者自制的解压缩算法,对"压缩率","解压速度"以及"解压程序大小"
 * 有一定权衡。看作者对压缩特性的分析和认识,tek解压缩算法应当相当优
 * 秀,不过此文还不打算阅读该解压缩算法相关细节。*/
    return buf;
}
