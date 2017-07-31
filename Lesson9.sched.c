//sched.c是内核中有关任务（进程）调度管理的程序，其中包括有关调度的基本函数
//以及一些简单的系统调用函数。系统时钟中断处理过程中调用的定时函数也被放置在
//本程序中。

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sys.h>
#include <asm/system.h>
#include <asm/io.h>
#include <serial_debug.h>

extern int timer_interrupt(void); //时钟中断处理程序
extern int system_call(void);     //系统调用中断处理程序

//每个进程在内核态运行时都有自己的内核态堆栈。这里定义了任务的内核态堆栈结构。
union task_union //定义任务共用体
{
    struct task_struct task; //一个任务的数据结构与其内核态堆栈在同一内存页中，
    char stack[PAGE_SIZE];   //所以从堆栈段寄存器ss可以获得其数据段选择符
};

static union task_union init_task = {
    INIT_TASK,
}; //定义初始任务的数据

long user_stack[PAGE_SIZE >> 2];
long startup_time;                               //开机时间，从1970年1月1日0:00开始计时的秒数。
struct task_struct *current = &(init_task.task); //当前任务指针（初始化指向任务0）
struct task_struct *last_task_used_math = NULL;  //使用过协处理器任务的指针
struct task_struct *task[NR_TASKS] = {
    &(init_task.task),
}; //定义任务指针数组

//定义用户堆栈，共1K项，容量4K字节。在内核态初始化操作过程中被用作内核栈，初始化
//完成后被用作任务0的用户态堆栈。在运行任务0之前它是内核栈，以后用作任务0和1的用户态
//栈。下面结构用于设置堆栈ss:esp（数据段选择符：指针），见head.s.
//ss被设置为内核数据段选择符(0x10)，指针esp指在user_stack数组最后一项后面。这是
//因为Intel CPU执行堆栈操作时先递减堆栈指针sp值，然后在sp指针处保存入栈内容。
struct
{
    long *a;
    short b;
} stack_start = {&user_stack[PAGE_SIZE >> 2], 0x10};

//把当前任务置为不可中断的等待状态，并让睡眠队列头指针指向当前任务。
//只有明确地唤醒时才会返回。该函数提供了进程与中断处理程序之间的同步机制。
//函数参数p是等待任务队列头指针。指针是含有一个变量地址的变量。这里参数p使用了指针的
//指针形式**p，这是因为C函数参数只能传值，没有直接的方式让被调用函数改变调用该参数
//程序中变量的值。但是指针*p指向的目标（这里是任务结构）会改变，因此为了能修改调用该
//函数程序中原来就是指针变量的值，就需要传递指针*p的指针，即**p.
void sleep_on(struct task_struct **p)
{
    struct task_struct *tmp;

    //若指针无效，则退出。指针所指的对象可以是NULL，但指针本身不应该为0.另外，如果当前
    //任务是任务0，则死机。因为任务0的运行不依赖自己的状态，所以内核代码把任务0置为睡眠
    //状态毫无意义。
    if (!p)
        return;
    if (current == &(init_task.task))
        panic("task[0] trying to sleep");

    //让tmp指向已经在等待队列上的任务（如果有）。并且将睡眠队列头的等待指针指向当前任务。
    //这样就把当前任务插入到了*p的等待队列中。然后将当前任务置为不可中断的等待状态，并执
    //行重新调度。
    tmp = *p;
    *p = current;
    current->state = TASK_UNINTERRUPTIBLE;
    schedule();

    //只有当这个等待任务被唤醒时，调度程序才又返回到这里，表示本进程已被明确地唤醒（就绪态）
    //既然所有进程都在等待同样的资源，在资源可用时，就有必要唤醒所有等待该资源的进程。该函数
    //嵌套调用，也会嵌套唤醒所有等待该资源的进程。这里嵌套调用是指当一个进程调用了sleep_on()
    //后就会在该函数中被切换掉，控制权被转移到其他进程中。此时若有进程也需要同一资源，那么也会
    //使用同一个等待队列头指针作为参数调用sleep_on()函数，并且也会陷入该函数而不会返回。只
    //有当内核某处代码以队列头指针作为参数wake_up该队列，那么当系统切换去执行头指针所指的进程
    //A时，该进程才会继续执行下面的if(tmp)之后的语句，把队列后一个进程B置为就绪状态（唤醒）。
    //而轮到B进程执行时，它也才可能继续执行if(tmp)之后的语句。若它后面还有等待的进程C，那么它
    //也会把C唤醒等。
    *p = tmp;
    if (tmp) //若在其前还存在等待的任务，则也将其置为就绪状态（唤醒）。
        tmp->state = TASK_RUNNING;
}

void schedule(void)
{
    // s_printk("schedule()\n");
    // 暂不考虑信号处理
    int i, next, c;
    struct task_struct **p;

    while (1)
    {
        // 初始化：i, p指向任务链表的末尾
        c = -1;
        next = 0;
        i = NR_TASKS;
        p = &task[NR_TASKS];
        while (--i)
        {
            // 跳过无效任务（空）
            if (!*(--p))
                continue;
            if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
            {
                c = (*p)->counter;
                next = i;
            }
        }
        // 如果循环之后，系统中的任务只有counter > 0的或者没有任何可以
        // 运行的任务，那么就退出循环并进行任务切换
        if (c)
            break;
        for (p = &LAST_TASK; p > &FIRST_TASK; p--)
        {
            if (*p)
            {
                (*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
            }
        }
    }
    // switch_to 接收参数为一个 Task 号
    // show_task_info(task[next]);
    // printk("[%x] Scheduler select task %d\n", jiffies, next);
    // printk("[%x] Scheduler select task\n", next);

    s_printk("[DEBUG] [%x] Scheduler select task %d\n", jiffies, next); // THE 'next' VALUE IS NOT CORRECT!
    s_printk("[DEBUG] Scheduler select task %d\n", next);               // THIS CAUSE CRASH
    // TODO Fix the bug
    // These two lines of code will cause OS Crash
    // When switch to printk it won't
    // Why?
    switch_to(next)
}

void show_task_info(struct task_struct *task)
{
    s_printk("Current task Info\n================\n");
    s_printk("pid = %d\n", task->state);
    s_printk("counter = %d\n", task->counter);
    s_printk("start_code = %x\n", task->start_code);
    s_printk("end_code = %x\n", task->end_code);
    s_printk("brk = %x\n", current->ldt[0]);
    s_printk("gid = 0x%x\n", current->gid);
    s_printk("tss.ldt = 0x%x\n", current->tss.ldt);
    // s_printk("tss.eip = 0x%x\n", current->eip);
}

//唤醒*p指向的任务。*p是任务等待队列头指针。由于新等待任务是插入在等待
//队列头指针处的，因此唤醒的是最后进入等待队列的任务。
void wake_up(struct task_struct **p)
{
    if (p && *p)
    {
        (*p)->state = TASK_RUNNING;
        *p = NULL;
    }
}

// 将当前任务置为可中断的等待状态，并放入*p指定的等待队列中。
void interruptible_sleep_on(struct task_struct **p)
{
    struct task_struct *tmp;
    //若指针无效，则退出。指针所指的对象可以是NULL，但指针本身不会是0.如果
    //任务是任务0，则死机。
    if (!p)
        return;
    if (current == &(init_task.task))
        panic("task[0] trying to sleep");
    //让tmp指向已经在等待队列上的任务（如果有）。并且将睡眠队列头的等待指针指向
    //当前任务。这样就把当前任务插入到了*p的等待队列中。然后将当前任务置为可中断
    //的等待状态，并执行重新调度。
    tmp = *p;
    *p = current;
rep_label:
    current->state = TASK_INTERRUPTIBLE;
    //这里会转到其他任务去执行
    schedule();
    //只有当这个等待任务被唤醒时，程序才又会返回到这里，表示进程已被明确地唤醒并
    //执行。如果等待队列中还有等待任务，并且队列头指针所指向的任务不是当前任务时，
    //则将该等待任务置为可运行的就绪状态，并重新执行调度程序。当指针*p所指向的
    //不是当前任务时，表示在当前任务被放入队列后，又有新的任务被插入等待队列前部。
    //因此我们先唤醒它们，而让自己仍然等待。等待这些后续进入队列的任务被唤醒执行来
    //唤醒本任务。于是去执行重新调度。
    if (*p && *p != current)
    {
        (*p)->state = TASK_RUNNING;
        goto rep_label;
    }
    *p = tmp;
    if (tmp)
        tmp->state = TASK_RUNNING;
}

int sys_pause(void)
{
    current->state = TASK_INTERRUPTIBLE;
    schedule();
    return 0;
}

// 计时器函数 do_timer, 记录程序运行时间，如果处于 CPL = 2 的进程执行时间超过时间片
// 则进行调度，否则继续。

// 下面的这段代码是一个demo，用来验证时钟中断；已经设置好并且可用, 目前不支持其他定时器
// floppy操作依旧不支持
int counter = 0;
long volatile jiffies = 0;
void do_timer(long cpl)
{
    if (!cpl)
        current->stime++;//内核态时间
    else
        current->utime++;//用户态时间
    if ((--current->counter) > 0)
        return;
    current->counter = 0;
    if (!cpl)
        return;
    schedule();
}

// 这是一个临时函数，用于初始化8253计时器
// 并开启时钟中断
void sched_init()
{
    int divisor = 1193180 / HZ;

    int i;
    struct desc_struct *p;

    // 初始化任务0的TSS, LDT
    set_tss_desc(gdt + FIRST_TSS_ENTRY, &(init_task.task.tss));
    set_ldt_desc(gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));

    // 初始化其余各项,因为TSS LDT各占一个，所以
    // +2 指向任务1的TSS, LDT(初始任务为0)
    // 把其余各项设置为0
    p = gdt + 2 + FIRST_TSS_ENTRY;
    for (i = 1; i < NR_TASKS; i++)
    {
        task[i] = NULL;
        p->a = p->b = 0;
        p++;
        p->a = p->b = 0;
        p++;
    }

    // Clear Nested Task(NT) Flag
    __asm__("pushfl; andl $0xffffbfff, (%esp); popfl");
    ltr(0);
    lldt(0);

    outb_p(0x43, 0x36);
    outb_p(0x40, divisor & 0xFF);
    outb_p(0x40, divisor >> 8);

    // timer interrupt gate setup: INT 0x20
    set_intr_gate(0x20, &timer_interrupt);
    // Make 8259 accept timer interrupt
    outb(0x21, inb_p(0x21) & ~0x01);

    // 初始化 system_call
    set_system_gate(0x80, &system_call);
}
