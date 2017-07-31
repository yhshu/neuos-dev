//sched.c是内核中有关任务（进程）调度管理的程序，其中包括有关调度的基本函数
//以及一些简单的系统调用函数。系统时钟中断处理过程中调用的定时函数也被放置在
//本程序中。

#include <linux/sched.h>
#include <linux/kernel.h> //内核头文件。含有一些内核常用函数的原形定义。
#include <linux/mm.h>     //内存管理头文件。
#include <asm/system.h>   //系统头文件。定义了设置或修改描述符/中断门等的嵌入式汇编宏。
#include <asm/io.h>       //io头文件。定义硬件端口输入/输出宏汇编语句。

//每个进程在内核态运行时都有自己的内核态堆栈。这里定义了任务的内核态堆栈结构。
union task_union //共用体
{
    struct task_struct task;
    char stack[PAGE_SIZE];
};

static union task_union init_task = {
    INIT_TASK,
}; //定义初始任务的数据

//定义用户堆栈，共1K项，容量4K字节。在内核态初始化操作过程中被用作内核栈，初始化
//完成后被用作任务0的用户态堆栈。在运行任务0之前它是内核栈，以后用作任务0和1的用户态
//栈。下面结构用于设置堆栈ss:esp（数据段选择符：指针），见head.s.
//ss被设置为内核数据段选择符(0x10)，指针esp指在user_stack数组最后一项后面。这是
//因为Intel CPU执行堆栈操作时先递减堆栈指针sp值，然后在sp指针处保存入栈内容。

long user_stack[PAGE_SIZE >> 2];
long startup_time;                               //开机时间，从1970年1月1日0:00开始计时的秒数。
struct task_struct *current = &(init_task.task); //当前任务指针（初始化指向任务0）

struct
{
    long *a;
    short b;
} stack_start = {&user_stack[PAGE_SIZE >> 2], 0x10};

void sleep_on(struct task_struct **p)
{
}

void schedule(void)
{
}

void wake_up(struct task_struct **p)
{
}

void interruptible_sleep_on(struct task_struct **p)
{
}

int sys_pause(void)
{
}

//计时器函数do_timer，记录程序运行时间，如果处于CPL = 2的进程执行时间超过时间片
//则进行调度，否则继续。
//下面这段代码是demo，用于验证时钟中断，已经设置好并且可用。
int counter = 0;
long volatile jiffies = 0;
//系统全局变量，记录系统时钟中断发生的次数。volatile是向编译器指明变量的内容可能
//由于被其他程序修改而变化。编译器会尽量将它放在通用寄存器中。
void do_timer(long cpl)
{
    jiffies++;
    counter++;
    if (counter == 10)
    {
        printk("Jiffies = %d\n", jiffies);
        counter = 0;
    }
    outb(0x20, 0x20);
    //清理中断
}

//这是一个临时函数，用于初始化8253计时器并开启时钟中断
void timer_init()
{
    int divisor = 1193180 / HZ;
    outb_p(0x43, 0x36);
    outb_p(0x40, divisor & 0xFF);
    outb_p(0x40, divisor >> 8);

    //时钟中断：INT 0x20
    set_intr_gate(0x20, &demo_timer_interrupt);
    //使8259允许时钟中断
    outb(0x21, inb_p(0x21) & ~0x01);
}
