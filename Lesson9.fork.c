//#include <errno.h> 当前所有错误号为stub状态
#include <linux/sched.h>
#include <asm/system.h>
#include <linux/mm.h>
#include <linux/kernel.h>

//写页面验证。若页面不可写，则复制页面。
extern void write_verify(unsigned long address);

long last_pid = 0; //最新进程号，其值会由 get_empty_process()生成。

//该函数对当前进程逻辑地址从 addr 到 addr + size 这一段范围以页为单位执行写操作
//前的检测操作。由于检测判断是以页面为单位进行操作，因此程序首先需要找出 addr 所在页
//面开始地址 start，然后 start 加上进程数据段基址，使这个 start 变换成 CPU 4G
//线性空间的地址。最后循环调用 write_verify()对指定大小的内存进行写前验证。
//当时的CPU还不支持 WP 位，在执行内核代码时用户空间中的数据页面保护标志没有作用
//无法以此实现写时复制机制。
void verify_area(void *addr, unsigned int size)
{
    unsigned long start;
    //首先将起始地址 start 调整为其所在页的左边界开始位置，同时相应地调整验证区域大小。
    start = (unsigned long)addr;
    size += start & 0xfff; //获得且加上指定起始位置 addr(start) 在所在页面中的偏移值
    start &= 0xfffff000;   //此时 start 是当前进程空间中的逻辑地址
                           //下面把 start 加上进程数据段在线性地址空间中的起始基址，变成系统整个线性空间中的地址位置。
    start += get_base(current->ldt[2]);
    while (size)
    {
        size -= 4096;
        write_verify(start);
        start += 4096;
    }
}

//复制内存页表
//nr 是新任务号；p 是新任务数据结构指针。该函数为新任务在线性地址空间中设置代码段
//和数据段基址、限长，并复制页表。
int copy_mem(int nr, struct task_struct *p)
{
    unsigned long old_data_base, new_data_base, data_limit;
    unsigned long old_code_base, new_code_base, code_limit;

    //首先取当前进程局部描述符表中代码段描述符和数据段描述符项中的段限长（字节数）。
    //0x0f是代码段选择符；0x17是数据段选择符。然后取当前进程代码段和数据段在线性地址空间中
    //的基地址。
    //Linux v0.11 内核不支持代码段和数据段分立。
    code_limit = get_limit(0x0f);
    data_limit = get_limit(0x17);
    old_code_base = get_base(current->ldt[1]);
    old_data_base = get_base(current->ldt[2]);
    if (old_code_base != old_data_base)                     //这绝不应该出现
        panic("Codeseg not fully overlapped with dataseg"); //代码段不能和数据段分立
    if (data_limit < code_limit)
        panic("Bad data limit");

    //然后设置创建中的新进程在线性地址空间中的基地址等于（64MB*其任务号），并且该值
    //设置新进程局部描述符表中段描述符中的基地址。接着设置新进程的页目录表项和页表项。
    new_data_base = new_code_base = (unsigned int)nr * 0x4000000; // 每个新进程 64M
    p->start_code = new_code_base;
    set_base(p->ldt[1], new_code_base);
    set_base(p->ldt[2], new_data_base);
    if (copy_page_tables(old_data_base, new_data_base, data_limit))
    {
        printk("free_page_tables: from copy_mem\n");
        free_page_tables(new_data_base, data_limit);
        return -1;
    }
    return 0;
}

//下面是主要的 fork 子程序。它复制系统进程信息（task[n]）
//拷贝 PCB 以及代码段、数据段。
//C语言函数参数的获取顺序是参数列表后面的参数对应在汇编中先压栈的参数，即离栈顶最远的参数。
//首先 CPU 进入中断调用，压栈SS, ESP, 标志寄存器 EFLAGS 和返回地址CS:EIP
//然后 system_call 里压栈的寄存器 ds, es, fs, edx, ecx, ebx
//然后调用 sys_fork 函数时，压入的函数返回地址
//以及sys_fork里压栈的参数 gs, esi, edi, ebp, eax(nr)
int copy_process(int nr, long ebp, long edi, long esi,
                 long gs, long none, long ebx, long ecx, long edx, long fs,
                 long es, long ds, long eip, long cs, long eflags, long esp, long ss)
{
    struct task_struct *p;
    int i;

    /* Only for emit the warning! */
    i = none;
    none = i;
    //i = eflags;
    //eflags = i;
    /* End */

    p = (struct task_struct *)get_free_page(); //为新任务分配页
    if (!p)                                    //若内存分配错误
        return -1;                             //返回出错码，退出
    task[nr] = p;                              //新任务结构指针放入任务数组nr项。
    *p = *current;                             //把当前进程任务结构内容复制到刚申请到的内存页面p开始处
                                               //这里仅仅复制 PCB(task_struct)

    //初始化进程控制块 PCB，任务状态段 TSS

    //随后对复制来的进程结构内容进行修改，作为新进程的任务结构，先将新进程的状态
    //置为不可中断等待状态，以防内核调度其执行。然后设置新进程的进程号 pid 和父进
    //程号 father，并初始化进程运行时间片值等于其 priority 值。接着复位新进程
    //的信号位图、报警定时值、会话、领导标志、进程及其子进程在内核和用户态运行时间
    //统计值，还设置进程开始运行的系统时间 start_time。
    p->state = TASK_UNINTERRUPTIBLE;
    p->pid = last_pid;        //新进程号
    p->father = current->pid; //父进程号
    p->counter = p->priority; //时间片值
    p->signal = 0;
    p->alarm = 0;              //报警定时值
    p->leader = 0;             //进程的领导权是不能继承的
    p->utime = p->stime = 0;   //用户态时间和核心态时间
    p->cutime = p->cstime = 0; //子进程用户态和核心态运行时间
    p->start_time = jiffies;   //进程开始运行时间
    //再修改任务状态段 TSS 数据。
    p->tss.back_link = 0;
    p->tss.eflags = eflags;            //标志寄存器
    p->tss.esp0 = PAGE_SIZE + (long)p; //任务内核态栈指针
    p->tss.ss0 = 0x10;                 //内核态栈的段选择符（与内核数据段相同）
    p->tss.eip = eip;                  //指令代码指针
    p->tss.eax = 0;                    //这是当fork()返回的时，新进程返回0的原因
    p->tss.ecx = ecx;
    p->tss.ebx = ebx;
    p->tss.edx = edx;
    p->tss.esp = esp;
    p->tss.ebp = ebp;
    p->tss.esi = esi;
    p->tss.edi = edi;
    p->tss.es = es & 0xffff; //段寄存器仅16位有效
    p->tss.cs = cs & 0xffff;
    p->tss.ds = ds & 0xffff;
    p->tss.ss = ss & 0xffff;
    p->tss.fs = fs & 0xffff;
    p->tss.gs = gs & 0xffff;
    p->tss.ldt = _LDT(nr); //任务局部表描述符的选择符（LDT描述符在GDT中）
    p->tss.bitmap = 0x80000000;

    //如果当前任务使用了协处理器，就保存其上下文
    if (last_task_used_math == current)
        __asm__("clts; fnsave %0" ::"m"(p->tss.i387));

    //复制进程页表。即在线性地址空间中设置新任务代码段和数据段描述符中的基址
    //和限长，并复制页表。如果出错（返回值不是0），则复位任务数组中相应项并
    //释放为该新任务分配的用于任务结构的内存页。
    if (copy_mem(nr, p)) //返回不为0表示出错
    {
        task[nr] = NULL;
        free_page((unsigned long)p);
        return -1;
    }

    //设置好 TSS 和 LDT 描述符，然后将任务置位
    //就绪状态，可被OS调度
    set_tss_desc(gdt + (nr << 1) + FIRST_TSS_ENTRY, &(p->tss));
    set_ldt_desc(gdt + (nr << 1) + FIRST_LDT_ENTRY, &(p->ldt));
    p->state = TASK_RUNNING; //最后才将新任务置为就绪态

    return last_pid; // 父进程返回新进程的pid
}

//为新进程取得不重复的进程号 last_pid（全局变量）。函数返回在任务数组中的任务号（数组项）。
int find_empty_process(void)
{
    int i;
    long tmp = last_pid; //记录最初起始进程号，用于标记循环结束

    //last_pid 是全局变量，不用返回
    while (1)
    {
        for (i = 0; i < NR_TASKS; i++)
        {
            if (task[i] && task[i]->pid == last_pid) //pid 被占用
                break;
        }
        if (i == NR_TASKS)
            break;
        if ((++last_pid) == tmp)
            break;
        //判断 last_pid 是否超出进程号数据表示范围，如果超出则重新从1开始使用 pid 号
        if (last_pid < 0)
            last_pid = 1;
    }
    for (i = 1; i < NR_TASKS; i++) //遍历任务数组，任务0 项被排除在外
        if (!task[i])              //如果为空
            return i;
    return -1;
}
