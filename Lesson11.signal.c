#include <linux/kernel.h>//内核头文件。含有一些内核常用函数的原形定义。
#include <signal.h>//信号头文件
#include <linux/types.h>
#include <serial_debug.h>//串行端口
#include <asm/segment.h>//段操作头文件。定义了有关段寄存器操作的嵌入式汇编函数。
#include <linux/sched.h>//调度程序头文件，定义任务结构task_struct、初始任务0的数据。

#define DEBUG


void dump_sigaction(struct sigaction *action) {
    s_printk("[DEBUG] Sigaction dump\n");
    s_printk("[DEBUG] addr = 0x%x, sa_mask = 0x%x, sa_handler = 0x%x, sa_restorer = 0x%x\n",
             action, action->sa_mask, action->sa_handler, action->sa_restorer);
}

//获取当前任务信号屏蔽位图（屏蔽码）。
int sys_sgetmask() {
    return (int) current->blocked;
}

//设置新的信号屏蔽位图。SIGKILL不能被屏蔽。返回值是原信号屏蔽位图。
int sys_ssetmask(int newmask) {
    int old;
    old = (int) current->blocked;
    current->blocked = (unsigned long) (newmask & ~(1 << (SIGKILL - 1)));
    return old;
}

// 复制sigaction数据到fs段to处。
static inline void save_old(char *from, char *to) {
    unsigned int i;
    verify_area((void *) to, sizeof(struct sigaction));//验证to处的内存是否足够
    for (i = 0; i < sizeof(struct sigaction); i++) {
        put_fs_byte(*from, to);//复制到fs段。一般是用户数据段。
        from++;//puts_fs_byte()在segment.h中。
        to++;
    }
}

//把sigaction的数据从fs段从from位置复制到to处。
static inline void get_new(char *from, char *to) {
    unsigned int i;
    for (i = 0; i < sizeof(struct sigaction); i++)
        *(to++) = get_fs_byte(from++);

}

//signal()系统调用。类似sigaction()。为指定的信号安装新的信号句柄（信号处理程序）。信号句柄
//可以是用户指定的函数，也可以是SIG_DEF（默认句柄）或SIG_IGN（忽略）。参数signum-指定的信号；
//handler-指定的句柄；restorer-恢复函数指针，该函数由Libc库提供。

int sys_signal(int signum, long handler, long restorer) {
    struct sigaction tmp;
    if (signum < 1 || signum > 32 || signum == SIGKILL) //验证信号，1-32内并且不能是SIGKILL
        return -1;
    tmp.sa_handler = (void (*)(int)) handler;//指定的信号处理句柄
    tmp.sa_mask = 0;//执行时的信号屏蔽码
    tmp.sa_flags = (int) (SA_ONESHOT | SA_NOMASK);//该句柄只使用1次后就恢复到默认值，并允许信号在自己的处理句柄中收到。
    tmp.sa_restorer = (void (*)(void)) restorer;//保存恢复处理函数指针
    handler = (long) current->sigaction[signum - 1].sa_handler;
    current->sigaction[signum - 1] = tmp;
    return handler;
}

//sigaction()系统调用。改变进程在收到一个信号时的操作。signum是除了SIGKILL以外的任何信
//号。如果新操作action不为空则新操作被重新安装。如果oldaction指针不为空，则原操作被保留到
//oldaction.成功则返回0，否则为-1.
int sys_sigaction(int signum, const struct sigaction *action,
                  struct sigaction *old_action) {
    struct sigaction tmp;
#ifdef DEBUG
    s_printk("[DEBUG] sys_sigaction entered, signum = %d\n", signum);
#endif
    if (signum < 1 || signum > 32 || signum == SIGKILL)//验证信号
        return -1;
    tmp = current->sigaction[signum - 1];//在信号sigaction结构中设置新操作（动作）。
#ifdef DEBUG
    s_printk("[DEBUG] sigaction MARK %d\n", __LINE__);
#endif
    get_new((char *) action, (char *) &(current->sigaction[signum - 1]));
    if (old_action)//若oldaction指针不空，则将原操作指针存到oldaction所指的位置。
        save_old((char *) &tmp, (char *) old_action);
    //如果允许信号在自己的信号句柄中收到，则令屏蔽码为0，否则设置屏蔽本信号。
    if (current->sigaction[signum - 1].sa_flags & SA_NOMASK)
        current->sigaction[signum - 1].sa_mask = 0;
    else
        current->sigaction[signum - 1].sa_mask |= (sigset_t)(1 << (signum - 1));
#ifdef DEBUG
    s_printk("[DEBUG] current pid = %d\n", current->pid);
    dump_sigaction(&current->sigaction[signum - 1]);
#endif
    return 0;
}


//系统调用中断处理程序中真正的信号处理程序。
//该段代码的主要作用是将信号的处理句柄插入到用户程序堆栈中，并在本系统调用结束返回后立刻
//执行信号句柄程序，然后继续执行用户的程序。
void do_signal(long signr, long eax, long ebx, long ecx,
               long edx, long fs, long es, long ds,
               unsigned long eip, long cs, long eflags, unsigned long *esp, long ss) {
#ifdef DEBUG
    s_printk("[DEBUG] Signal = %d\n", signr);
    s_printk("[DEBUG] Context signr = %d, eax = 0x%x, ebx = 0x%x, ecx = 0x%x\n \
            edx = 0x%x, fs = 0x%x, es = 0x%x, ds = 0x%x\n \
            eip = 0x%x, cs = 0x%x, eflags = 0x%x, esp = 0x%x, ss= 0x%x\n",
             signr, eax, ebx, ecx, edx, fs, es, ds, eip, cs, eflags, esp, ss);
    s_printk("[DEBUG] current pid = %d\n", current->pid);
    dump_sigaction(&current->sigaction[signr - 1]);
#endif

    unsigned long sa_handler;
    unsigned long old_eip = eip;
    struct sigaction *sa = current->sigaction + signr - 1;//current->sigaction[signr - 1]
    unsigned int longs;
    unsigned long *tmp_esp;

    sa_handler = (unsigned long) sa->sa_handler;
    //如果信号句柄为SIG_IGN（忽略），则返回；如果句柄为SIG_DFL（默认处理），则：如果信号是SIGCHLD
    //则返回，否则终止进程的运行
    if (sa_handler == 1)//SIG_IGN
    {
        return;
    }
    if (!sa_handler)//SIG_DFL
    {
        if (signr == SIGCHLD)
            return;
        else
            panic("Default signal handler");
    }

    //如果该信号句柄只需使用一次，则将该句柄置空（该信号句柄已经保存在sa_handler指针中）
    if (sa->sa_flags & SA_ONESHOT)
        sa->sa_handler = NULL;

    //下面这段代码用信号句柄替换内核堆栈中原用户程序eip，同时也将sa_restorer,signr,进程屏蔽码
    //（如果SA_NOMASK）没置位，eax，ecx，edx作为参数以及原调用系统调用的程序返回指针及标志寄存器
    //值压入堆栈。因此在本次调用中断（0x80）返回用户程序时会首先执行用户的信号句柄程序，然后再继续执行
    //用户程序。下面一句是将用户调用系统调用的代码指针eip指向该信号处理句柄。
    *(&eip) = sa_handler;

    //如果允许信号自己的处理句柄收到信号自己，则也需要将进程的阻塞码压入堆栈。
    longs = (sa->sa_flags & SA_NOMASK) ? 7 : 8;

    //longs 的结果应该选择(7*4):(8*4)，因为堆栈是以4字节为单位操作的。
    //将原调用程序的用户的堆栈指针向下拓展7（或8）个长字（用来存放调用信号句柄的参数等），并
    //检查内存使用情况（例如如果内存超界则分配新页等）。
    *(&esp) -= longs;
    verify_area(esp, longs * 4);

    //在用户堆栈中从下到上存放sa_restorer、信号signr、屏蔽码blocked（如果SA_NOMASK置位）、eax、
    //ecx、edx、eflags和用户程序原代码指针。
    tmp_esp = (unsigned long *) esp;
    put_fs_long((unsigned long) sa->sa_restorer, tmp_esp++);
    put_fs_long((unsigned long) signr, tmp_esp++);
    if (!(sa->sa_flags & SA_NOMASK))
        put_fs_long((unsigned long) current->blocked, tmp_esp++);
    put_fs_long((unsigned long) eax, tmp_esp++);
    put_fs_long((unsigned long) ecx, tmp_esp++);
    put_fs_long((unsigned long) edx, tmp_esp++);
    put_fs_long((unsigned long) eflags, tmp_esp++);
    put_fs_long((unsigned long) old_eip, tmp_esp++);
    current->blocked |= sa->sa_mask;//进程阻塞码（屏蔽码）添上sa_mask中的码位
}

#undef DEBUG
