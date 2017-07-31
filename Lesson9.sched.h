//调度程序头文件，定义了任务结构 task_struct、初始任务0的数据等。
#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64 // 系统中同时最多有 64 个进程（任务）
#define HZ 100      // 时钟频率 100 hz

#define FIRST_TASK task[0]           //任务0 比较特殊，特意给它单独定义一个符号
#define LAST_TASK task[NR_TASKS - 1] //任务数组中的最后一项任务

#include <linux/mm.h>   //内存管理头文件。含有页面大小定义和一些页面释放函数原型。
#include <linux/head.h> //head头文件，定义了段描述符的简单结构，和几个选择符常量。
#include <signal.h>     //信号头文件。

//定义进程运行可能处于的状态。
#define TASK_RUNNING 0         //进程正在运行或已准备就绪。
#define TASK_INTERRUPTIBLE 1   //进程处于可中断等待状态。
#define TASK_UNINTERRUPTIBLE 2 //进程处于不可中断等待状态，主要用于I/O操作等待。
#define TASK_ZOMBIE 3          //进程处于僵死状态，已经停止运行，但父进程还没有发信号。
#define TASK_STOPPED 4         //进程已停止。

#ifndef NULL
#define NULL ((void *)0) //定义NULL为空指针。
#endif

//复制进程的页目录页表。
extern int copy_page_tables(unsigned long from, unsigned long to, unsigned long size);
//释放页表所指定的内存块及页表本身。
extern int free_page_tables(unsigned long from, unsigned long size);

//进程调度函数。
extern void schedule(void);

typedef int (*fn_ptr)(); //定义函数指针类型。

//下面是数学协处理器使用的结构，主要用于保存进程切换时i387的执行状态信息。
struct i387_struct
{
    long cwd;          // 控制字(Control word)
    long swd;          // 状态字(Status word)
    long twd;          // 标记字(Tag word)
    long fip;          // 协处理器代码指针IP
    long fcs;          // 协处理器代码段寄存器CS
    long foo;          // 内存操作数的偏移位置
    long fos;          // 内存操作数的段值
    long st_space[20]; // 8个10字节的协处理器累加器
};

//任务状态段数据结构，包含了 i387 协处理器的结构
struct tss_struct
{
    long back_link;
    long esp0;
    long ss0;
    long esp1;
    long ss1;
    long esp2;
    long ss2;
    long cr3;
    long eip;
    long eflags;
    long eax, ecx, edx, ebx; //顺序很重要
    long esp;
    long ebp;
    long esi;
    long edi;
    long es;
    long cs;
    long ss;
    long ds;
    long fs;
    long gs;
    unsigned long ldt;
    unsigned long bitmap;
    struct i387_struct i387;
};

//任务（进程）数据结构，或称为进程描述符
struct task_struct
{
    // --- 硬编码部分，下面不应修改 ---
    long state;
    long counter;  // 时间片计数器
    long priority; // 优先级
    // 信号相关结构，分别为信号集、响应信号的行为、屏蔽的信号集
    long signal;
    struct sigaction sigaction[32];
    long blocked;
    // --- 硬编码部分结束 ---
    int exit_code; // 其父进程会读取该任务的 exit_code
    unsigned long start_code;
    unsigned long end_code;
    unsigned long end_data;
    unsigned long brk;         // 总长度brk
    unsigned long start_stack; // 堆栈段地址

    long pid;     // pid 进程ID
    long father;  // 父进程ID
    long pgrp;    // 进程组ID
    long session; // 会话(session)ID
    long leader;  // 会话(session)的首领

    unsigned short uid;  // 用户ID
    unsigned short euid; // 有效用户ID
    unsigned short suid; // SetUID
    unsigned short gid;  // 组ID
    unsigned short egid; // 有效组ID
    unsigned short sgid; // SetGID

    long alarm;      // 报警定时值 单位: jiffies
    long utime;      // 用户态运行时间
    long stime;      // 内核态运行时间
    long cutime;     // 子进程用户态运行时间
    long cstime;     // 子进程内核态运行时间
    long start_time; // 进程开始运行的时点

    unsigned short used_math; // 是否使用了协处理器

    // int tty;
    // 下面是和文件系统相关的变量，暂时不使用，先注释掉
    // unsigned short umask;
    // struct m_inode *pwd;
    // struct m_inode *executable;
    // unsigned long close_on_exec;
    // struct file * filp[NR_OPEN];
    struct desc_struct ldt[3]; // LDT 第一项为空， 第二项是代码段，第三项是数据段（也是堆栈段)
    struct tss_struct tss;     // 该进程的TSS结构
};

//INIT_TASK 用于设置第1个任务表；不应修改。
//基址 Base = 0，段长 limit = 0x9ffff (= 640kB)
#define INIT_TASK                                                                                                                                                                              \
    /* state info */ {                                                                                                                                                                         \
        0, 15, 15,                                                                                                                                                                             \
            /* signals */ 0, {                                                                                                                                                                 \
                                 {},                                                                                                                                                           \
                             },                                                                                                                                                                \
            0, /* exit_code, brk */ 0, 0, 0, 0, 0, 0, /* pid */ 0, -1, 0, 0, 0, /* uid */ 0, 0, 0, 0, 0, 0, /* alarm, etc... */ 0, 0, 0, 0, 0, 0, /* math */ 0, /* LDT */ {                    \
                                                                                                                                                   {0, 0}, {0x9f, 0xc0fa00}, {0x9f, 0xc0f200}, \
                                                                                                                                               },                                              \
        /* TSS */ {                                                                                                                                                                            \
            0, PAGE_SIZE + (long)&init_task, 0x10, 0, 0, 0, 0, (long)&pg_dir,                                                                                                                  \
                0, 0, 0, 0, 0, 0, 0, 0,                                                                                                                                                        \
                0, 0, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,                                                                                                                                      \
                _LDT(0), 0x80000000,                                                                                                                                                           \
            {                                                                                                                                                                                  \
            }                                                                                                                                                                                  \
        }                                                                                                                                                                                      \
    }

extern struct task_struct *task[NR_TASKS];      //任务指针数组
extern struct task_struct *last_task_used_math; //上一个使用过协处理器的进程
extern struct task_struct *current;             //从开机起计算的jiffies (10 ms / 1 jiffy)
extern long volatile jiffies;
extern long start_time; //开机时间，从1970年1月1日00:00开始计时

#define CURRENT_TIME (start_time + jiffies / HZ) //当前时间（秒数）

//添加定时器函数（定时时间jiffies，定时到时调用函数*fn()）
extern void add_timer(long *jiffies, void (*fn)(void));
//不可中断的等待睡眠。
extern void sleep_on(struct task_struct **p);
//可中断的等待睡眠。
extern void interruptible_sleep_on(struct task_struct **p);
//明确唤醒睡眠的进程。
extern void wake_up(struct task_struct **p);
extern void show_task_info(struct task_struct *task);

#define FIRST_TSS_ENTRY 4                     //全局表中第1个任务状态段(TSS)描述符的选择符索引号
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY + 1) //全局表中第1个局部描述符表(LDT)描述符的选择符索引号。
//下面计算出TSS，LDT在GDT中的偏移量
//_TSS(n)表示第n个TSS，计算方式为，第一个 TSS 的入口为 4 << 3(因为每一个Entry占8Byte, 所以第4个的偏移量为4 << 3)
#define _TSS(n) ((((unsigned long)n) << 4) + (FIRST_TSS_ENTRY << 3))
#define _LDT(n) ((((unsigned long)n) << 4) + (FIRST_LDT_ENTRY << 3))
#define ltr(n) __asm__ volatile("ltr %%ax" ::"a"(_TSS(n)))
#define lldt(n) __asm__ volatile("lldt %%ax" ::"a"(_LDT(n)))
//取出当前的任务号，返回为n: 当前任务号（和进程号不一样)
#define str(n)                            \
    __asm__ volatile("str %%ax\n\t"       \
                     "subl %2, %%eax\n\t" \
                     "shrl $4, %%eax\n\t" \
                     : "=a"(n)            \
                     : "a"(0), "i"(FIRST_TSS_ENTRY << 3))

// #define PAGE_ALIGN(n) (((n) + 0xfff) & 0xfffff000) 这一行虽然在原有的linux0.11中，不过在整个repo中都没有使用到

#define switch_to(n)                                                                              \
    {                                                                                             \
        struct                                                                                    \
        {                                                                                         \
            long a, b;                                                                            \
        } __tmp;                                                                                  \
        __asm__ volatile("cmpl %%ecx, current\n\t"                                                \
                         "je 1f\n\t"                                                              \
                         "movw %%dx, %1\n\t"                                                      \
                         "xchgl %%ecx, current\n\t"                                               \
                         "ljmp *%0\n\t" /* 长跳跃到新的TSS选择符，表示任务切换 */ \
                         "cmpl %%ecx, last_task_used_math\n\t"                                    \
                         "jne 1f\n\t"                                                             \
                         "clts\n\t"                                                               \
                         "1:" ::"m"(*&__tmp.a),                                                   \
                         "m"(*&__tmp.b),                                                          \
                         "d"(_TSS(n)), "c"((long)task[n]));                                       \
    }

#define _set_base(addr, base)                              \
    __asm__ volatile(/* "push %%edx\n\t" */                \
                     "movw %%dx, %0\n\t"                   \
                     "rorl $16, %%edx\n\t"                 \
                     "movb %%dl, %1\n\t"                   \
                     "movb %%dh, %2\n\t" /* "pop %%edx" */ \
                     ::"m"(*((addr) + 2)),                 \
                     "m"(*((addr) + 4)),                   \
                     "m"(*((addr) + 7)),                   \
                     "d"(base) /*:"dx"*/                   \
                     )

#define _set_limit(addr, limit) \
    __asm__ volatile(           \
        "movw %%dx, %0\n\t"     \
        "rorl $16, %%edx\n\t"   \
        "movb %1, %%dh\n\t"     \
        "andl $0xf0, %%dh\n\t"  \
        "orb %%dh, %%dl\n\t"    \
        "mov %%dl %1"           \
        : "m"(*(addr)),         \
          "m"((*(addr) + 6)) "d"(limit))

#define set_base(ldt, base) _set_base(((char *)&(ldt)), (base))
// limit >> 12 是因为当Descriptor中G位置位的时候，Limit单位是4KB
#define set_limit(ldt, limit) _set_limit(((char *)ldt), (limit - 1) >> 12)

static inline unsigned long _get_base(char *addr)
{
    unsigned long __base;
    __asm__ volatile("movb %3, %%dh\n\t"
                     "movb %2, %%dl\n\t"
                     "shll $16, %%edx\n\t"
                     "movw %1, %%dx\n\t"
                     : "=&d"(__base)
                     : "m"(*((addr) + 2)),
                       "m"(*((addr) + 4)),
                       "m"(*((addr) + 7)));
    return __base;
}

#define get_base(ldt) _get_base(((char *)&(ldt)))
#define get_limit(segment) ({                 \
    unsigned long __limit;                    \
    __asm__ volatile("lsll %1, %0\n\tincl %0" \
                     : "=r"(__limit)          \
                     : "r"(segment));         \
    __limit;                                  \
})

#endif
