//traps.c主要包括一些处理异常故障代码（硬件中断）底层代码asm.s文件中调用的相应C函数
//用于显示出错位置和出错号等调试信息。
//die()通用函数用于在中断处理中显示详细的出错信息

#include <linux/head.h> //定义了段描述符的简单结构，和几个选择符常量。
#include <asm/io.h>     //输入/输出头文件。定义硬件端口输入/输出宏汇编语句。
#include <asm/system.h> //系统头文件。定义了设置或修改描述符/中断门等的嵌入式汇编宏。

void page_fault(void);

// 在kernel/asm.s中使用的函数原型
void divide_error(void);
void debug(void);
void nmi(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void double_fault(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void coprocessor_error(void);
void irq13(void);
void reserved(void);
void parallel_interrupt(void);

//用来引发一个异常，因为目前 NEUOS 没有实现信号机制，且为单进程，所以die的处理
//就是打印完必要的信息，停机进入死循环
static void die(char *str, long esp_ptr, long nr)
{
    long *esp = (long *)esp_ptr;
    //打印出错中断的名称、出错号。
    printk("%s: %x", str, nr & 0xffff);
    //下行打印语句显示当前调用进程的 CS:EIP、EFLAGS 和 SS:ESP 的值。
    //esp[1]是段选择符cs，esp[0]是eip，esp[2]是eflags，esp[4]是原ss，esp[3]是原esp
    printk("EIP: %x:%x\n EFLAGS: %x\n ESP %x:%x\n",
           esp[1], esp[0], esp[2], esp[4], esp[3]);

    printk("No Process now, System HALT!   :(\n"); //系统宕机。
    for (;;)
        ;
    return;
}

//以下以do_开头的函数是asm.s中对应中断处理程序调用的C函数。
void do_double_fault(long esp, long error_code)
{
    die("double fault", esp, error_code);
}

void do_general_protection(long esp, long error_code)
{
    die("general protection", esp, error_code);
}

void do_divide_error(long esp, long error_code)
{
    die("divide error", esp, error_code);
}

//参数是进入中断后被顺序压入堆栈的寄存器值。参见asm.s程序。
void do_int3(long *esp, long error_code,
             long fs, long es, long ds,
             long ebp, long esi, long edi,
             long edx, long ecx, long ebx, long eax)
{

    // 这里尚不支持进程注册。
    int tr = 0;
    __asm__ volatile("str %%ax"
                     : "=a"(tr)
                     : "0"(0));

    printk("eax\tebx\tecx\tedx\t\n%x\t%x\t%x\t%x\n", eax, ebx, ecx, edx);
    printk("esi\tedi\tebp\tesp\t\n%x\t%x\t%x\t%x\n", esi, edi, ebp, (long)esp);
    printk("ds\tes\tfs\ttr\n%x\t%x\t%x\t%x\n", ds, es, fs, tr);
    printk("EIP: %x    CS:%x     EFLAGS: %x", esp[0], esp[1], esp[2]);
    return;
}

void do_nmi(long esp, long error_code)
{
    die("nmi", esp, error_code);
}

void do_debug(long esp, long error_code)
{
    die("debug", esp, error_code);
}

void do_overflow(long esp, long error_code)
{
    die("overflow", esp, error_code);
}

void do_bounds(long esp, long error_code)
{
    die("bounds", esp, error_code);
}

void do_invalid_op(long esp, long error_code)
{
    die("invalid operand", esp, error_code);
}

void do_device_not_available(long esp, long error_code)
{
    die("device not available", esp, error_code);
}

void do_coprocessor_segment_overrun(long esp, long error_code)
{
    die("coprocessor segment overrun", esp, error_code);
}

void do_invalid_TSS(long esp, long error_code)
{
    die("invalid TSS", esp, error_code);
}

void do_segment_not_present(long esp, long error_code)
{
    die("segment not present", esp, error_code);
}

void do_stack_segment(long esp, long error_code)
{
    die("stack segment", esp, error_code);
}

void do_coprocessor_error(long esp, long error_code)
{
    die("coprocessor error", esp, error_code);
}

void do_reserved(long esp, long error_code)
{
    die("reserved(15, 17-47)error", esp, error_code);
}

void do_stub(long esp, long error_code)
{
    printk("stub interrupt!\n");
}

//下面是异常（陷阱）中断程序初始化子程序。设置它们的中断调用门（中断向量）。
//set_trap_gate()与set_system_gate()都使用了中断描述符表IDT中的陷阱门。
//它们之间的主要区别在于前者设置的特权级为0，后者是3.因此断点陷阱中断 int3、
//溢出中断 overflow 和边界出错中断 bounds 可由任何程序产生。
void trap_init(void)
{
    int i;

    set_trap_gate(0, &divide_error); //设置除操作出错的中断向量值。
    set_trap_gate(1, &debug);
    set_trap_gate(2, &nmi);
    set_system_gate(3, &int3); //int3-5 可被所有程序执行。
    set_system_gate(4, &overflow);
    set_system_gate(5, &bounds);
    set_trap_gate(6, &double_fault);
    set_trap_gate(7, &device_not_available);
    set_trap_gate(8, &double_fault);
    set_trap_gate(9, &coprocessor_segment_overrun);
    set_trap_gate(10, &invalid_TSS);
    set_trap_gate(11, &segment_not_present);
    set_trap_gate(12, &stack_segment);
    set_trap_gate(13, &general_protection);
    set_trap_gate(14, &page_fault);
    set_trap_gate(15, &reserved);
    set_trap_gate(16, &coprocessor_error);
    //下面把 int 17-47的陷阱门先均设置为 reserved，以后各硬件初始化时会重新
    //设置自己的陷阱门。
    for (i = 17; i < 48; i++)
    {
        set_trap_gate(i, &reserved);
    }
    //设置协处理器中断 0x2d（45）陷阱门描述符，并允许其产生中断请求。设置并行口中断描述符。
    set_trap_gate(45, &irq13);
    set_trap_gate(39, &parallel_interrupt);//设置并行口1的中断 0x27 陷阱门描述符。
}
