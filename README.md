### haribote_meeting
---
`haribote_meeting` just only means haribote OS learning, reading and annotating.

I had tried my best to read and annotate haribote OS during 2019.06 ~ 2019.11, the small fragment following extracted from harib27f/haribote/graphic.c.
```C
/* boxfill8,
 * 用色号c充当窗口[(x0,y0),(x1,y1)]区域画面信息,窗口x方向像素点数为xsize。
 * |===========================|
 * | (x0,y0)|-------|          |
 * |        |ccccccc|          |
 * |        |-------|(x1,y1)   |
 * |                           |
 * |===========================|
 * |<-----window xsize-------->|
 * 
 * vram, 用于缓存窗口的画面信息;
 * [(x0,y0),(x1,y1)], 基于窗口左上角的窗口区域;
 * c, 填充窗口[(x0,y0),(x1,y1)]区域的色号。
 *
 * 当vram为显存基址,xsize为屏幕x箱数点时,屏幕[(x0,y0),(x1,y1)]区域将会直接显示色号c对应的RGB颜色。*/
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;

    /* 将色号c写入窗口像素(x,y)处所对应缓存中 */
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    }

    return;
}
```
Goddess hopes more knowledgeable guys just like you can  ontinue to improve haribote_meeting together.
