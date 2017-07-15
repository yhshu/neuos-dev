//printk()是内核中使用的打印（显示）函数，功能与C标准函数库中的printf()相同。
#include <linux/kernel.h> //内核头文件。含有一些内核常用函数的原型定义。
#include <asm/io.h>
#include <stdarg.h>
#include <stddef.h> //标准定义头文件。

#define PAGE_SIZE 4096
#define VIDEO_MEM 0xB8000
#define VIDEO_X_SZ 80
#define VIDEO_Y_SZ 25
#define CALC_MEM(x, y) (2 * ((x) + 80 * (y)))

long user_stack[PAGE_SIZE >> 2]; //用户栈

struct
{
    long *a;
    short b;
} stack_start = {&user_stack[PAGE_SIZE >> 2], 0x10};

struct video_info
{
    unsigned int retval;    //返回值 return value
    unsigned int colormode; //颜色模式
    unsigned int feature;   //特征设置
};

extern int video_x;
extern int video_y;

char *video_buffer = VIDEO_MEM;

void video_init()
{
    struct video_info *info = 0x9000;

    video_x = 0;
    video_y = 0;
    video_clear();
    update_cursor(video_y, video_x);
}

int video_getx()
{
    return video_x;
}

int video_gety()
{
    return video_y;
}

void update_cursor(int row, int col)
{
    unsigned int pos = (row * VIDEO_X_SZ) + col;
    // LOW Cursor端口到VGA索引寄存器
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    // High Cursor端口到VGA索引寄存器
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
    return;
}

int get_cursor()
{
    int offset;
    outb(0x3D4, 0xF);
    offset = inb(0x3D5) << 8;
    outb(0x3D4, 0xE);
    offset += inb(0x3D5);
    return offset;
}

void printk(char *fmt, ...) //可变参数函数
{
    va_list ap;             //参数列表
    va_start(ap, fmt);      //设置传递给函数的参数列表的第一个可选参数

    char c, *s;
    while (*fmt)
    {
        c = *fmt++;         //先赋值再自增
        if (c != '%')       //如果不是%，输出字符，跳转下一循环
        {
            video_putchar(c);
            continue;
        }
        c = *fmt++;         //如果是%，读取%后面的字符
        if (c == '\0')
            break;
        switch (c)
        {
            case 'd':       //%d：整数
                printnum(va_arg(ap, int), 10, 1);
                break;
            case 'u':       //%u：无符号整数
                printnum(va_arg(ap, int), 10, 0);
                break;
            case 'x':       //%x：16进制整数
                printnum(va_arg(ap, int), 16, 0);
            case 's':       //%s：字符串
                s = va_arg(ap, char *);
                while (*s)
                    video_putchar(*s++);
                break;
            case '%':
                video_putchar('%');
                break;
        }
    }
    return;
}

void printnum(int num, int base ,int sign) //base进制，sign符号
{
    char digits[] = "0123456789ABCDEF";
    char buf[50] = "";
    int cnt = 0;
    int i;

    if(sign && num < 0)             //检查是sign或unsign
    {
        video_putchar('-');         //输出负数 
        num = -num;
    }

    if(num == 0)
    {
        video_putchar('0');
        return ;
    }

    //如果num != 0
    while(num)
    {
        buf[cnt++] = digits[num % base];
        num = num / base;
    }

    for(i = cnt - 1; i >= 0; i--)
    {
        video_putchar(buf[i]);
    }
    return ;
}

void video_clear()  //清屏
{
    int i;
    int j;
    video_x = 0;
    video_y = 0;
    for(i = 0; i < VIDEO_X_SZ; i++) 
    {
        for(j = 0; j < VIDEO_Y_SZ; j++) 
        {
           video_putchar_at(' ', i, j, 0x0F);  //0x0F是亮白；此处不要使用0x00, 否则会失去闪烁的光标！
        }
    }
    return ;
}

void video_putchar_at(char ch, int x, int y, char attr)
{
    if(x >= 80)
        x = 80;
    if(y >= 25)
        y = 25;
    *(video_buffer + 2 * (x + 80 * y)) = ch;            //字符
    *(video_buffer + 2 * (x + 80 * y) + 1) = attr;      //光标
    return ;
}

void video_putchar(char ch)
{
    if(ch == '\n')              //换行符
    {
        video_x = 0;
        video_y++;
    }
    else                        //不是换行符
    {
        video_putchar_at(ch, video_x, video_y, 0x0F);   
        video_x++;
    }
    if(video_x >= VIDEO_X_SZ)   //行满，输出到下一行
    {
        video_x = 0;
        video_y++;
    }
    if(video_y >= VIDEO_Y_SZ)   //列满，滚屏
    {
        roll_screen();
        video_x = 0;
        video_y = VIDEO_Y_SZ - 1;
    }

    update_cursor(video_y, video_x);
    return ;
}

void roll_screen()  //putchar过程中的滚屏
{
    int i;
    //将下一行复制到这一行
    for(i = 1;i < VIDEO_X_SZ; i++)
    {
        memcpy(video_buffer + (i - 1) * 80 * 2, video_buffer + i * 80 * 2, VIDEO_X_SZ, 2 * sizeof(char));
    }
    //清除最后一行
    for(i = 0;i < VIDEO_X_SZ; i++)
    {
        video_putchar_at(' ', i, VIDEO_Y_SZ - 1, 0x0F);
    }
    return ;
}

void memcpy(char *dest, char *src, int count, int size)
{
    int i;
    int j;
    for(i = 0; i < count; i++)
    {
        for(j = 0; j < size; j++)
        {
            *(dest + i * size + j) = *(src + i * size + j);
        }
    } 
    return ;
}
