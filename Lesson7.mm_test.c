//实现部分内存管理的功能。

#include <linux/kernel.h>

#define invalidate() __asm__ volatile("mov %%eax, %%cr3" ::"a"(0))
//用于使得TLB（页表缓存）失效，刷新缓存。

unsigned long put_page(unsigned long page, unsigned long address);
//释放物理内存地址addr开始的一页面内存。修改页面映射数组mem_map[]中引用次数信息。

void testoom()
{
    for (int i = 0; i < 20 * 1024 * 1024; i += 4096)
        do_no_page(0, i); //处理缺页异常
    return;
}

void test_put_page()
{
    char *b = 0x100000;
    calc_mem(); //显示当前内存用量
    *b = 'k';
    while (1)
        ;
    return;
}

//将线性地址转换为PTE
//若成功返回物理地址，若失败返回NULL
unsigned long *linear_to_pte(unsigned long addr)
{
    //首先获取页目录表，后面会被用于引用pde
    unsigned long *pde = (unsigned long *)((addr >> 20) & 0xffc);
    if (!(*pde & 1) || pde > 0x1000) //页目录表不存在或地址不在页表范围内
    {
        return 0;
    }
    //页表地址 + 页表索引 = PTE
    pde = (unsigned long *)(*pde & 0xfffff000);
    return pde + ((addr >> 12) & 0x3ff);
}

void disable_linear(unsigned long addr)
{
    unsigned long *pte = linear_to_pte(addr);
    *pte = 0;
    invalidate(); //刷新页表缓存
    return;
}

void mm_read_only(unsigned long addr)
{
    unsigned long *pte = linear_to_pte(addr);
    printk("Before: 0x%x\n", *pte);
    *pte = *pte & 0xfffffffd;
    printk("After: 0x%x\n", *pte);
    invalidate();
    return;
}

void mm_print_pageinfo(unsigned long addr)
{
    unsigned long *pte = linear_to_pte(addr);
    printk("Linear addr: 0x%x, PTE addr: 0x%x. Flags[ ", addr, pte);
    if (*pte & 0x1)
        printk("P ");
    if (*pte & 0x2)
        printk("R/W");
    else
        printk("RO ");
    if (*pte & 0x4)
        printk("U/S ");
    else
        printk("S ");
    printk("]\n");
    printk("Phyaddr = %x\n", (*pte & 0xfffff000));
}

int mmtest_main(void)
{
    int i = 0;
    printk("Running Memory function tests\n");
    printk("1. Make Linear Address 0xdad233 unavailable\n");

    disable_linear(0xdad233);

    printk("2. Put page(0x300000) at linear address 0xdad233\n");
    put_page(0x300000, 0xdad233);

    unsigned long *x = 0xdad233;
    *x = 0x23333333;
    printk("X = %x\n", *x);

    printk("3. Make 0xdad233 READ ONLY\n");
    mm_read_only(0xdad233);
    x = 0xdad233;

    asm volatile("mov %%cr0, %%eax\n\t"
                 "orl $0x00010000, %%eax\n\t"
                 "mov %%eax, %%cr0\n\t" ::);

    printk("4. Print the page info of 0xdad233 in human readable mode\n");
    mm_print_pageinfo(0xdad233);
}
