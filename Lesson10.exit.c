#include <errno.h>//错误号头文件，包含系统中各种出错号。
#include <signal.h>//信号头文件
#include <linux/sched.h>//调度程序头文件
#include <linux/kernel.h>//内核头文件
#include <linux/tty.h>//tty头文件，定义了有关tty_io，串行通信方面的参数、常数

//向指定任务p发送信号sig，权限为priv
//sig信号值，p指定任务的指针，priv强制发送信号的标志。即不需要考虑进程用户属性或级别而能发送信号的权利。
//该函数首先判断参数的正确性，然后判断条件是否满足。若满足就向指定进程发送信号sig并退出，否则返回未许可错误号。
static inline int send_sig(long sig, struct task_struct *p, int priv) {
    //若信号不正确或任务指针为空则出错退出
    if (!p || sig < 1 || sig > 32)
        return -EINVAL;
    //如果强制发送标志priv置位，或者当前进程的有效用户标识符euid就是指定进程的euid，或者当前进程是超级用户，
    //则向进程p发送信号sig，即在进程p位图中添加该信号，否则出错退出。
    //neuos目前没有用户权限检查
    if (priv || (current->euid == p->euid) /* ||suser() */) {
        p->signal |= (1 << (sig - 1));
    } else
        return -EPERM;
    return 0;
}

//sys_kill为系统调用 kill 的入口点
//可用于向任何进程或进程组发送任何信号，而并非只是杀死进程
//参数pid是进程号，sig是需要发送的信号
int sys_kill(int pid, int sig) {
    struct task_struct **p = task + NR_TASKS;
    int err, retval = 0;
    s_printk("sys_kill entered\n");

    //目前没有进程组的概念，只处理pid>0的情况
    //pid>0，发送给进程号是pid的进程
    //pid=0，发送给当前进程的进程组中的所有进程
    //pid=-1，发送给除第一个进程外(初始进程init)的所有进程
    //pid<-1，发送给进程组-pid中的所有进程
    //若sig为0，则不发送任何信号，但仍会进行错误检查，如果成功返回0
    if (pid > 0) {
        while (--p > &FIRST_TASK) {
            if (*p && (*p)->pid == pid) {
                // neuos目前没有实现sys.c相关的逻辑，此处暂时给予执行kill的用户最高权限（priv==1）
                if (err = send_sig(sig, *p, 1)) {
                    s_printk("send_sig error err = %d\n", err);
                    retval = err;
                }
            }
        }
    }
}