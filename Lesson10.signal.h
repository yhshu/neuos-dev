//信号处理的具体函数实现见程序kernel/signal.c

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>// 类型头文件，定义了基本的系统数据类型


typedef int sig_atomic_t;//定义信号原子操作类型
typedef unsigned int sigset_t;//定义信号集类型

#define _NSIG 32 //32种信号，下面的信号集也是32位
#define NSIG _NSIG

//定义信号宏

#define SIGHUP        1 //Hang up 挂断控制终端或进程
#define SIGINT        2 //Interrupt 来自键盘的中断
#define SIGQUIT       3 //Quit 来自键盘的退出
#define SIGILL        4 //Illeagle 非法指令
#define SIGTRAP       5 //Trap 跟踪断点
#define SIGABRT       6 //Abort 异常结束
#define SIGIOT        6 //IO Trap 同上
#define SIGUNUSED     7 ///Unused 没有使用
#define SIGFPE        8 //FPE 协处理器
#define SIGKILL       9 //Kill 强迫进程终止
#define SIGUSR1       10//User1 用户信号1，进程可使用
#define SIGSEGV       11//Segment Violation 无效内存引用
#define SIGUSR2       12//User2 用户信号2，进程可使用
#define SIGPIPE       13//Pipe 管道写出错，无读者
#define SIGALRM       14//Alarm 实时定时器报警
#define SIGTERM       15//Terminate 进程终止
#define SIGSTKFLT     16//Stack Fault 栈出错（协处理器）
#define SIGCHLD       17//Child 子进程停止或被终止
#define SIGCONT       18//Continue 恢复进程继续执行
#define SIGSTOP       19//Stop 停止进程的执行
#define SIGTSTP       20//TTY Stop TTY发出停止进程，可忽略
#define SIGTTIN       21//TTY In 后台进程请求输入
#define SIGTTOU       22//TTY Out 后台进程请求输出

//定义 sigaction 需要的结构
#define SA_NOCLDSTOP 1 //当子进程处于停止状态，就不对SIGCHLD处理
#define SA_NOMASK 0x40000000 // 不阻止在指定的信号处理程序中再收到该信号
#define SA_ONESHOT 0x80000000 //信号句柄一旦被调用过就恢复到默认处理句柄

#define SIG_BLOCK 0//在阻塞信号集中加上给定信号
#define SIG_UNBLOCK 1//从阻塞信号集中删除指定信号
#define SIG_SETMASK 2//设置阻塞信号集

#define SIG_DFL ((void(*) (int))0)  // 默认信号处理句柄
#define SIG_IGN ((void(*) (int))1)  // 忽略信号的处理程序

void (*signal(int _sig, void(*func)(int)))(int);

int raise(int sig);//向自身发信号
int kill(pid_t pid, int sig);//向某个进程发信号

typedef unsigned int sigset_t;//32位信号集，一位表示一个信号

//下面是sigaction的数据结构
//sa_handler 是对应某信号指定要采取的行动。可以用上面的SIG_DFL，或SIG_ING来
//忽略该信号，也可以是指向处理该信号函数的一个指针。
//sa_mask 给出了对信号的屏蔽码，在信号程序执行时将阻塞对这些信号的处理。
//sa_flags 指定改变信号处理过程中的信号集。
//sa_restorer 是恢复函数指针，由函数库Libc提供，用于清理用户态堆栈。
//引起触发信号处理的信号也将被阻塞，除非使用了SA_NOMASK标志。
struct sigaction {
    void (*sa_handler)(int);

    sigset_t sa_mask;
    int sa_flags;

    void (*sa_restorer)(void);
};

//对阻塞信号集的操作
int sigaddset(sigset_t *mask, int signo);

int sigdelset(sigset_t *mask, int signo);

int sigemptyset(sigset_t *mask);

int sigfillset(sigset_t *mask);

int sigismember(sigset_t *mask, int signo);

int sigpending(sigset_t *set);

int sigprocmask(int how, sigset_t *set, sigset_t *oldset);

//改变对于某个信号sig的处理过程
int sigaction(int sig, struct sigaction *act, struct sigaction *oldact);

#endif